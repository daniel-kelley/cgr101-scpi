/*
   parser.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <stddef.h>
#include <string.h>
#include "parser.h"
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

        info->parser->ps = yypstate_new();
        if (!info->parser->ps) {
            break;
        }
        rv = 0;

    } while (0);

    return rv;
}

int parser_fini(struct info *info)
{
    if (info->lexer) {
        yylex_destroy(&info->lexer->scanner);
        free(info->lexer);
    }

    if (info->parser) {
        yypstate_delete(info->parser->ps);
        free(info->parser);
    }

    return 0;
}

/* line modified in place, needs *two* NULs at end; len reflects this */
int parser_read(struct info *info, char *line, size_t len)
{
    YY_BUFFER_STATE bs;
    int status;
    int c;
    YYSTYPE yys;
    YYLTYPE yyl;
    yyscan_t *scanner = &info->lexer->scanner;

    bs = yy_scan_buffer(line, len, scanner);

    do {
        c = yylex(&yys, &yyl, scanner);
        status = yypush_parse(info->parser->ps, c, &yys, &yyl);
    } while (status == YYPUSH_MORE);

    yy_delete_buffer(bs, scanner);

    return 0;
}
