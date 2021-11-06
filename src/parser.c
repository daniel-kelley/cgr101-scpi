/*
   parser.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "parser.h"
#include "scpi_type.h"
#include "scpi.tab.h"
#include "scanner.h"

struct lexer {
    yyscan_t scanner;
};

struct parser {
    yypstate *ps;
};


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

/* line modified in place, needs *two* NULs at end; len reflects this */
/*
 * Lexer/Parser feedback: to accommodate 488.2 7.6.1.5 Header
 * Compounding, the lexer needs to keep track of leading program
 * mnemonics and command separators. The idea is that if there is a
 * program mnemonic IDENT immediately after a command separator, then
 * the 'header-path' tokens (supplied by the parser) are passed back
 * via the lexer before supplying the program mnemonic that triggered
 * the header-path injection. The intial command, and any 'absolute'
 * commands that start with ':' reset the header-path.
 */
int parser_send(struct info *info, char *line, size_t len)
{
    YY_BUFFER_STATE bs;
    yyscan_t scanner = info->lexer->scanner;
    int err = 1;

    bs = yy_scan_buffer(line, len, scanner);

    do {
        if (bs == NULL) {
            fprintf(stderr, "yy_scan_buffer(%s)[%lu] failed\n", line, len);
            break;
        }

        if (!info->busy) {
            info->parser->ps = yypstate_new();
            if (!info->parser->ps) {
                fprintf(stderr, "yypstate_new() failed\n");
                break;
            }
            info->busy = 1;
        }

        parser_loop(info, scanner);
        if (!info->busy) {
            yypstate_delete(info->parser->ps);
        }
        yy_delete_buffer(bs, scanner);
        err = 0;
    } while (0);

    return err;
}

int parser_num(const char *s, struct scpi_type *val, int token)
{
    /* FIXME: needs error checking. */
    val->type = SCPI_TYPE_INT;
    val->val.ival = strtol(s, NULL, 10);
    val->src = s;
    return token;
}
