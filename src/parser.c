/*
   parser.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "parser.h"
#include "scpi_type.h"
#include "yyerror.h"
#include "scpi.tab.h"
#include "scanner.h"

/*
 * Maximum command depth in grammar, i.e. maximum number of ':'
 * separators, including separators.
 */
#define MAX_CMD_DEPTH (5*2)

struct lexer {
    yyscan_t scanner;
};

struct parser {
    yypstate *ps;
    struct {
        int valid;
        struct parser_msg_loc data;
    } error;
    int trace_yyerror;
    int trace_loop;
    int separator;
    int prefix_length;
    int prefix[MAX_CMD_DEPTH];
    struct {
        int active;
        int idx;
    } replay;
};

enum parser_loop_state {
    LEX,
    PREFIX,
};

struct parser_strpool_s {
    size_t len;
    size_t max;
    char **pool;
};

static struct parser_strpool_s parser_strpool;

void yyerror(const YYLTYPE *loc, struct info *info, const char *s)
{
    /* 's' may be transient, (it is last I looked) so make a copy. */
    if (info->parser->error.data.msg) {
        free(info->parser->error.data.msg);
    }
    info->parser->error.data.loc = loc;
    info->parser->error.data.msg = strdup(s);
    info->parser->error.valid = 1;
    info->busy = 0;
    if (info->verbose) {
        fprintf(stderr,">>> %s:\n", s);
    }
}

void parser_input(struct info *info, char *buf, int result, int max_size)
{
    (void)info;
    (void)buf;
    (void)result;
    (void)max_size;
}

int parser_init(struct info *info)
{
    int rv = -1;
    do {
        info->lexer = calloc(1, sizeof(*info->lexer));
        if (!info->lexer) {
            break;
        }

        info->parser = calloc(1, sizeof(*info->parser));
        if (!info->parser) {
            break;
        }

        if (yylex_init_extra(info, &info->lexer->scanner)) {
            break;
        }

        if (info->debug) {
            if (strchr(info->debug,'f')) {
                yyset_debug(1, info->lexer->scanner);
            }
            if (strchr(info->debug,'b')) {
                yydebug = 1;
            }
            if (strchr(info->debug,'y')) {
                info->parser->trace_yyerror = 1;
            }
            if (strchr(info->debug,'L')) {
                info->parser->trace_loop = 1;
            }
        }

        rv = 0;

    } while (0);

    return rv;
}

int parser_done(struct info *info)
{
    if (info->lexer) {
        yylex_destroy(info->lexer->scanner);
        free(info->lexer);
    }

    if (info->parser->error.data.msg) {
        free(info->parser->error.data.msg);
    }

    if (info->parser) {
        free(info->parser);
    }

    return 0;
}

static void parser_replay_reset(struct parser *parser)
{
    parser->replay.active = 0;
    parser->replay.idx = 0;
    parser->separator = 0;
}

static int parser_replay_prefix(struct parser *parser)
{
    int token;

    assert(parser->replay.active);
    token = parser->prefix[parser->replay.idx++];
    assert(token);

    /* Reset when the last prefix token is returned. */
    if (parser->replay.idx == parser->prefix_length) {
        parser_replay_reset(parser);
    }

    return token;
}

static int parser_replay_active(struct parser *parser)
{
    /*
     * If not currently active and a separator is seen, make replay
     * active.
     */
    if (!parser->replay.active && parser->separator) {
        parser->replay.active = 1;
    }

    return parser->replay.active;
}

static void parser_reset_prefix(struct parser *parser)
{
    parser->replay.active = 0;
    parser->prefix_length = 0;
    memset(parser->prefix, 0, sizeof(parser->prefix));
}

int parser_separator(struct info *info, int value)
{
    int separator = info->parser->separator;

    if (info->parser->trace_loop) {
        fprintf(stderr, "[LOOP] %s: separator:%d->%d\n",
                __func__, separator, value);
    }
    info->parser->separator = value;
    if (!value) {
        parser_reset_prefix(info->parser);
    }

    return separator;
}

/* Somewhat complicated by the interaction of ':' cancelling
 * prefixes. */
static void parser_loop(struct info *info,
                        yyscan_t scanner)
{
    int status;
    int c;
    YYSTYPE yys;
    YYLTYPE yyl;
    enum parser_loop_state state = LEX;
    int suffix = 0;
    int active;

    parser_replay_reset(info->parser);
    do {
        memset(&yys, 0, sizeof(yys));
        memset(&yyl, 0, sizeof(yyl));
        switch (state) {
        case LEX:
            c = yylex(&yys, &yyl, scanner);
            /* If replay is active, then the previous token was ';'
             * and the current token needs to be saved as 'suffix' and
             * the next state becomes PREFIX, *UNLESS* the current
             * token is ':', which cancels the replay, and the state
             * remains LEX.
             */
            active = parser_replay_active(info->parser);
            if (active && c != COLON) {
                suffix = c;
                state = PREFIX;
                c = parser_replay_prefix(info->parser);
            }
            break;
        case PREFIX:
            /* Replay the saved prefix. When done, the next state is
             * LEX with the previously saved 'suffix' token. */
            if (parser_replay_active(info->parser)) {
                c = parser_replay_prefix(info->parser);
            } else {
                c = suffix;
                state = LEX;
            }
            break;
        default:
            assert(0);
        }
        if (info->parser->trace_loop) {
            fprintf(stderr, "[LOOP] %s: %d %d\n", __func__, state, c);
        }
        status = yypush_parse(info->parser->ps, c, &yys, &yyl, info);
    } while (status == YYPUSH_MORE);

}

/*
 * Lexer/Parser feedback: to accommodate 488.2 7.6.1.5 Header
 * Compounding, the lexer needs to keep track of leading program
 * mnemonics and command separators. The idea is that if there is a
 * program mnemonic IDENT immediately after a command separator, then
 * the 'header-path' tokens (supplied by the lexer) are passed back
 * instead of the lexer before supplying the program mnemonic that
 * triggered the header-path injection. The intial command, and any
 * 'absolute' commands that start with ':' reset the header-path.
 *
 * This function handles exactly one line. Multiple lines need to be
 * split up by the caller.
 */
int parser_send(struct info *info, char *line, int len)
{
    YY_BUFFER_STATE bs;
    yyscan_t scanner = info->lexer->scanner;
    int err = 1;

    bs = yy_scan_bytes(line, len, scanner);

    do {
        if (bs == NULL) {
            fprintf(stderr, "yy_scan_bytes(%s)[%d] failed\n", line, len);
            break;
        }

        if (!info->busy) {
            info->parser->ps = yypstate_new();
            if (!info->parser->ps) {
                fprintf(stderr, "yypstate_new() failed\n");
                break;
            }
            info->busy = 1;
            info->rsp.valid = 0;
            parser_separator(info, 0);
        }

        parser_loop(info, scanner);
        if (!info->busy) {
            yypstate_delete(info->parser->ps);
        }
        yy_delete_buffer(bs, scanner);
        err = (info->parser->error.valid);
    } while (0);

    return err;
}

int parser_error_get(struct info *info,
                     struct parser_msg_loc *data,
                     int *trace)
{
    int err = 1;

    assert(data);
    assert(trace);
    if (info->parser->error.valid) {
        *data = info->parser->error.data;
        *trace = info->parser->trace_yyerror;
        info->parser->error.valid = 0;
        err = 0;
    }

    return err;
}

static void parser_update_location(YYLTYPE *loc, size_t len)
{
    loc->first_line = loc->last_line;
    loc->first_column = loc->last_column+1;
    loc->last_column = loc->first_column+(int)len;
}

static void parser_int(const char *s, struct scpi_type *val, YYLTYPE *loc)
{
    /* FIXME: needs error checking. */
    parser_update_location(loc, strlen(s));
    val->type = SCPI_TYPE_INT;
    val->val.ival = strtol(s, NULL, 10);
    val->src = s;
}

/* squeeze the blanks out of src into dst. */
static void parser_squeeze(char *dst, const char *src)
{
    while (*src != 0) {
        if (*src != ' ') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = 0;
}

static void parser_float(const char *s,
                         struct scpi_type *val,
                         YYLTYPE *loc)
{
    char p[32];

    parser_update_location(loc, strlen(s));
    parser_squeeze(p,s);
    /* FIXME: needs error checking. */
    val->type = SCPI_TYPE_FLOAT;
    sscanf(p,"%lf",&val->val.fval);
    val->src = s;
}

int parser_num(const char *s,
               struct scpi_type *val,
               YYLTYPE *loc,
               int token)
{
    if (token == NUM) {
        parser_int(s, val, loc);
    } else if (token == FLOAT) {
        parser_float(s, val, loc);
    } else {
        assert(0);
    }

    return token;
}

#define STRPOOL_CHUNK 256

static const char *parser_strpool_add(struct parser_strpool_s *pool,
                                      const char *s)
{
    char *t;
    void *buf;

    t = strdup(s);
    assert(t);
    if (pool->len >= pool->max) {
        pool->max += STRPOOL_CHUNK;
        buf = realloc(pool->pool, sizeof(pool->pool[0])*pool->max);
        assert(buf);
        pool->pool = buf;
    }
    pool->pool[pool->len++] = t;

    return t;

}

void parser_cleanup(void)
{
    size_t idx;

    for (idx = 0; idx < parser_strpool.len; idx++) {
        free(parser_strpool.pool[idx]);
    }
    free(parser_strpool.pool);
    parser_strpool.len = 0;
    parser_strpool.max = 0;
    parser_strpool.pool = NULL;
}

int parser_ident(const char *s,
                 struct scpi_type *val,
                 YYLTYPE *loc,
                 int token)
{
    parser_update_location(loc, strlen(s));
    val->token = token;
    val->type = SCPI_TYPE_STR;
    val->src = parser_strpool_add(&parser_strpool, s);

    return token;
}

int parser_punct(const char *s,
                 struct scpi_type *val,
                 YYLTYPE *loc,
                 int token)
{
    parser_update_location(loc, strlen(s));
    val->token = token;
    val->type = SCPI_TYPE_STR;
    val->src = parser_strpool_add(&parser_strpool, s);

    return token;
}

void parser_blank(const char *s, struct scpi_type *val, YYLTYPE *loc)
{
    parser_update_location(loc, strlen(s));
    (void)val;
}

void parser_eol(const char *s, struct scpi_type *val, YYLTYPE *loc)
{
    assert(s[0] == '\n');
    assert(s[1] == 0);
    loc->first_line = loc->last_line + 1;
    loc->first_column = 0;
    loc->last_line = loc->first_line;
    loc->last_column = 0;
    (void)val;
}

static void parser_add_prefix_(struct info *info, int token)
{
    assert(info);
    assert(info->parser->prefix_length < MAX_CMD_DEPTH);

    info->parser->prefix[info->parser->prefix_length++] = token;
}

void parser_add_prefix(struct info *info, int token)
{
    if (!info->parser->replay.active) {
        parser_add_prefix_(info, token);
        parser_add_prefix_(info, COLON);
    }
}

#define DSTR_CHUNK 256

void parser_dstr_add(const char *s,
                     struct scpi_type *val,
                     YYLTYPE *loc)
{
    assert(s[0] != 0);
    assert(s[1] == 0);
    assert(val->type == SCPI_TYPE_DSTR);
    assert(!val->val.dstr.frozen);
    parser_update_location(loc, strlen(s));

    if (val->val.dstr.len >= val->val.dstr.max) {
        val->val.dstr.max += DSTR_CHUNK;
        val->val.dstr.s = realloc(val->val.dstr.s, val->val.dstr.max);
    }
    val->val.dstr.s[val->val.dstr.len++] = s[0];
    val->val.dstr.s[val->val.dstr.len] = 0;
}

void parser_dstr_new(const char *s,
                     struct scpi_type *val,
                     YYLTYPE *loc)
{
    assert(s[0] != 0);
    assert(s[1] == 0);
    parser_update_location(loc, strlen(s));
    val->type = SCPI_TYPE_DSTR;
    val->val.dstr.s = NULL;
    val->val.dstr.len = 0;
    val->val.dstr.max = 0;
    val->val.dstr.frozen = 0;
}

void parser_dstr_quote(const char *s,
                       struct scpi_type *val,
                       YYLTYPE *loc)
{
    assert(s[0] != 0);
    assert(s[0] == s[1]);
    assert(s[2] == 0);
    assert(val->type == SCPI_TYPE_DSTR);
    assert(!val->val.dstr.frozen);
    parser_update_location(loc, strlen(s));
    parser_dstr_add(s+1,val,loc);
}

int parser_dstr(const char *s,
                struct scpi_type *val,
                YYLTYPE *loc,
                int token)
{
    assert(val->type == SCPI_TYPE_DSTR);
    assert(!val->val.dstr.frozen);
    parser_update_location(loc, strlen(s));
    /* Tricky: SCPI_TYPE_DSTR is a temporary type; convert to final
     * STR type and free the string accumulation buffer. */
    val->type = SCPI_TYPE_STR;
    val->src = parser_strpool_add(&parser_strpool, val->val.dstr.s);
    val->val.dstr.frozen = 1;
    free(val->val.dstr.s);

    return token;
}

void parser_include(struct info *info, const char *file)
{
    (void)info;
    (void)file;
}
