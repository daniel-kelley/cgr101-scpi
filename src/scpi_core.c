/*
   scpi_core.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "scpi.h"
#include "scpi_core.h"
#include "scpi_error.h"
#include "parser.h"

static uint8_t scpi_core_status_update(struct info *info)
{
    return info->scpi->sbr;
}

void scpi_common_cls(struct info *info)
{
    struct scpi_core *scpi = info->scpi;

    scpi->ques = 0;
    scpi->oper = 0;
    scpi->seser = 0;
    scpi->sesr = 0;
    scpi->error.head = 0;
    scpi->error.tail = 0;
    scpi->output.len = 0;
}

void scpi_common_ese(struct info *info, struct scpi_type *val)
{
    if (val->type == SCPI_TYPE_INT &&
        val->val.ival >= 0 &&
        val->val.ival <= 255)
    {
        info->scpi->seser = (uint8_t)val->val.ival;
    } else {
        /*scpi_error(info->scpi, SCPI_ERR_DATA_OUT_OF_RANGE, val->src);*/
    }
}

void scpi_common_eseq(struct info *info)
{
    scpi_output_int(&info->scpi->output, info->scpi->seser);
}

void scpi_common_esrq(struct info *info)
{
    scpi_output_int(&info->scpi->output, info->scpi->sesr);
    /* 488.2: Reading sesr clears it. */
    info->scpi->sesr = 0;
}

void scpi_common_idnq(struct info *info)
{
    scpi_output_str(&info->scpi->output, "GMP,CGR101-SCPI,1.0,01-02");
}

void scpi_common_opc(struct info *info)
{
     info->scpi->sesr |= SCPI_SESR_OPC;
}

void scpi_common_opcq(struct info *info)
{
    scpi_output_int(&info->scpi->output,
                    info->scpi->sesr & SCPI_SESR_OPC ? 1 : 0);
}

void scpi_common_rst(struct info *info)
{
    scpi_common_opc(info);
}

void scpi_common_sre(struct info *info, struct scpi_type *val)
{
    if (val->type == SCPI_TYPE_INT &&
        val->val.ival >= 0 &&
        val->val.ival <= 255)
    {
        info->scpi->srer = (uint8_t)val->val.ival;
    } else {
        /*scpi_error(info->scpi, SCPI_ERR_DATA_OUT_OF_RANGE, val->src);*/
    }
}

void scpi_common_sreq(struct info *info)
{
    scpi_output_int(&info->scpi->output, info->scpi->srer);
}

void scpi_common_stbq(struct info *info)
{
    uint8_t value;
    value = scpi_core_status_update(info);
    scpi_output_int(&info->scpi->output, value);
}

void scpi_common_tstq(struct info *info)
{
    scpi_output_int(&info->scpi->output, 0);
}

void scpi_common_wai(struct info *info)
{
    scpi_common_opc(info);
}

void scpi_system_versionq(struct info *info)
{
    scpi_output_str(&info->scpi->output, "1999.0");
}

void scpi_system_capabilityq(struct info *info)
{
    scpi_output_str(&info->scpi->output, "\"DIGITIZER\"");
}

void scpi_system_error_countq(struct info *info)
{
    int n = scpi_error_count(&info->scpi->error);
    scpi_output_int(&info->scpi->output, n);
}

void scpi_system_error_nextq(struct info *info)
{
    enum scpi_err_num error;
    const char *msg;
    const char *syndrome;

    scpi_error_get(&info->scpi->error, &error, &msg, &syndrome);

    if (syndrome) {
        scpi_output_printf(&info->scpi->output,
                           "%d,\"%s;%s\"",
                           error, msg, syndrome);
        free((void *)syndrome);
    } else {
        scpi_output_printf(&info->scpi->output,"%d,\"%s\"", error, msg);
    }
}

void scpi_system_internal_setupq(struct info *info)
{
    scpi_output_int(&info->scpi->output, 0);
}

void scpi_system_internal_quit(struct info *info)
{
    info->quit = 1;
}

static int scpi_core_parser_error(struct info *info,
                                  int err_in,
                                  const char *msg)
{
    int err = 1;
    char *syndrome;

    if (err_in) {
        scpi_error(&info->scpi->error,
                   SCPI_ERR_INTERNAL_PARSER_ERROR,
                   NULL);
    } else {
        char *p;

        syndrome = strdup(info->cli_buf);
        p = strchr(syndrome, '\n');
        if (p) {
            *p = 0;
        }
        assert(syndrome);

        if (msg) {
            /* syndrome appended with ;<msg> */
            size_t slen = strlen(syndrome);
            size_t mlen = strlen(msg);
            syndrome = realloc(syndrome, mlen+slen+2);
            assert(syndrome);
            syndrome[slen] = ';';
            memcpy(syndrome+slen+1,msg,mlen+1);
        }

        scpi_error(&info->scpi->error,
                   SCPI_ERR_UNDEFINED_HEADER,
                   syndrome);

        free(syndrome); /* scpi_error() makes own copy */
    }

    err = 0;

    return err;
}

int scpi_core_send(struct info *info, char *buf, int len)
{
    int err;

    do {

        memset(&info->rsp, 0, sizeof(info->rsp));
        scpi_output_clear(&info->scpi->output);

        err = parser_send(info, buf, len);
        if (info->busy) {
            break;
        }

        if (err) {
            const char *errmsg;
            int trace = 0;
            err = parser_error_get(info, &errmsg, &trace);
            err = scpi_core_parser_error(info, err, trace ? errmsg : NULL);
        } else {
            err = scpi_output_get(
                &info->scpi->output, &info->rsp.buf, &info->rsp.len);

            if (err) {
                /* handle error */
                assert(0);
                break;
            }
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
        scpi_error_done(&info->scpi->error);
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
