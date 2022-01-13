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
#include <math.h>
#include "worker.h"
#include "spawn.h"
#include "misc.h"
#include "scpi_output.h"
#include "cgr101.h"

#define ID_MAX 32
#define RCV_MAX 4100 /* 4097 plus some slop */
#define ERR_MAX 1024

#define COUNT_OF(a) (sizeof((a))/sizeof((a)[0]))
#define FPEPSILON 0.0001
#define FPWITHIN(a,b,e) (((a)>=((b)-(e))) && ((a)<=((b)+(e))))

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
    SCOPE_OFFSET,
    SCOPE_STATUS,
    SCOPE_READ,
    SCOPE_ADDR,
    SCOPE_DATA,
};

enum cgr101_event {
    EVENT_IDENTIFY_COMPLETE,
    EVENT_IDENTIFY_OUTPUT,
    EVENT_DIGITAL_READ_COMPLETE,
    EVENT_DIGITAL_READ_OUTPUT,
    EVENT_SCOPE_STATUS_COMPLETE,
    EVENT_SCOPE_STATUS_OUTPUT,
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

enum cgr101_scope_offset_state {
    STATE_SCOPE_OFFSET_IDLE,
    STATE_SCOPE_OFFSET_EXPECT_A_HIGH,
    STATE_SCOPE_OFFSET_EXPECT_B_HIGH,
    STATE_SCOPE_OFFSET_EXPECT_A_LOW,
    STATE_SCOPE_OFFSET_EXPECT_B_LOW,
    STATE_SCOPE_OFFSET_COMPLETE,
};

enum cgr101_scope_status_state {
    STATE_SCOPE_STATUS_IDLE,
    STATE_SCOPE_STATUS_PENDING,
    STATE_SCOPE_STATUS_COMPLETE,
};

enum cgr101_scope_addr_state {
    STATE_SCOPE_ADDR_IDLE,
    STATE_SCOPE_ADDR_EXPECT_HIGH,
    STATE_SCOPE_ADDR_EXPECT_LOW,
    STATE_SCOPE_ADDR_COMPLETE,
};

enum cgr101_scope_data_state {
    STATE_SCOPE_DATA_IDLE,
    STATE_SCOPE_DATA_EXPECT_A_HIGH,
    STATE_SCOPE_DATA_EXPECT_B_HIGH,
    STATE_SCOPE_DATA_EXPECT_A_LOW,
    STATE_SCOPE_DATA_EXPECT_B_LOW,
    STATE_SCOPE_DATA_COMPLETE,
};

enum cgr101_scope_trigger_source {
    SCOPE_TRIGGER_SOURCE_INT,
    SCOPE_TRIGGER_SOURCE_EXT,
    SCOPE_TRIGGER_SOURCE_IMM,
};

enum cgr101_waveform_shape {
    WAV_NONE,
    WAV_RAND,
    WAV_SIN,
    WAV_SQU,
    WAV_TRI,
    WAV_USER,
};

#define WAVEFORM_USER_MAX 256

struct cgr101_waveform_map_s {
    const char *name;
    enum cgr101_waveform_shape shape;
};

static struct cgr101_waveform_map_s cgr101_waveform_map[] = {
    { "RAND", WAV_RAND},
    { "SIN", WAV_SIN},
    { "SQU", WAV_SQU},
    { "TRI", WAV_TRI},
    { "USER", WAV_USER},
    { NULL, WAV_NONE},
};

#define SCOPE_NUM_CHAN 2
#define SCOPE_NUM_RANGE 2
#define SCOPE_NUM_SAMPLE 1024
#define SCOPE_DEFAULT_LOW (-25.0)
#define SCOPE_DEFAULT_HIGH 25.0
#define SCOPE_DEFAULT_MIDPOINT 0.0
#define SCOPE_DEFAULT_PTP 50.0
#define SCOPE_SR_DIV_MAX 15
#define SCOPE_SR_MAX 20.0e6 /* 20MHz */
#define SCOPE_NUM_DATA = (SCOPE_NUM_SAMPLE*SCOPE_NUM_CHAN*2)
static char cgr101_range_cmd[SCOPE_NUM_CHAN][SCOPE_NUM_RANGE] = {
    {'A','a'},
    {'B','b'}
};

/*
 * By experiment and code inspection of the CGR-101 TCL code, a manual
 * trigger must be delayed from an "S G" command by the following
 * times in milliseconds indexed by the sample rate code. The TCL code
 * did not use a sample rate code of '13' or '3', so those delays are
 * estimated.
 */
static unsigned int cgr101_manual_trigger_delay_ms[] = {
     500,   500,   500,   500,
     500,   500,   500,   500,
     500,   500,   500,   500,
    1000,  2000,  2000,  2000
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
    /* Waveform */
    struct {
        const char *name;
        enum cgr101_waveform_shape shape;
        double user[WAVEFORM_USER_MAX];
        double frequency;
    } waveform;
    struct {
        enum cgr101_scope_offset_state offset_state;
        double sweep_time;
        int sample_rate_divisor;        /* derived from sweep_time */
        int internal_trigger_source;
        int trigger_polarity;
        int trigger_external;
        int trigger_filter_disable;
        enum cgr101_scope_status_state status_state;
        int status;
        double trigger_level;
        enum cgr101_scope_trigger_source trigger_source;
        double trigger_offset;     /* SCPI SENSe:SWEep:OFFSet:POINts */
        double trigger_ref;        /* SCPI SENSe:SWEep:OREFerence:POINts */
        enum cgr101_scope_addr_state addr_state;
        int addr;               /* offset of first data capture. */
        enum cgr101_scope_data_state data_state;
        int data_count;
        struct {
            double input_low;
            double input_high;
            double input_midpoint;
            double input_ptp;
            int input_low_range; /* derived from above */
            int offset_low;
            int offset_high;
            int enable;
            double data[SCOPE_NUM_SAMPLE];
        } channel[SCOPE_NUM_CHAN];
    } scope;
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
    case 'O':
        info->device->rcv_state = SCOPE_OFFSET;
        /* O<int8_t>*4 */
        err = 0;
        break;
    case 'S':
        info->device->rcv_state = SCOPE_STATUS;
        /* 'Status N' */
        err = 0;
        break;
    case 'A':
        info->device->rcv_state = SCOPE_ADDR;
        /* O<int8_t>*2 */
        err = 0;
        break;
    case 'D':
        info->device->rcv_state = SCOPE_DATA;
        /* 'DA1a1B1b2A2a2B2b2...' */
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
 * Offset Handling
 */

static int cgr101_rcv_scope_offset(struct info *info, char c)
{
    int err = 0;

    switch (info->device->scope.offset_state) {
    case STATE_SCOPE_OFFSET_EXPECT_A_HIGH:
        info->device->scope.channel[0].offset_high = (int)c; /* signed */
        info->device->scope.offset_state = STATE_SCOPE_OFFSET_EXPECT_B_HIGH;
        break;
    case STATE_SCOPE_OFFSET_EXPECT_B_HIGH:
        info->device->scope.channel[1].offset_high = (int)c; /* signed */
        info->device->scope.offset_state = STATE_SCOPE_OFFSET_EXPECT_A_LOW;
        break;
    case STATE_SCOPE_OFFSET_EXPECT_A_LOW:
        info->device->scope.channel[0].offset_low = (int)c; /* signed */
        info->device->scope.offset_state = STATE_SCOPE_OFFSET_EXPECT_B_LOW;
        break;
    case STATE_SCOPE_OFFSET_EXPECT_B_LOW:
        info->device->scope.channel[1].offset_low = (int)c; /* signed */
        info->device->scope.offset_state = STATE_SCOPE_OFFSET_COMPLETE;
        /* Done receiving. */
        cgr101_rcv_idle(info);
        break;
    default:
        assert(0);
        break;
    }

    return err;
}

/*
 * Oscilloscope Status Handling
 */
static int cgr101_scope_status_start(struct info *info)
{
    int err = cgr101_device_send(info, "S S\n");

    if (!err) {
        info->device->scope.status_state = STATE_SCOPE_STATUS_PENDING;
    }

    return err;
}

static int cgr101_rcv_scope_status(struct info *info, char c)
{
    int err = 0;

    switch (info->device->scope.status_state) {
    case STATE_SCOPE_STATUS_PENDING:
        if ((c >= '1') && (c <= '6')) {
            info->device->scope.status = (int)c - '0'; /* convert to int 1-6 */
            info->device->scope.status_state = STATE_SCOPE_STATUS_COMPLETE;
            cgr101_event_send(info, EVENT_SCOPE_STATUS_COMPLETE);

            /* Done receiving. */
            cgr101_rcv_idle(info);
        }
        break;
    default:
        assert(0);
        break;
    }

    return err;
}

static void cgr101_scope_status_output(struct info *info)
{
    switch (info->device->scope.status_state) {
    case STATE_SCOPE_STATUS_IDLE:
    {
        int err = cgr101_scope_status_start(info);
        assert(!err);
    }
        break;
    case STATE_SCOPE_STATUS_PENDING:
        /* Reschedule */
        cgr101_event_send(info, EVENT_SCOPE_STATUS_OUTPUT);
        break;
    case STATE_SCOPE_STATUS_COMPLETE:
        scpi_output_int(info->output, info->device->scope.status);
        break;
    default:
        assert(0);
        break;
    }
}

static void cgr101_scope_status_completion(struct info *info)
{
    cgr101_event_send(info, EVENT_SCOPE_STATUS_OUTPUT);
}

/*
 * Oscilloscope Data Handling
 */

static void cgr101_digitizer_update_control(struct info *info)
{
    int ctl;
    int err;

    assert(info->device->scope.sample_rate_divisor >= 0);
    assert(info->device->scope.sample_rate_divisor <= SCOPE_SR_DIV_MAX);
    assert(info->device->scope.internal_trigger_source >= 0);
    assert(info->device->scope.internal_trigger_source <= 1);
    assert(info->device->scope.trigger_polarity >= 0);
    assert(info->device->scope.trigger_polarity <= 1);
    assert(info->device->scope.trigger_external >= 0);
    assert(info->device->scope.trigger_external <= 1);
    assert(info->device->scope.trigger_filter_disable >= 0);
    assert(info->device->scope.trigger_filter_disable <= 1);

    ctl = info->device->scope.sample_rate_divisor;
    ctl |= (info->device->scope.internal_trigger_source << 4);
    ctl |= (info->device->scope.trigger_polarity << 5);
    ctl |= (info->device->scope.trigger_external << 6);
    /* (scope.tcl)scope::updateScopeControlReg twiddles with bit 7
     * which is not documented in circuit-gear-manual.pdf */
    ctl |= (info->device->scope.trigger_filter_disable << 7);

    err = cgr101_device_printf(info, "S R %d\n", ctl);
    assert(!err);
}

static void cgr101_digitizer_manual_trigger(struct info *info)
{
    int cur_ext_trig = info->device->scope.trigger_external;
    int err;

    if (cur_ext_trig == 0) {
        info->device->scope.trigger_external = 1;
        cgr101_digitizer_update_control(info);
    }

    /* Blocking Delay. */
    assert(info->device->scope.sample_rate_divisor >= 0);
    assert(info->device->scope.sample_rate_divisor <= SCOPE_SR_DIV_MAX);
    assert(COUNT_OF(cgr101_manual_trigger_delay_ms) == SCOPE_SR_DIV_MAX+1);
    usleep(cgr101_manual_trigger_delay_ms[
               info->device->scope.sample_rate_divisor
               ]*1000);

    /* Manual Trigger; needs ext trigger */
    err = cgr101_device_send(info, "S D 5\n");
    assert(!err);
    err = cgr101_device_send(info, "S D 4\n");
    assert(!err);

    /* Restore internal trigger */
    if (cur_ext_trig == 0) {
        info->device->scope.trigger_external = 0;
        cgr101_digitizer_update_control(info);
    }
}

static int cgr101_digitizer_start(struct info *info, int manual)
{
    int err = 1;
    int low;
    int high;
    int post_trigger = SCOPE_NUM_SAMPLE;
    int trigger_ref = (int)info->device->scope.trigger_ref;
    int trigger_offset = (int)info->device->scope.trigger_offset;

    /* Data return state initialization */

    info->device->scope.addr_state = STATE_SCOPE_ADDR_EXPECT_HIGH;
    info->device->scope.addr = -1; /* Illegal value */
    info->device->scope.data_state = STATE_SCOPE_DATA_EXPECT_A_HIGH;
    info->device->scope.data_count = 0;

    do {
        /* Trigger Level - assume done */

        /* Post Trigger Sample Count */
        post_trigger -= trigger_ref;
        post_trigger -= trigger_offset;

        if (post_trigger < 0 || post_trigger >= SCOPE_NUM_SAMPLE) {
            /* Error: post_trigger out of bounds. */
            break;
        }
        low = post_trigger & 0xff;
        high = (post_trigger >> 8) & 0x03;
        err = cgr101_device_printf(info, "S C %d %d\n", high, low);
        if (err) {
            break;
        }

        /* GO */

        err = cgr101_device_send(info, "S G\n");
        if (err) {
            break;
        }

        /* Manual Trigger handling */
        if (info->device->scope.trigger_source == SCOPE_TRIGGER_SOURCE_IMM ||
            manual) {
            cgr101_digitizer_manual_trigger(info);
        }

        info->sweep_status = 1;

    } while (0);

    return err;
}

static int cgr101_rcv_scope_addr(struct info *info, char c)
{
    int err = 0;

    switch (info->device->scope.addr_state) {
    case STATE_SCOPE_ADDR_EXPECT_HIGH:
        assert(c >= 0);
        assert(c <= 3);
        info->device->scope.addr = c<<8;
        info->device->scope.addr_state = STATE_SCOPE_ADDR_EXPECT_LOW;
        break;
    case STATE_SCOPE_ADDR_EXPECT_LOW:
        info->device->scope.addr |= c;
        info->device->scope.addr_state = STATE_SCOPE_ADDR_COMPLETE;
        cgr101_rcv_idle(info);
        /* Get the buffer. */
        err = cgr101_device_send(info, "S B\n");
        if (err) {
            break;
        }
        /* Done receiving. */
        break;
    default:
        assert(0);
        break;
    }

    return err;
}

static void cgr101_rcv_scope_data_chan(struct info *info,
                                       int chan,
                                       int byte,
                                       char c)
{
    int idx = info->device->scope.data_count;

    assert(idx >= 0);
    assert((size_t)idx < COUNT_OF(info->device->scope.channel[chan].data));
    if (byte) {
        info->device->scope.channel[chan].data[idx] = (double)(c<<8);
    } else {
        info->device->scope.channel[chan].data[idx] += (double)c;
    }
}

static int cgr101_rcv_scope_data(struct info *info, char c)
{
    int err = 0;

    switch (info->device->scope.data_state) {
    case STATE_SCOPE_DATA_EXPECT_A_HIGH:
        cgr101_rcv_scope_data_chan(info, 0, 1, c);
        info->device->scope.data_state = STATE_SCOPE_DATA_EXPECT_A_LOW;
        break;
    case STATE_SCOPE_DATA_EXPECT_A_LOW:
        cgr101_rcv_scope_data_chan(info, 0, 0, c);
        info->device->scope.data_state = STATE_SCOPE_DATA_EXPECT_B_HIGH;
        break;
    case STATE_SCOPE_DATA_EXPECT_B_HIGH:
        cgr101_rcv_scope_data_chan(info, 1, 1, c);
        info->device->scope.data_state = STATE_SCOPE_DATA_EXPECT_B_LOW;
        break;
    case STATE_SCOPE_DATA_EXPECT_B_LOW:
        cgr101_rcv_scope_data_chan(info, 1, 0, c);
        info->device->scope.data_count++;
        if (info->device->scope.data_count == SCOPE_NUM_SAMPLE) {
            info->device->scope.data_state = STATE_SCOPE_DATA_COMPLETE;
            info->sweep_status = 0;
            cgr101_rcv_idle(info);
            /* Done receiving. */
        } else {
            info->device->scope.data_state = STATE_SCOPE_DATA_EXPECT_A_HIGH;
        }
        break;
    default:
        assert(0);
        break;
    }

    return err;
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
    case EVENT_SCOPE_STATUS_COMPLETE:
        cgr101_scope_status_completion(info);
        break;
    case EVENT_SCOPE_STATUS_OUTPUT:
        cgr101_scope_status_output(info);
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
    case SCOPE_OFFSET:
        err = cgr101_rcv_scope_offset(info, c);
        break;
    case SCOPE_STATUS:
        err = cgr101_rcv_scope_status(info, c);
        break;
    case SCOPE_ADDR:
        err = cgr101_rcv_scope_addr(info, c);
        break;
    case SCOPE_DATA:
        err = cgr101_rcv_scope_data(info, c);
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
 * Waveform programming
 */

static void cgr101_waveform_lookup(const char *value,
                                   enum cgr101_waveform_shape *shape)
{
    struct cgr101_waveform_map_s *p;

    assert(value);
    assert(shape);
    for (p = cgr101_waveform_map; p != NULL; p++) {
        if (!strcmp(value, p->name)) {
            *shape = p->shape;
            break;
        }
    }
    assert(p != NULL);
}

/*
 * Waveform functions:
 *  phase: 0.0 .. 1.0
 *  amplitude: -1.0 .. 1.0
 */

static double cgr101_fn_sin(double phase)
{
    double angle = 2.0 * M_PI * phase;
    return ((double)sin(angle));
}


static double cgr101_fn_squ(double phase)
{
    return (double)((phase < 0.5) ? 1.0 : -1.0);
}

static double cgr101_fn_tri(double phase)
{
    double amp = phase * 2.0;
    return (double)((phase < 0.5) ? (amp - 1.0) : (1.0 - amp));
}

typedef double (*wff_t)(double phase);

static void cgr101_waveform_create_fn(double *data, size_t points, wff_t wfn)
{
    size_t i;
    double phase = 0.0;
    double phase_incr = 1.0/((double)points);
    for (i=0; i<points; i++) {
        data[i] = (double)wfn(phase);
        phase += phase_incr;
    }
}

static void cgr101_waveform_create(struct info *info,
                                   enum cgr101_waveform_shape shape)
{
    switch (shape) {
    case WAV_RAND:
        /* Handled by device programming. */
        break;
    case WAV_SIN:
        cgr101_waveform_create_fn(info->device->waveform.user,
                                  sizeof(info->device->waveform.user),
                                  cgr101_fn_sin);
        break;
    case WAV_SQU:
        cgr101_waveform_create_fn(info->device->waveform.user,
                                  sizeof(info->device->waveform.user),
                                  cgr101_fn_squ);
        break;
    case WAV_TRI:
        cgr101_waveform_create_fn(info->device->waveform.user,
                                  sizeof(info->device->waveform.user),
                                  cgr101_fn_tri);
        break;
    default:
        assert(0);
        break;
    }
}

static void cgr101_waveform_program(struct info *info)
{
    int err;

    if (info->device->waveform.shape == WAV_RAND) {
        err = cgr101_device_printf(info, "W N\n");
        assert(!err);
    } else {
        size_t i;
        for (i=0; i<COUNT_OF(info->device->waveform.user); i++) {
            int val = (int)info->device->waveform.user[i]*255;
            err = cgr101_device_printf(info, "W S %d %d\n", i, val);
            assert(!err);
        }
        err = cgr101_device_printf(info, "W P\n");
        assert(!err);
        err = cgr101_device_printf(info, "W A 255\n"); /*FIXME*/
        assert(!err);
    }
}

static void cgr101_waveform_user(struct info *info,
                                 size_t len,
                                 double *data)
{
    (void)info;
    (void)len;
    (void)data;
}

static void cgr101_waveform_userq(struct info *info)
{
    (void)info;
}

/*
 * Oscilloscope Handling
 */

static void cgr101_digitizer_set_midpoint_ptp(struct info *info, int chan)
{
    double low = info->device->scope.channel[chan].input_low;
    double high = info->device->scope.channel[chan].input_high;
    double ptp = high - low;
    double midpoint = low + (ptp/2.0);
    info->device->scope.channel[chan].input_ptp = ptp;
    info->device->scope.channel[chan].input_midpoint = midpoint;
}

static void cgr101_digitizer_set_high_low(struct info *info, int chan)
{
    double ptp = info->device->scope.channel[chan].input_ptp;
    double midpoint = info->device->scope.channel[chan].input_midpoint;
    double low = midpoint - (ptp/2.0);
    double high = midpoint + (ptp/2.0);
    info->device->scope.channel[chan].input_low = low;
    info->device->scope.channel[chan].input_high = high;
}

static void cgr101_digitizer_set_range(struct info *info,int chan)
{
    int err;
    int low_range = (info->device->scope.channel[chan].input_low >= -2.5 &&
                     info->device->scope.channel[chan].input_high <= 2.5);
    assert(chan >= 0 && chan < SCOPE_NUM_CHAN);
    assert(low_range >= 0 && low_range < SCOPE_NUM_RANGE);
    /* Only set preamp when changing */
    if (info->device->scope.channel[chan].input_low_range != low_range) {
        info->device->scope.channel[chan].input_low_range = low_range;
        err = cgr101_device_printf(info, "S P %c\n",
                                   cgr101_range_cmd[chan][low_range]);
        assert(!err);
    }
}

static void cgr101_sweep_time(struct info *info, double time)
{
    double sweep_time = (double)SCOPE_NUM_SAMPLE/SCOPE_SR_MAX;
    double sweep_time2 = sweep_time * 2.0;
    int sweep_div;

    for (sweep_div = 0; sweep_div <= SCOPE_SR_DIV_MAX; sweep_div++) {
        if (time >= sweep_time && time < sweep_time2) {
            break;
        }
        sweep_time = sweep_time2;
        sweep_time2 = sweep_time * 2.0;
    }

    if (sweep_div >= 0 && sweep_div <= SCOPE_SR_DIV_MAX) {
        info->device->scope.sweep_time = sweep_time;
        info->device->scope.sample_rate_divisor = sweep_div;
    } else {
        /*error*/
    }
}


/*
 * Initialization
 */

static void cgr101_device_init(struct info *info)
{
    int chan;
    int err;
    double midpoint = (double)SCOPE_NUM_SAMPLE/2;
    /*Consistency check*/
    assert(COUNT_OF(info->device->scope.channel) == SCOPE_NUM_CHAN);
    /* Set Defaults */
    info->device->scope.trigger_offset = 0;
    info->device->scope.trigger_ref = midpoint;
    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        info->device->scope.channel[chan].input_low = SCOPE_DEFAULT_LOW;
        info->device->scope.channel[chan].input_high = SCOPE_DEFAULT_HIGH;
        info->device->scope.channel[chan].input_midpoint =
            SCOPE_DEFAULT_MIDPOINT;
        info->device->scope.channel[chan].input_ptp = SCOPE_DEFAULT_PTP;
        /* Force */
        info->device->scope.channel[chan].input_low_range = -1;
        cgr101_digitizer_set_range(info, chan);
    }

    /* Query device for offsets. */
    if (info->device->scope.offset_state != STATE_SCOPE_OFFSET_EXPECT_A_HIGH) {
        info->device->scope.offset_state = STATE_SCOPE_OFFSET_EXPECT_A_HIGH;
        err = cgr101_device_send(info, "S O\n"); /* Get offsets. */
        assert(!err);
    }
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

        /* Initialize device. */
        cgr101_device_init(info);
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
    int err;

    if (info->device->digital_read_requested) {
        err = cgr101_digital_read_start(info);
        assert(!err);
    }

    if (info->device->scope.channel[0].enable ||
        info->device->scope.channel[1].enable) {
        err = cgr101_digitizer_start(info, 0);
        assert(!err);
    }


    return 0;
}

int cgr101_initiate_immediate(struct info *info)
{
    int err;

    if (info->device->digital_read_requested) {
        err = cgr101_digital_read_start(info);
        assert(!err);
    }

    if (info->device->scope.channel[0].enable ||
        info->device->scope.channel[1].enable) {
        err = cgr101_digitizer_start(info, 1);
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

#define K1 0.09313225746
void cgr101_source_waveform_frequency(struct info *info, double value)
{
    int phase_incr = (int)floor(value/K1);
    int f0 = (phase_incr >> 24) & 0xff;
    int f1 = (phase_incr >> 16) & 0xff;
    int f2 = (phase_incr >>  8) & 0xff;
    int f3 = (phase_incr)       & 0xff;
    int err;

    err = cgr101_device_printf(info, "W F %d %d %d %d\n", f0, f1, f2, f3);
    assert(!err);
    info->device->waveform.frequency = value;
}

void cgr101_source_waveform_frequencyq(struct info *info)
{
    scpi_output_fp(info->output, info->device->waveform.frequency);
}

void cgr101_source_waveform_function(struct info *info,
                                     const char *value)
{
    enum cgr101_waveform_shape shape;

    info->device->waveform.name = value;
    cgr101_waveform_lookup(value, &shape);
    cgr101_waveform_create(info, shape);
    cgr101_waveform_program(info);
}

void cgr101_source_waveform_functionq(struct info *info)
{
    scpi_output_str(info->output, info->device->waveform.name);
}

void cgr101_source_waveform_user(struct info *info, size_t len, double *data)
{
    info->device->waveform.name = "USER";
    info->device->waveform.shape = WAV_USER;
    cgr101_waveform_user(info, len, data);
    cgr101_waveform_program(info);
}

void cgr101_source_waveform_userq(struct info *info)
{
    cgr101_waveform_userq(info);
}

void cgr101_source_pwm_duty_cycle(struct info *info, double value)
{
    (void)info;
    (void)value;
}

void cgr101_source_pwm_duty_cycleq(struct info *info)
{
    (void)info;
}

void cgr101_source_pwm_frequency(struct info *info, double value)
{
    (void)info;
    (void)value;
}

void cgr101_source_pwm_frequencyq(struct info *info)
{
    (void)info;
}

void cgr101_digitizer_coupling(struct info *info, const char *value)
{
    (void)info;
    (void)value;
}

void cgr101_digitizer_dataq(struct info *info, long chan_mask)
{
    int chan;
    int j;
    int offset;
    double data;

    assert(info->device->scope.addr_state == STATE_SCOPE_ADDR_COMPLETE);
    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        if (!(chan_mask & 1<<chan)) {
            continue;
        }
        for (j=0; j<SCOPE_NUM_SAMPLE; j++) {
            offset = info->device->scope.addr;
            offset += j;
            if (offset == SCOPE_NUM_SAMPLE) {
                /* wrapped */
                offset = 0;
            }
            data = info->device->scope.channel[chan].data[offset];
            /* FIXME: handle offset and gain scaling. */
            scpi_output_fp(info->output, data);
        }
    }
}

void cgr101_digitizer_concurrent(struct info *info, int value)
{
    (void)info;
    (void)value;
}

void cgr101_digitizer_channel_state(struct info *info,
                                    long chan_mask,
                                    int value)
{
    int chan;

    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        if (!(chan_mask & 1<<chan)) {
            continue;
        }
        info->device->scope.channel[chan].enable = value;
    }
}

void cgr101_digitizer_channel_stateq(struct info *info, long chan_mask)
{
    int chan;

    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        if (!(chan_mask & 1<<chan)) {
            continue;
        }
        scpi_output_int(info->output, info->device->scope.channel[chan].enable);
    }
}

void cgr101_digitizer_offset_point(struct info *info, double value)
{
    double offset = floor(value);
    double midpoint = (double)SCOPE_NUM_SAMPLE/2;

    if (offset >=-midpoint && offset < midpoint) {
        info->device->scope.trigger_offset = offset;
    } else {
        /*Some error */
    }
}

void cgr101_digitizer_offset_pointq(struct info *info)
{
    scpi_output_fp(info->output, info->device->scope.trigger_offset);
}

void cgr101_digitizer_offset_time(struct info *info, double value)
{
    double interval = info->device->scope.sweep_time / SCOPE_NUM_SAMPLE;
    cgr101_digitizer_offset_point(info, value*interval);
}

void cgr101_digitizer_offset_timeq(struct info *info)
{
    double value =
        (info->device->scope.sweep_time / SCOPE_NUM_SAMPLE) *
        info->device->scope.trigger_offset;
    scpi_output_fp(info->output, value);
}

void cgr101_digitizer_oref_loc(struct info *info, double value)
{
    double offset = floor(value * SCOPE_NUM_SAMPLE);

    if (offset >=0 && offset < SCOPE_NUM_SAMPLE) {
        info->device->scope.trigger_ref = offset;
    } else {
        /*Some error */
    }
}

void cgr101_digitizer_oref_locq(struct info *info)
{
    double loc = info->device->scope.trigger_ref/SCOPE_NUM_SAMPLE;
    scpi_output_fp(info->output, loc);
}

void cgr101_digitizer_oref_point(struct info *info, double value)
{
    double offset = floor(value);

    if (offset >= 0 && offset < SCOPE_NUM_SAMPLE) {
        info->device->scope.trigger_ref = offset;
    } else {
        /*Some error */
    }
}

void cgr101_digitizer_oref_pointq(struct info *info)
{
    scpi_output_fp(info->output, info->device->scope.trigger_ref);
}

void cgr101_digitizer_sweep_pointq(struct info *info)
{
    scpi_output_int(info->output, SCOPE_NUM_SAMPLE);
}

void cgr101_digitizer_sweep_time(struct info *info, double value)
{
    cgr101_sweep_time(info, value);
}

void cgr101_digitizer_sweep_timeq(struct info *info)
{
    double value = info->device->scope.sweep_time;
    scpi_output_fp(info->output, value);

}

void cgr101_digitizer_sweep_interval(struct info *info, double value)
{
    cgr101_sweep_time(info, value * SCOPE_NUM_SAMPLE);
}

void cgr101_digitizer_sweep_time_intervalq(struct info *info)
{
    double value = info->device->scope.sweep_time / SCOPE_NUM_SAMPLE;
    scpi_output_fp(info->output, value);
}

/*
 * Range setting:
 *   low:
 *     high:        unchanged
 *     ptp:         high - low
 *     midpoint:    low + (ptp / 2)
 *   high:
 *     low:         unchanged
 *     ptp:         high - low
 *     midpoint:    low + (ptp / 2)
 *   ptp:
 *     low:         midpoint - ptp/2
 *     high:        midpoint + ptp/2
 *     midpoint:    unchanged
 *   midpoint:
 *     low:         midpoint - ptp/2
 *     high:        midpoint + ptp/2
 *     ptp:         unchanged
 *
 * preamp: (low >= -2.5 && high <= 2.5) ? low_setting : high_setting
 *
 */

/* Lowest expected voltage. */
void cgr101_digitizer_voltage_low(struct info *info,
                                  long chan_mask,
                                  double value)
{
    int chan;

    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        if (!(chan_mask & 1<<chan)) {
            continue;
        }
        info->device->scope.channel[chan].input_low = value;
        cgr101_digitizer_set_midpoint_ptp(info, chan);
        cgr101_digitizer_set_range(info, chan);
    }
}

/* Midpoint expected voltage. */
void cgr101_digitizer_voltage_offset(struct info *info,
                                     long chan_mask,
                                     double value)
{
    int chan;

    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        if (!(chan_mask & 1<<chan)) {
            continue;
        }
        info->device->scope.channel[chan].input_midpoint = value;
        cgr101_digitizer_set_high_low(info, chan);
        cgr101_digitizer_set_range(info, chan);
    }
}

void cgr101_digitizer_voltage_ptp(struct info *info,
                                  long chan_mask,
                                  double value)
{
    int chan;

    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        if (!(chan_mask & 1<<chan)) {
            continue;
        }
        info->device->scope.channel[chan].input_ptp = value;
        cgr101_digitizer_set_high_low(info, chan);
        cgr101_digitizer_set_range(info, chan);
    }
}

/* Highest expected voltage. */
void cgr101_digitizer_voltage_up(struct info *info,
                                 long chan_mask,
                                 double value)
{
    int chan;

    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        if (!(chan_mask & 1<<chan)) {
            continue;
        }
        info->device->scope.channel[chan].input_high = value;
        cgr101_digitizer_set_midpoint_ptp(info, chan);
        cgr101_digitizer_set_range(info, chan);
    }
}

void cgr101_digitizer_lowq(struct info *info, long chan_mask)
{
    int chan;

    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        if (!(chan_mask & 1<<chan)) {
            continue;
        }
        scpi_output_fp(info->output,
                       info->device->scope.channel[chan].input_low);
    }
}

void cgr101_digitizer_offsetq(struct info *info, long chan_mask)
{
    int chan;

    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        if (!(chan_mask & 1<<chan)) {
            continue;
        }
        scpi_output_fp(info->output,
                       info->device->scope.channel[chan].input_midpoint);
    }
}

void cgr101_digitizer_ptpq(struct info *info, long chan_mask)
{
    int chan;

    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        if (!(chan_mask & 1<<chan)) {
            continue;
        }
        scpi_output_fp(info->output,
                       info->device->scope.channel[chan].input_ptp);
    }
}

void cgr101_digitizer_upq(struct info *info, long chan_mask)
{
    int chan;

    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        if (!(chan_mask & 1<<chan)) {
            continue;
        }
        scpi_output_fp(info->output,
                       info->device->scope.channel[chan].input_high);
    }
}

void cgr101_digitizer_statq(struct info *info)
{
    cgr101_event_send(info, EVENT_SCOPE_STATUS_OUTPUT);
}

void cgr101_digitizer_reset(struct info *info)
{
    int err;
    err = cgr101_device_send(info, "S D 1\n");
    assert(!err);
    err = cgr101_device_send(info, "S D 0\n");
    assert(!err);
}

void cgr101_digitizer_immediate(struct info *info)
{
    cgr101_digitizer_start(info, 1);
}

void cgr101_trigger_coupling(struct info *info, const char *value)
{
    /* Ignore for now. */
    (void)info;
    (void)value;
}

#define K2 0.052421484375
void cgr101_trigger_level(struct info *info, double value)
{
    int trigger_value;
    double gain;
    int err;

    info->device->scope.trigger_level = value;
    gain = 1.0;
    /* Gain depends on preamp setting for the trigger source
     * channel. If external, use B - that's what the CGR-101 TCL code
     * does. */
    if (((info->device->scope.trigger_external ||
         info->device->scope.internal_trigger_source) &&
         info->device->scope.channel[1].input_low_range) ||
        ((!info->device->scope.internal_trigger_source) &&
         info->device->scope.channel[1].input_low_range)) {
        gain = 10.0;
    }
    trigger_value = (int)floor(511.0 - gain * (value/K2));
    assert(trigger_value >= 0);
    err = cgr101_device_printf(info,
                               "S T %d %d\n",
                               trigger_value >> 8,
                               trigger_value & 0xff);
    assert(!err);
}

void cgr101_trigger_levelq(struct info *info)
{
    scpi_output_fp(info->output, info->device->scope.trigger_level);
}

void cgr101_trigger_slope(struct info *info, const char *value)
{

    assert(value);
    if (!strcmp(value, "POS")) {
        info->device->scope.trigger_polarity = 1;
    } else if (!strcmp(value, "NEG")) {
        info->device->scope.trigger_polarity = 0;
    } else {
        assert(0);
    }

    cgr101_digitizer_update_control(info);
}

void cgr101_trigger_slopeq(struct info *info)
{
    char *str = info->device->scope.trigger_polarity ? "POS" : "NEG";
    scpi_output_str(info->output, str);
}

void cgr101_trigger_source(struct info *info, const char *value)
{
    /*IMM INT EXT - FIXME how to pick A, B? */
    assert(value);
    if (!strcmp(value, "IMM")) {
        info->device->scope.trigger_source = SCOPE_TRIGGER_SOURCE_IMM;
    } else if (!strcmp(value, "INT")) {
        info->device->scope.trigger_source = SCOPE_TRIGGER_SOURCE_INT;
    } else if (!strcmp(value, "EXT")) {
        info->device->scope.trigger_source = SCOPE_TRIGGER_SOURCE_EXT;
    } else {
        assert(0);
    }
    cgr101_digitizer_update_control(info);
}

void cgr101_trigger_sourceq(struct info *info)
{
    char *str = NULL;
    switch (info->device->scope.trigger_source) {
    case SCOPE_TRIGGER_SOURCE_IMM:
        str = "IMM";
        break;
    case SCOPE_TRIGGER_SOURCE_INT:
        str = "INT";
        break;
    case SCOPE_TRIGGER_SOURCE_EXT:
        str = "EXT";
        break;
    default:
        assert(0);
    }
    scpi_output_str(info->output, str);

}

