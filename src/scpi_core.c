/*
   scpi_core.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <stdint.h>
#include <stdlib.h>
#include "scpi.h"
#include "scpi_core.h"
#include "parser.h"

void yyerror(void *loc, struct info *info, const char *s)
{
    (void)loc;
    (void)info;
    (void)s;
}

void scpi_common_cls(struct info *info)
{
    struct scpi_core *scpi = info->scpi;

    scpi->ques = 0;
    scpi->oper = 0;
    scpi->event = 0;
    scpi->status = 0;
    scpi->error.head = 0;
    scpi->error.tail = 0;
    scpi->output.len = 0;
}

void scpi_common_ese(struct scpi_type *val, struct info *info)
{
    if (val->type == SCPI_TYPE_INT &&
        val->val.ival >= 0 &&
        val->val.ival <= 255)
    {
        info->scpi->event = (uint8_t)val->val.ival;
    } else {
        /*scpi_error(info->scpi, SCPI_ERR_DATA_OUT_OF_RANGE, val->src);*/
    }
}

void scpi_common_eseq(struct info *info)
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
    int err;

    do {
        info->scpi = calloc(1,sizeof(*info->scpi));
        if (!info->scpi) {
            err = -1;
            break;
        }
        err = parser_init(info);
    } while (0);

    return err;
}

int scpi_core_done(struct info *info)
{
    int err;

    do {
        if (info->scpi) {
            free(info->scpi);
        }
        err = parser_done(info);
    } while (0);

    return err;
}
