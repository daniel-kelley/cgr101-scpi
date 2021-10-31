/*
   scpi_core.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include "scpi.h"
#include "parser.h"

void yyerror(void *loc, struct info *info, const char *s)
{
    (void)loc;
    (void)info;
    (void)s;
}

void scpi_common_cls(struct info *info)
{
    (void)info;
}

void scpi_system_internal_quit(struct info *info)
{
    info->quit = 1;
}

int scpi_core_send(struct info *info)
{
    return parser_send(info, info->cli_buf, sizeof(info->cli_buf));
}

int scpi_core_init(struct info *info)
{
    return parser_init(info);
}

int scpi_core_done(struct info *info)
{
    return parser_done(info);
}
