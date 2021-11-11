/*
   scpi_core.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "scpi.h"
#include "scpi_core.h"
#include "parser.h"

void yyerror(void *loc, struct info *info, const char *s)
{
    (void)loc;
    if (info->verbose) {
        fprintf(stderr,">>> %s:\n", s);
    }
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
    scpi_output_int(&info->scpi->output, info->scpi->event);
}

void scpi_system_versionq(struct info *info)
{
    scpi_output_str(&info->scpi->output, "1999.0");
}

void scpi_system_error_countq(struct info *info)
{
    scpi_output_int(&info->scpi->output, 0);
}

void scpi_system_internal_quit(struct info *info)
{
    info->quit = 1;
}

int scpi_core_send(struct info *info, char *buf, int len)
{
    int err;

    do {

        info->rsp.valid = 0;
        scpi_output_clear(&info->scpi->output);

        err = parser_send(info, buf, len);
        if (err || info->busy) {
            break;
        }

        err = scpi_output_get(
            &info->scpi->output, &info->rsp.buf, &info->rsp.len);

        if (err) {
            /* handle error */
            break;
        }

        info->rsp.valid = (info->rsp.len != 0);

    } while (0);

    return err;
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

void scpi_core_top(struct info *info)
{
    info->busy = 0;
}

void scpi_core_cmd_sep(struct info *info)
{
    scpi_output_cmd_sep(&info->scpi->output);
}
