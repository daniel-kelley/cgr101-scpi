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

struct lexer {
    yyscan_t scanner;
};

struct parser {
    yypstate *ps;
    struct {
        int valid;
        const YYLTYPE *loc;
        char *msg;
    } error;
    int trace_yyerror;
};

void yyerror(const YYLTYPE *loc, struct info *info, const char *s)
{
    /* 's' may be transient, (it is last I looked) so make a copy. */
    if (info->parser->error.msg) {
        free(info->parser->error.msg);
    }
    info->parser->error.loc = loc;
    info->parser->error.msg = strdup(s);
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

    if (info->parser->error.msg) {
        free(info->parser->error.msg);
    }

    if (info->parser) {
        free(info->parser);
    }

    return 0;
}

static void parser_loop(struct info *info,
                        yyscan_t scanner)
{
    int status;
    int c;
    YYSTYPE yys;
    YYLTYPE yyl;

    do {
        memset(&yys, 0, sizeof(yys));
        memset(&yyl, 0, sizeof(yyl));
        c = yylex(&yys, &yyl, scanner);
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
int parser_send(struct info *info, char *line, int len, int *busy)
{
    YY_BUFFER_STATE bs;
    yyscan_t scanner = info->lexer->scanner;
    int err = 1;

    info->busy = 0;

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
        }

        parser_loop(info, scanner);
        if (!info->busy) {
            yypstate_delete(info->parser->ps);
        }
        yy_delete_buffer(bs, scanner);
        err = (info->parser->error.valid);
    } while (0);

    *busy = info->busy;

    return err;
}

int parser_error_get(struct info *info, const char **msg, int *trace)
{
    int err = 1;

    if (msg != NULL && trace != NULL && info->parser->error.valid) {
        *msg = info->parser->error.msg;
        *trace = info->parser->trace_yyerror;
        info->parser->error.valid = 0;
        err = 0;
    }

    return err;
}

static void parser_int(const char *s, struct scpi_type *val)
{
    /* FIXME: needs error checking. */
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

static void parser_float(const char *s, struct scpi_type *val)
{
    char p[32];

    parser_squeeze(p,s);
    /* FIXME: needs error checking. */
    val->type = SCPI_TYPE_FLOAT;
    sscanf(p,"%lf",&val->val.fval);
    val->src = s;
}

int parser_num(const char *s, struct scpi_type *val, int token)
{
    if (token == NUM) {
        parser_int(s, val);
    } else if (token == FLOAT) {
        parser_float(s, val);
    } else {
        assert(0);
    }

    return token;
}
