/*
   cgr101.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "worker.h"
#include "spawn.h"
#include "scpi_output.h"
#include "cgr101.h"

#define CMD "sp -b230400 -f/dev/ttyUSB0"
#define ID_MAX 32
#define RCV_MAX 4100 /* 4097 plus some slop */
#define ERR_MAX 1024

enum cgr101_rcv_state {
    IDLE,
    IDENTIFY,
};

struct cgr101 {
    struct spawn child;
    char device_id[ID_MAX];
    enum cgr101_rcv_state rcv_state;
    size_t cur_rcv_len;
    size_t cur_rcv_parsed;
    size_t exp_rcv_len;
    char rcv_data[RCV_MAX];
    char err_data[ERR_MAX];
};

/*
 * Receive State Machine
 */

static int cgr101_rcv_idle(struct info *info)
{
    int err = 0;

    info->device->rcv_state = IDLE;
    info->device->cur_rcv_len = 0;
    info->device->cur_rcv_parsed = 0;
    info->device->exp_rcv_len = 0;

    return err;
}

static int cgr101_rcv_start(struct info *info)
{
    int err = 1;

    assert(info->device->cur_rcv_len > 0);

    switch (info->device->rcv_data[0]) {
    case '*':
        info->device->rcv_state = IDENTIFY;
        /* Variable length: terminated by <cr><lf> */
        info->device->exp_rcv_len = sizeof(info->device->rcv_data);
        info->device->cur_rcv_parsed++;
        err = 0;
        break;
    default:
        assert(0);
    }

    return err;
}

/*
 * Device Identify Handling
 */

static int cgr101_identify_start(struct info *info)
{
    ssize_t len;

    len = write(info->device->child.stdin, "i\n", 2UL);
    assert(len > 0);

    return (len <= 0);
}

static int cgr101_identify_done(struct info *info)
{
    scpi_output_printf(info->output,
                       "GMP,CGR101-SCPI,1.0,%s",
                       info->device->device_id);

    return 0;
}

static int cgr101_rcv_ident(struct info *info)
{
    int err = 0;

    info->device->cur_rcv_parsed = info->device->cur_rcv_len;

    if (info->device->rcv_data[info->device->cur_rcv_len-1] == '\n') {
        /* Entire ident string received. */
        memcpy(
            info->device->device_id,
            info->device->rcv_data + 1,
            info->device->cur_rcv_len - 3);

        err = cgr101_identify_done(info);

        /* Done receiving. */
        cgr101_rcv_idle(info);

    }

    return err;
}

int cgr101_identify(struct info *info)
{
    if (info->device->device_id[0]) {
        cgr101_identify_done(info);
    } else {
        cgr101_identify_start(info);
    }

    return 0;
}

/*
 * Recieve Output Channel Handling
 */

static int cgr101_rcv_sm(struct info *info)
{
    int err = 1;

    switch (info->device->rcv_state) {
    case IDLE:
        err = cgr101_rcv_start(info);
        break;
    case IDENTIFY:
        err = cgr101_rcv_ident(info);
        break;
    default:
        assert(0);
    }

    return err;
}

static int cgr101_rcv_data(struct info *info)
{
    int err = 1;
    int count = 0;
    while (info->device->cur_rcv_parsed < info->device->cur_rcv_len) {
        err = cgr101_rcv_sm(info);
        if (err) {
            break;
        }
        /* Backstop to prevent infinite loops. */
        if (count++ > RCV_MAX) {
            break;
        }
    }

    return err;
}

static int cgr101_out(void *arg)
{
    struct info *info = arg;
    int err = 0;
    ssize_t len;

    len = read(
        info->device->child.stdout,
        info->device->rcv_data + info->device->cur_rcv_len,
        RCV_MAX - info->device->cur_rcv_len);

    if (len < 0) {
        if (errno != EINTR && errno != EAGAIN) {
            err = 1;
        }
    } else if (len != 0) {
        info->device->cur_rcv_len += (size_t)len;
        err = cgr101_rcv_data(info);
    }

    return err;
}

/*
 * Recieve Error Channel Handling
 */

static int cgr101_err(void *arg)
{
    struct info *info = arg;
    int err = 0;
    ssize_t len;

    len = read(
        info->device->child.stderr,
        info->device->err_data,
        sizeof(info->device->err_data));

    assert(len > 0);
    assert(len <= (ssize_t)sizeof(info->device->err_data));
    (void)info;

    return err;
}

/*
 * Device Open/Close
 */

int cgr101_open(struct info *info)
{
    int err;
    struct cgr101 *cgr101;

    cgr101 = calloc(1,sizeof(*cgr101));
    assert(cgr101);
    if (info->debug && strchr(info->debug,'E')) {
        info->emulation = 1;
    }

    info->device = cgr101;

    do {
        err = spawn(CMD, &info->device->child);
        if (err) {
            break;
        }

        err = worker_add(
            info->worker,
            info->device->child.stdout,
            cgr101_out,
            info);
        if (err) {
            break;
        }

        err = worker_add(
            info->worker,
            info->device->child.stderr,
            cgr101_err,
            info);
        if (err) {
            break;
        }

    } while (0);

    return err;
}

int cgr101_close(struct info *info)
{
    unspawn(&info->device->child);
    if (info->device) {
        free(info->device);
    }

    return 0;
}
