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

#define ID_MAX 32
#define RCV_MAX 4100 /* 4097 plus some slop */
#define ERR_MAX 1024

static char *CMD[] = {
    "sp",
    "-b230400",
    "-f/dev/ttyUSB0",
    NULL
};

enum cgr101_rcv_state {
    IDLE,
    IDENTIFY,
    DIGITAL_DATA,
};

struct cgr101 {
    struct spawn child;
    int device_id_len;
    char device_id[ID_MAX];
    enum cgr101_rcv_state rcv_state;
    char rcv_data[RCV_MAX];
    char err_data[ERR_MAX];
    int digital_data_requested;
    int digital_data_fetch;
    int digital_data_valid;
    int digital_data;
};

/*
 * Sender
 */
static int cgr101_send(struct info *info, const char *str)
{
    size_t len_in = strlen(str);
    ssize_t len_out;
    int result;

    len_out = write(info->device->child.stdin, str, len_in);
    result = (len_out > 0) ? ((size_t)len_out == len_in) : 0;
    assert(result);

    return (result != 0);
}


/*
 * Receive State Machine
 */

static void cgr101_rcv_idle(struct info *info)
{
    info->device->rcv_state = IDLE;
}

static int cgr101_rcv_start(struct info *info, char c)
{
    int err = 1;

    switch (c) {
    case '*':
        info->device->rcv_state = IDENTIFY;
        /* Variable length: terminated by <cr><lf> */
        err = 0;
        break;
    case 'I':
        info->device->rcv_state = DIGITAL_DATA;
        /* I<uint8_t> */
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
    return cgr101_send(info, "i\n");
}

static void cgr101_identify_done(struct info *info)
{
    scpi_output_printf(info->output,
                       "GMP,CGR101-SCPI,1.0,%s",
                       info->device->device_id);
}

static int cgr101_rcv_ident(struct info *info, char c)
{
    int err = 0;

    if (c == '\n') {
        /* Done receiving. */
        cgr101_identify_done(info);
        cgr101_rcv_idle(info);
    } else if (c != '\r') {
        info->device->device_id[info->device->device_id_len++] = c;
    }

    return err;
}

int cgr101_identify(struct info *info)
{
    if (info->device->device_id_len > 0) {
        cgr101_identify_done(info);
    } else {
        cgr101_identify_start(info);
    }

    return 0;
}

/*
 * Device Digital Data Read Handling
 */
static void cgr101_output_digital_data(struct info *info)
{
    assert(info->device->digital_data_valid);
    scpi_output_int(info->output, info->device->digital_data);
}

static int cgr101_digital_data_start(struct info *info)
{
    return cgr101_send(info, "D I\n");
}

static int cgr101_rcv_digital_data(struct info *info, char c)
{
    int err = 0;

    /* Done receiving. */
    info->device->digital_data = c;
    info->device->digital_data_valid = 1;
    if (info->device->digital_data_fetch) {
        cgr101_output_digital_data(info);
        info->device->digital_data_fetch = 0;
    }
    cgr101_rcv_idle(info);

    return err;
}

/*
 * Recieve Output Channel Handling
 */

static int cgr101_rcv_sm(struct info *info, char c)
{
    int err = 1;

    switch (info->device->rcv_state) {
    case IDLE:
        err = cgr101_rcv_start(info, c);
        break;
    case IDENTIFY:
        err = cgr101_rcv_ident(info, c);
        break;
    case DIGITAL_DATA:
        err = cgr101_rcv_digital_data(info, c);
        break;
    default:
        assert(0);
    }

    return err;
}

static int cgr101_rcv_data(struct info *info, const char *buf, size_t len)
{
    int err = 1;

    while (len--) {
        err = cgr101_rcv_sm(info, *buf++);
        if (err) {
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

    len = read(info->device->child.stdout, info->device->rcv_data, RCV_MAX);

    if (len < 0) {
        if (errno != EINTR && errno != EAGAIN) {
            err = 1;
        }
    } else if (len != 0) {
        err = cgr101_rcv_data(info, info->device->rcv_data, (size_t)len);
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

/*
 * SCPI Interfaces
 */

int cgr101_initiate(struct info *info)
{
    if (info->device->digital_data_requested) {
        cgr101_digital_data_start(info);
    }

    return 0;
}

int cgr101_configure_digital_data(struct info *info)
{
    info->device->digital_data_requested = 1;

    return 0;
}

int cgr101_digital_data_configured(struct info *info)
{
    return info->device->digital_data_requested;
}

void cgr101_fetch_digital_data(struct info *info)
{
    assert(info->device->digital_data_requested);
    if (info->device->digital_data_valid) {
        cgr101_output_digital_data(info);
    } else {
        /* RACE CONDITION - data could have been received after valid
         is checked. */
        info->device->digital_data_fetch = 1;
    }
}
