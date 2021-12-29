/*
   cgr101.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include "worker.h"
#include "spawn.h"
#include "misc.h"
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
    DIGITAL_READ,
};

enum cgr101_event {
    EVENT_IDENTIFY_COMPLETE,
    EVENT_IDENTIFY_OUTPUT,
    EVENT_DIGITAL_READ_COMPLETE,
    EVENT_DIGITAL_READ_OUTPUT,
};

enum cgr101_identify_state {
    STATE_IDENTIFY_IDLE,
    STATE_IDENTIFY_PENDING,
    STATE_IDENTIFY_COMPLETE,
};

enum cgr101_digital_read_state {
    STATE_DIGITAL_READ_IDLE,
    STATE_DIGITAL_READ_PENDING,
    STATE_DIGITAL_READ_COMPLETE,
};

struct cgr101 {
    struct spawn child;
    /* ID */
    enum cgr101_identify_state identify_state;
    int identify_output_requested;
    int device_id_len;
    char device_id[ID_MAX];
    /* Device Receiver */
    enum cgr101_rcv_state rcv_state;
    char rcv_data[RCV_MAX];
    char err_data[ERR_MAX];
    /* Digital Data Input */
    enum cgr101_digital_read_state digital_read_state;
    int digital_read_requested;
    int digital_read_data;
    /* Event Queue */
    int eventq[2];
    /* Digital Data Output */
    int digital_write_data;

};

/*
 * Event Queue Sender
 */

static void cgr101_event_send(struct info *info, enum cgr101_event event)
{
    ssize_t len_out;
    char cevent = (char)event;
    assert(event == (enum cgr101_event)cevent);
    len_out = write(info->device->eventq[1], &cevent, sizeof(cevent));
    assert(len_out == sizeof(cevent));
}


/*
 * Device Sender
 */
static int cgr101_device_send(struct info *info, const char *str)
{
    size_t len_in = strlen(str);
    ssize_t len_out;
    int result;

    len_out = write(info->device->child.stdin, str, len_in);
    result = (len_out > 0) ? ((size_t)len_out == len_in) : 0;

    return (result == 0);
}

static int cgr101_device_printf(struct info *info,
                                const char *format,
                                ...)
{
    int err = 1;
    char buf[32];
    size_t buf_len = sizeof(buf);
    size_t actual;
    va_list ap;

    assert(format != NULL);
    va_start(ap, format);
    actual = (size_t)vsnprintf(buf, buf_len, format, ap);
    /* 'old' GLIBC prints return -1 on overflow, not the actual length
     * needed, but the cast should result in a very large number
     * anyway.
     */
    assert(actual < buf_len);

    va_end(ap);

    err = cgr101_device_send(info, buf);

    return err;
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
        info->device->rcv_state = DIGITAL_READ;
        /* I<uint8_t> */
        err = 0;
        break;
    default:
        assert(0);
    }

    return err;
}

/*
 * Device Digital Data Read Handling
 */
static void cgr101_output_digital_read(struct info *info)
{
    assert(info->device->digital_read_state == STATE_DIGITAL_READ_COMPLETE);
    scpi_output_int(info->output, info->device->digital_read_data);
}

static int cgr101_digital_read_start(struct info *info)
{
    int err = cgr101_device_send(info, "D I\n");

    if (!err) {
        info->device->digital_read_state = STATE_DIGITAL_READ_PENDING;
    }

    return err;
}

static int cgr101_rcv_digital_read(struct info *info, char c)
{
    int err = 0;

    /* Done receiving. */
    info->device->digital_read_data = (unsigned char)c;
    cgr101_event_send(info, EVENT_DIGITAL_READ_COMPLETE);
    cgr101_rcv_idle(info);

    return err;
}

static void cgr101_digital_read_completion(struct info *info)
{
    /* By default, the device sends updates when the inputs change, so
     * completion may happen when in the PENDING state *or* the
     * COMPLETION state.
     */
    info->device->digital_read_state = STATE_DIGITAL_READ_COMPLETE;
}

static void cgr101_digital_read_output(struct info *info)
{
    switch (info->device->digital_read_state) {
    case STATE_DIGITAL_READ_IDLE:
        /*
         * Digital reads are started via initiate, so should never be
         * IDLE by the time a read is requested.
         */
        assert(0);
        break;
    case STATE_DIGITAL_READ_PENDING:
        /* Reschedule */
        cgr101_event_send(info, EVENT_DIGITAL_READ_OUTPUT);
        break;
    case STATE_DIGITAL_READ_COMPLETE:
        /* Done. */
        cgr101_output_digital_read(info);
        break;
    default:
        assert(0);
        break;
    }
}


/*
 * Device Identify Handling
 */

static int cgr101_identify_start(struct info *info)
{
    int err = cgr101_device_send(info, "i\n");

    if (!err) {
        info->device->identify_state = STATE_IDENTIFY_PENDING;
    }

    return err;
}

static void cgr101_identify_done(struct info *info)
{
    cgr101_event_send(info, EVENT_IDENTIFY_COMPLETE);
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
    info->device->identify_output_requested = 1;
    cgr101_event_send(info, EVENT_IDENTIFY_OUTPUT);
    return 0;
}

static void cgr101_identify_completion(struct info *info)
{
    assert(info->device->identify_state == STATE_IDENTIFY_PENDING);
    info->device->identify_state = STATE_IDENTIFY_COMPLETE;
    if (info->device->identify_output_requested) {
        cgr101_event_send(info, EVENT_IDENTIFY_OUTPUT);
    }
}

static void cgr101_identify_output(struct info *info)
{
    switch (info->device->identify_state) {
    case STATE_IDENTIFY_IDLE:
    {
        int err = cgr101_identify_start(info);
        assert(!err);
    }
        break;
    case STATE_IDENTIFY_PENDING:
        /* Reschedule */
        cgr101_event_send(info, EVENT_IDENTIFY_OUTPUT);
        break;
    case STATE_IDENTIFY_COMPLETE:
        scpi_output_printf(info->output,
                           "GMP,CGR101-SCPI,1.0,%s",
                           info->device->device_id);
        break;
    default:
        assert(0);
        break;
    }
}

/*
 * Event Handler
 */

static int cgr101_event_handler(void *arg)
{
    struct info *info = arg;
    int err = 0;
    ssize_t len;
    char eventc;
    enum cgr101_event event;

    len = read(
        info->device->eventq[0],
        &eventc,
        sizeof(eventc));

    assert(len == sizeof(eventc));
    event = (enum cgr101_event)eventc;
    switch (event) {
    case EVENT_IDENTIFY_COMPLETE:
        cgr101_identify_completion(info);
        break;
    case EVENT_IDENTIFY_OUTPUT:
        cgr101_identify_output(info);
        break;
    case EVENT_DIGITAL_READ_COMPLETE:
        cgr101_digital_read_completion(info);
        break;
    case EVENT_DIGITAL_READ_OUTPUT:
        cgr101_digital_read_output(info);
        break;
    default:
        assert(0);
        break;
    }

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
    case DIGITAL_READ:
        err = cgr101_rcv_digital_read(info, c);
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

        /* Event queue. */
        err = pipe(info->device->eventq);
        if (err) {
            break;
        }

        cloexec(info->device->eventq[0]);
        cloexec(info->device->eventq[1]);

        err = worker_add(
            info->worker,
            info->device->eventq[0],
            cgr101_event_handler,
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
    if (info->device->digital_read_requested) {
        int err = cgr101_digital_read_start(info);
        assert(!err);
    }

    return 0;
}

int cgr101_configure_digital_data(struct info *info)
{
    info->device->digital_read_requested = 1;

    return 0;
}

int cgr101_digital_data_configured(struct info *info)
{
    return info->device->digital_read_requested;
}

void cgr101_fetch_digital_data(struct info *info)
{
    cgr101_event_send(info, EVENT_DIGITAL_READ_OUTPUT);
}

void cgr101_source_digital_data(struct info *info, int value)
{
    int err;

    info->device->digital_write_data = value;
    err = cgr101_device_printf(info, "D O %d\n", value);
    assert(!err);
}

void cgr101_source_digital_dataq(struct info *info)
{
    scpi_output_int(info->output, info->device->digital_write_data);
}

void cgr101_source_waveform_frequency(struct info *info, float value)
{
    (void)info;
    (void)value;
}

void cgr101_source_waveform_frequencyq(struct info *info)
{
    (void)info;
}

void cgr101_source_waveform_function(struct info *info,
                                     const char *value)
{
    (void)info;
    (void)value;
}

void cgr101_source_waveform_functionq(struct info *info)
{
    (void)info;
}

void cgr101_source_waveform_user(struct info *info, size_t len, float *data)
{
    (void)info;
    (void)len;
    (void)data;
}

void cgr101_source_waveform_userq(struct info *info)
{
    (void)info;
}

void cgr101_source_pwm_duty_cycle(struct info *info, float value)
{
    (void)info;
    (void)value;
}

void cgr101_source_pwm_duty_cycleq(struct info *info)
{
    (void)info;
}

void cgr101_source_pwm_frequency(struct info *info, float value)
{
    (void)info;
    (void)value;
}

void cgr101_source_pwm_frequencyq(struct info *info)
{
    (void)info;
}

extern void cgr101_digitizer_coupling(struct info *info, const char *value)
{
    (void)info;
    (void)value;
}

extern void cgr101_digitizer_dataq(struct info *info, long chan_mask)
{
    (void)info;
    (void)chan_mask;
}

extern void cgr101_digitizer_concurrent(struct info *info, int value)
{
    (void)info;
    (void)value;
}

extern void cgr101_digitizer_channel_state(struct info *info,
                                           long chan_mask,
                                           int value)
{
    (void)info;
    (void)chan_mask;
    (void)value;
}

extern void cgr101_digitizer_channel_stateq(struct info *info, long chan_mask)
{
    (void)info;
    (void)chan_mask;
}

extern void cgr101_digitizer_sweep_point(struct info *info, float value)
{
    (void)info;
    (void)value;
}

extern void cgr101_digitizer_sweep_time(struct info *info, float value)
{
    (void)info;
    (void)value;
}

extern void cgr101_digitizer_sweep_interval(struct info *info, float value)
{
    (void)info;
    (void)value;
}

extern void cgr101_digitizer_sweep_pretrigger(struct info *info, float value)
{
    (void)info;
    (void)value;
}

extern void cgr101_digitizer_voltage_low(struct info *info,
                                         long chan_mask,
                                         float value)
{
    (void)info;
    (void)chan_mask;
    (void)value;
}

extern void cgr101_digitizer_voltage_offset(struct info *info,
                                            long chan_mask,
                                            float value)
{
    (void)info;
    (void)chan_mask;
    (void)value;
}

extern void cgr101_digitizer_voltage_ptp(struct info *info,
                                         long chan_mask,
                                         float value)
{
    (void)info;
    (void)chan_mask;
    (void)value;
}

extern void cgr101_digitizer_voltage_range(struct info *info,
                                           long chan_mask,
                                           float value)
{
    (void)info;
    (void)chan_mask;
    (void)value;
}

extern void cgr101_digitizer_voltage_up(struct info *info,
                                        long chan_mask,
                                        float value)
{
    (void)info;
    (void)chan_mask;
    (void)value;
}

extern void cgr101_digitizer_lowq(struct info *info, long chan_mask)
{
    (void)info;
    (void)chan_mask;
}

extern void cgr101_digitizer_offsetq(struct info *info, long chan_mask)
{
    (void)info;
    (void)chan_mask;
}

extern void cgr101_digitizer_ptpq(struct info *info, long chan_mask)
{
    (void)info;
    (void)chan_mask;
}

extern void cgr101_digitizer_rangeq(struct info *info, long chan_mask)
{
    (void)info;
    (void)chan_mask;
}

extern void cgr101_digitizer_upq(struct info *info, long chan_mask)
{
    (void)info;
    (void)chan_mask;
}

