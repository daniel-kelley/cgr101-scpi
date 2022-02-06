/*
   scpi_core.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include "scpi.h"
#include "scpi_core.h"
#include "scpi_error.h"
#include "scpi_input.h"
#include "parser.h"
#include "event.h"

/* Private for signal handling. */
static int scpi_core_block_until_active;

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
    if (info->scpi->ques.event && info->scpi->ques.enable) {
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
    if (info->scpi->oper.event & info->scpi->oper.enable) {
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

    scpi->ques.event = 0;
    scpi->ques.enable = 0;
    scpi->oper.enable = 0;
    scpi->oper.event = 0;
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

void scpi_common_opc(struct info *info)
{
    /* FIXME: schedules an event to set SCPI_SESR_OPC when all
     * operations are complete. */
    info->scpi->sesr |= SCPI_SESR_OPC;
}

void scpi_common_opcq(struct info *info)
{
    if (info->overlapped) {
        info->block_input = 1;
        info->scpi->opcq = 1;
    } else {
        scpi_output_int(info->output, 1);
    }
}

void scpi_common_rst(struct info *info)
{
    scpi_dev_rst(info);
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
    if (info->overlapped) {
        info->block_input = 1;
    }
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
    uint16_t cond = 0;

    if (info->sweep_status) {
        cond |= SCPI_OPER_SWE;
    }

    if (info->digital_event_status) {
        cond |= SCPI_OPER_DE;
    }

    info->scpi->oper.cond = cond;

    scpi_output_int(info->output, info->scpi->oper.cond);
}

void scpi_status_operation_enable(struct info *info, struct scpi_type *val)
{
    scpi_input_uint16(info, val, &info->scpi->oper.enable);
}

void scpi_status_operation_enableq(struct info *info)
{
    scpi_output_int(info->output, info->scpi->oper.enable);
}

void scpi_status_operation_preset(struct info *info)
{
    (void)info;
}

void scpi_status_questionableq(struct info *info)
{
    scpi_output_int(info->output, info->scpi->ques.cond);
}

void scpi_status_questionable_enable(struct info *info,
                                     struct scpi_type *val)
{
    scpi_input_uint16(info, val, &info->scpi->ques.enable);
}

void scpi_status_questionable_enableq(struct info *info)
{
    scpi_output_int(info->output, info->scpi->ques.enable);
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
                                  const char *buf,
                                  int err_in,
                                  const struct parser_msg_loc *data)
{
    int err = 1;
    char *syndrome;

    if (err_in) {
        scpi_error(info->error,
                   SCPI_ERR_INTERNAL_PARSER_ERROR,
                   NULL);
    } else {
        char *p;

        syndrome = strdup(buf);
        p = strchr(syndrome, '\n');
        if (p) {
            *p = 0;
        }
        assert(syndrome);

        if (data) {
            /* syndrome appended with ;<msg> */
            size_t slen = strlen(syndrome);
            size_t mlen = strlen(data->msg);
            syndrome = realloc(syndrome, mlen+slen+2);
            assert(syndrome);
            syndrome[slen] = ';';
            memcpy(syndrome+slen+1,data->msg,mlen+1);
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
            struct parser_msg_loc data;
            int trace = 0;
            err = parser_error_get(info, &data, &trace);
            assert(!err);
            err = scpi_core_parser_error(info, buf, err, trace ? &data : NULL);
        }

    } while (0);

    return err;
}

static void scpi_core_unblock_input(void *arg)
{
    struct info *info = arg;

    info->block_input = 0;
    if (info->scpi->opcq) {
        info->scpi->opcq = 0;
        scpi_output_int(info->output, 1);
    }
}

static int scpi_core_io_done(struct info *info)
{
    int err = 0;

    if (info->cli_in_fd != STDIN_FILENO) {
        close(info->cli_in_fd);
    }
    if (info->cli_out_fd != STDOUT_FILENO) {
        close(info->cli_out_fd);
    }

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

        scpi_core_io_done(info);

    } while (0);

    return err;
}

void scpi_core_top(struct info *info)
{
    if (info->scpi->pool) {
        free(info->scpi->pool);
        info->scpi->pool = NULL;
    }
    parser_cleanup();
    info->busy = 0;
}

/*   prev current output
 *      0       0 no  - this is a ':' not preceded by ';'
 *      0       1 yes - this is a ';'
 *      1       0 no  - this is a ':' resetting the path from previous ';'
 *      1       1 yes - this is another ';'
 */
void scpi_core_cmd_sep(struct info *info, int value)
{
    parser_separator(info, value);
    if (value) {
        scpi_output_cmd_sep(info->output);
    }
}

struct scpi_type *scpi_core_format_type(struct info *info,
                                        struct scpi_type *v1,
                                        struct scpi_type *v2)
{
    (void)info;
    (void)v2;

    return v1;
}

#define NRF_CHUNK 256

struct scpi_type *scpi_core_nrf_list(struct info *info,
                                     struct scpi_type *vl,
                                     struct scpi_type *vs)
{
    union scpi_type_val val;
    char *buf;
    size_t hsize = sizeof(struct scpi_type_list);

    assert(NRF_CHUNK > 1); /* Check minimum chunk size for allocations. */
    /* Lists will be converted to arrays of the underlying scalar type
     * in place, which will only work if the header is at least the
     * size of the largest scalar type.
     */
    assert(hsize >= sizeof(val));
    if (vl->type != SCPI_TYPE_LIST) {
        /* Start list */
        assert(vl->type == vs->type);
        val = vl->val;
        vl->type = SCPI_TYPE_LIST;
        buf = malloc(hsize + sizeof(val)*NRF_CHUNK);
        assert(buf);
        assert(!info->scpi->pool);
        info->scpi->pool = buf;
        vl->val.list = (struct scpi_type_list *)buf;
        buf += sizeof(struct scpi_type_list);
        vl->val.list->type = vs->type;
        vl->val.list->val = (union scpi_type_val *)buf;
        vl->val.list->len = 1;
        vl->val.list->max = NRF_CHUNK;
        vl->val.list->val[0] = val;
    } else {
        /* Lists always contain a uniform type */
        assert(vl->val.list->type == vs->type);
        if (vl->val.list->len == vl->val.list->max) {
            /* Expand list */
            vl->val.list->max += NRF_CHUNK;
            buf = realloc(vl->val.list, hsize + sizeof(val)*vl->val.list->max);
            assert(buf);
            vl->val.list = (struct scpi_type_list *)buf;
        }
    }
    /* Append vs to list vl. */
    assert(vl->val.list->len < vl->val.list->max);
    vl->val.list->val[vl->val.list->len] = vs->val;
    vl->val.list->len++;

    return vl;
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

void scpi_core_add_prefix(struct info *info, int token)
{
    parser_add_prefix(info, token);
}

void scpi_system_internal_sleep(struct info *info, struct scpi_type *v)
{
    double value;
    double sec;
    double usec;
    struct itimerval itimer;
    int err;

    scpi_input_fp(info, v, &value);

    sec = floor(value);
    usec = floor((value - sec)*1000000.0);
    assert(sec >= 0.0);
    assert(usec >= 0.0);

    memset(&itimer, 0, sizeof(itimer));
    itimer.it_value.tv_sec = (time_t)sec;
    itimer.it_value.tv_usec = (suseconds_t)usec;

    /* Mark block_until active */
    assert(!scpi_core_block_until_active);
    scpi_core_block_until_active = 1;

    /* Set timer. */
    err = setitimer(ITIMER_REAL, &itimer, NULL);
    assert(!err);
}

void scpi_system_internal_echo(struct info *info, struct scpi_type *v)
{
    scpi_output_str(info->output, v->src);
}

void scpi_system_internal_include(struct info *info, struct scpi_type *v)
{
    parser_include(info, v->src);
}

static void scpi_core_output_flush(void *arg)
{
    struct info *info = arg;

    scpi_output_flush(info->output, info->cli_out_fd);
}



static int scpi_core_scpi_send(struct info *info, char *buf, size_t len)
{
    int rc = 0;

    rc = scpi_core_send(info, buf, (int)len);
    event_send(info->event, EVENT_OUTPUT_FLUSH);

    return rc;
}

static int scpi_core_cli_line(struct info *info)
{
    char *eol;
    size_t llen;
    int rc = 0;

    assert(info->cli_line);
    assert(info->cli_line_len > 0);
    eol = strchr(info->cli_line, '\n');
    if (eol) {
        llen = (size_t)(eol - info->cli_line);
        assert(llen < INFO_CLI_LEN);
        if (llen) {
            rc = scpi_core_scpi_send(info, info->cli_line, llen);
        }
        info->cli_line = eol + 1;
        info->cli_line_len -= (llen + 1);
        if (info->cli_line_len == 0) {
            info->cli_line = NULL;
        }
    } else if (info->cli_line[0] == 0) {
        /* EOF */
        rc = scpi_core_scpi_send(info, info->cli_line, info->cli_line_len);
    } else {
        /* Do something with the leftovers */
        assert(info->cli_line_len < INFO_CLI_LEN);
        memmove(info->cli_buf, info->cli_line, info->cli_line_len);
        info->cli_offset = info->cli_line_len;
        info->cli_line = NULL;
        info->cli_line_len = 0;
    }

    return rc;
}

void scpi_core_line(struct info *info)
{
    if (info->cli_line && !scpi_core_block_input(info)) {
        scpi_core_cli_line(info);
    }
}

int scpi_core_block_input(struct info *info)
{
    return scpi_core_block_until_active || info->block_input;
}


static int scpi_core_cli_read(struct info *info)
{
    int rc;
    char *cli_buf;
    size_t cli_len;

    /* Append to leftovers from the previous pass. */
    cli_buf = info->cli_buf;
    cli_len = sizeof(info->cli_buf);
    cli_buf += info->cli_offset;
    cli_len -= info->cli_offset;

    memset(cli_buf, 0, cli_len);

    do {
        rc = (int)read(info->cli_in_fd, cli_buf, cli_len);
    } while (rc < 0 && errno == EAGAIN);

    if (rc < 0) {
        if (errno != EWOULDBLOCK) {
            perror("read() failed");
        }
    } else if (rc == 0) {
        if (info->verbose) {
            printf("Done.\r\n");
        }
        /* EOF */
        assert(cli_buf[0] == 0);
        assert(cli_buf[1] == 0);
        info->cli_line = cli_buf;
        info->cli_line_len = 1;
    } else {
        info->cli_line = cli_buf;
        info->cli_line_len = (size_t)rc;
        rc = 0;
    }

    return rc;
}

static int scpi_core_cli_read_worker(void *arg)
{
    struct info *info = arg;
    int err = 0;

    if (!info->cli_line &&
        !scpi_core_block_input(info)) {
        err = scpi_core_cli_read(info);
    }

    return err;
}

static void scpi_core_io_init(struct info *info)
{
    int err = 0;
    int on = 1;

    /* default in/out from console */
    info->cli_in_fd = STDIN_FILENO;
    info->cli_out_fd = STDOUT_FILENO;

    err = ioctl(info->cli_in_fd, FIONBIO, (char *) &on);
    assert(!err);

    err = ioctl(info->cli_out_fd, FIONBIO, (char *) &on);
    assert(!err);
}

static void scpi_core_sigalrm(int sig)
{
    (void)sig;

    scpi_core_block_until_active = 0;
}

static void scpi_core_alarm_init(void)
{
    struct sigaction sa;
    int err;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = scpi_core_sigalrm;
    sa.sa_flags = SA_RESTART;
    err = sigaction(SIGALRM, &sa, NULL);
    assert(!err);
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

        scpi_core_io_init(info);
        scpi_core_alarm_init();

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
        if (err) {
            break;
        }

        err = event_add(info->event,
                        EVENT_UNBLOCK,
                        scpi_core_unblock_input,
                        info);

        if (err) {
            break;
        }

        err = worker_add(info->worker,
                        info->cli_in_fd,
                        scpi_core_cli_read_worker,
                        info);

        if (err) {
            break;
        }

        err = event_add(info->event,
                        EVENT_OUTPUT_FLUSH,
                        scpi_core_output_flush,
                        info);
        assert(!err);

    } while (0);

    return err;
}
