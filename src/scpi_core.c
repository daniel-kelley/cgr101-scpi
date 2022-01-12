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
#include "cgr101.h"

static uint8_t scpi_core_status_update(struct info *info)
{
    uint8_t sbr = 0;
    uint8_t stbq = 0;

    /* SCPI Vol.1 Section 9.2 */
    /* SBR.0: Available to instrument */
    /* SBR.1: Available to instrument */

    /* SBR.2: SCPI error queue not empty */
    if (scpi_error_count(info->error)) {
        sbr |= SCPI_SBR_ERQ;
    }

    /* SBR.3: SCPI QUEStionable status not zero */
    if (info->scpi->ques_event && info->scpi->ques_enable) {
        sbr |= SCPI_SBR_QUES;
    }

    /* SBR.4: 488.2 MAV */
    /* Currently unused. */

    /* SBR.5: 488.2 SESR not zero */
    if (info->scpi->sesr & info->scpi->seser ) {
        sbr |= SCPI_SBR_SESR;
    }

    /* SBR.6: 488.2 RQS not zero */
    /* Currently unused. */

    /* SBR.7: SCPI OPERation status not zero */
    if (info->scpi->oper_event & info->scpi->oper_enable) {
        sbr |= SCPI_SBR_OPER;
    }

    /*
     * Message Summary status is reflected in the value returned by *stb?
     * but not in the actual stb register itself, so the stb register is
     * updated *first*, then the stb MSS bit cleared, MSS calculated and
     * stuffed back in the stbq value.
     */
    info->scpi->sbr = stbq = sbr;

    stbq &= (uint8_t)~SCPI_SBR_MSS;
    if (stbq & info->scpi->srer) {
        stbq |= SCPI_SBR_MSS;
    }

    return stbq;
}

void scpi_common_cls(struct info *info)
{
    struct scpi_core *scpi = info->scpi;

    scpi->ques_event = 0;
    scpi->ques_enable = 0;
    scpi->oper_enable = 0;
    scpi->oper_event = 0;
    scpi->seser = 0;
    scpi->sesr = 0;
    scpi_error_reset(info->error);
    scpi_output_reset(info->output);
}

void scpi_common_ese(struct info *info, struct scpi_type *val)
{
    scpi_input_uint8(info, val, &info->scpi->seser);
}

void scpi_common_eseq(struct info *info)
{
    scpi_output_int(info->output, info->scpi->seser);
}

void scpi_common_esrq(struct info *info)
{
    scpi_output_int(info->output, info->scpi->sesr);
    /* 488.2: Reading sesr clears it. */
    info->scpi->sesr = 0;
}

void scpi_common_idnq(struct info *info)
{
    cgr101_identify(info);
}

void scpi_common_opc(struct info *info)
{
     info->scpi->sesr |= SCPI_SESR_OPC;
}

void scpi_common_opcq(struct info *info)
{
    scpi_output_int(info->output,
                    info->scpi->sesr & SCPI_SESR_OPC ? 1 : 0);
}

void scpi_common_rst(struct info *info)
{
    scpi_common_opc(info);
}

void scpi_common_sre(struct info *info, struct scpi_type *val)
{
    scpi_input_uint8(info, val, &info->scpi->srer);
}

void scpi_common_sreq(struct info *info)
{
    scpi_output_int(info->output, info->scpi->srer);
}

void scpi_common_stbq(struct info *info)
{
    uint8_t value;
    value = scpi_core_status_update(info);
    scpi_output_int(info->output, value);
}

void scpi_common_tstq(struct info *info)
{
    scpi_output_int(info->output, 0);
}

void scpi_common_wai(struct info *info)
{
    scpi_common_opc(info);
}

void scpi_system_versionq(struct info *info)
{
    scpi_output_str(info->output, "1999.0");
}

void scpi_system_capabilityq(struct info *info)
{
    scpi_output_str(info->output, "\"DIGITIZER\"");
}

void scpi_system_error_countq(struct info *info)
{
    int n = scpi_error_count(info->error);
    scpi_output_int(info->output, n);
}

void scpi_system_error_nextq(struct info *info)
{
    enum scpi_err_num error;
    const char *msg;
    const char *syndrome;

    scpi_error_get(info->error, &error, &msg, &syndrome);

    if (syndrome) {
        scpi_output_printf(info->output,
                           "%d,\"%s;%s\"",
                           error, msg, syndrome);
        free((void *)syndrome);
    } else {
        scpi_output_printf(info->output,"%d,\"%s\"", error, msg);
    }
}

void scpi_status_operation_eventq(struct info *info)
{
    scpi_output_int(info->output, 0);
}

void scpi_status_operation_conditionq(struct info *info)
{
    scpi_output_int(info->output, info->scpi->oper_event);
}

void scpi_status_operation_enable(struct info *info, struct scpi_type *val)
{
    scpi_input_uint16(info, val, &info->scpi->oper_enable);
}

void scpi_status_operation_enableq(struct info *info)
{
    scpi_output_int(info->output, info->scpi->oper_enable);
}

void scpi_status_operation_preset(struct info *info)
{
    (void)info;
}

void scpi_status_questionableq(struct info *info)
{
    scpi_output_int(info->output, info->scpi->ques_event);
}

void scpi_status_questionable_enable(struct info *info,
                                     struct scpi_type *val)
{
    scpi_input_uint16(info, val, &info->scpi->ques_enable);
}

void scpi_status_questionable_enableq(struct info *info)
{
    scpi_output_int(info->output, info->scpi->ques_enable);
}

void scpi_system_internal_setupq(struct info *info)
{
    scpi_output_int(info->output, 0);
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
        scpi_error(info->error,
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

        scpi_error(info->error,
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
        scpi_output_clear(info->output);

        err = parser_send(info, buf, len);
        if (info->busy) {
            break;
        }

        if (err) {
            const char *errmsg;
            int trace = 0;
            err = parser_error_get(info, &errmsg, &trace);
            err = scpi_core_parser_error(info, err, trace ? errmsg : NULL);
        }

    } while (0);

    return err;
}

int scpi_core_recv_ready(struct info *info)
{
    return scpi_output_ready(info->output);
}

/* call only when ready */
int scpi_core_recv(struct info *info)
{
    int err = 0;

    err = scpi_output_get(
        info->output, &info->rsp.buf, &info->rsp.len);

    assert(!err);

    info->rsp.valid = (info->rsp.len != 0);

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

        info->output = scpi_output_init();
        if (!info->output) {
            err = -1;
            break;
        }

        info->error = scpi_error_init();
        if (!info->error) {
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
        scpi_error_done(info->error);
        if (info->scpi) {
            free(info->scpi);
        }

        if (info->output) {
            scpi_output_done(info->output);
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
    scpi_output_cmd_sep(info->output);
}

struct scpi_type *scpi_core_format_type(struct info *info,
                                        struct scpi_type *v1,
                                        struct scpi_type *v2)
{
    (void)info;
    (void)v2;

    return v1;
}

struct scpi_type *scpi_core_nrf_list(struct info *info,
                                     struct scpi_type *v)
{
    (void)info;

    return v;
}

struct scpi_type *scpi_core_symbolic_value(struct info *info,
                                           struct scpi_type *v)
{
    (void)info;

    return v;
}

void scpi_core_format(struct info *info, struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_core_formatq(struct info *info)
{
    (void)info;
}

int scpi_core_initiate(struct info *info)
{
    return cgr101_initiate(info);
}

void scpi_core_initiate_immediate(struct info *info)
{
    cgr101_initiate_immediate(info);
}

void scpi_system_communicate_tcp_controlq(struct info *info)
{
    (void)info;
}

void scpi_system_internal_calibrate(struct info *info)
{
    (void)info;
}

void scpi_system_internal_configure(struct info *info)
{
    (void)info;
}

void scpi_system_internal_showq(struct info *info)
{
    (void)info;
}

