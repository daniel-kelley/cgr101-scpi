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
#include <sys/time.h>
#include "worker.h"
#include "spawn.h"
#include "event.h"
#include "scpi_output.h"
#include "scpi_error.h"
#include "cgr101.h"

#define ID_MAX 32       /* '*' identify message */
#define RCV_MAX 8192    /* 'D' message; 4097 plus a lot of slop */
#define ERR_MAX 1024    /* stderr from device interface */
#define E_MAX 32        /* 'E' message */

#define COUNT_OF(a) (sizeof((a))/sizeof((a)[0]))
#define FPEPSILON 0.0001
#define FPWITHIN(a,b,e) (((a)>=((b)-(e))) && ((a)<=((b)+(e))))

/* Digitizer step and format offsets */
#define STEP_HIGH 0.0521
#define STEP_LOW  0.00592
#define MP8  128      /* 8 bit Input Offset midpoint */
#define MP10 511      /* 10 bit sample midpoint */

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
    ERROR_MSG,
    INTERRUPT_MSG,
};

enum cgr101_identify_state {
    STATE_IDENTIFY_IDLE,
    STATE_IDENTIFY_PENDING,
    STATE_IDENTIFY_COMPLETE,
};

enum cgr101_error_state {
    STATE_ERROR_IDLE,
    STATE_ERROR_PENDING,
    STATE_ERROR_COMPLETE,
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
    { "NONE", WAV_NONE},
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
#define SCOPE_MIN_SWEEP_TIME ((double)SCOPE_NUM_SAMPLE/SCOPE_SR_MAX)
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

struct digital_event {
    struct timeval tv;
    int data;
};

/* Channels are one based, but channel masks are zero based. */
#define CHAN(n) ((n)-1)
#define INTERRUPT_CHANNEL_MASK (1<<CHAN(4))
#define DIGITAL_DATA_CHANNEL_MASK (1<<CHAN(6))

#define DIGITAL_EVENT_INTERRUPT 256

struct cgr101_event_mode_map_s {
    const char *name;
    char mode;
};

static struct cgr101_event_mode_map_s cgr101_event_mode_map[] = {
    { "NONE", 'D'},
    { "POS", 'R'},
    { "NEG", 'F'},
    { "HIGH", 'H'},
    { "LOW", 'L'},
    { NULL, 0},
};

/* PWM Frequency in Hz. Index is argument to D F command. */
double cgr101_pwm_freq[] = {
    72000.0,
    36000.0,
    9000.0,
    4500.0,
    1125.0,
    564.0,
    281.0,
    141.0,
    70.0,
    35.0,
    0.0,
    0.0, /* end-of-list sentinel */
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
    size_t rcv_data_len;
    const char *rcv_data_ptr;
    char err_data[ERR_MAX];
    int sent;
    /* Digital Data Input */
    enum cgr101_digital_read_state digital_read_state;
    int digital_read_requested;
    int digital_read_data;
    /* Device Error Message */
    enum cgr101_error_state error_state;
    int error_msg_len;
    char error_msg[E_MAX];
    /* Digital Data Output */
    int digital_write_data;
    /* Waveform */
    struct {
        enum cgr101_waveform_shape shape;
        double user[WAVEFORM_USER_MAX];
        double frequency;
        double amplitude;
    } waveform;
    /* Oscilloscope */
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
        unsigned int addr;         /* offset of first data capture. */
        enum cgr101_scope_data_state data_state;
        int data_count;
        int output_pending;
        long output_mask;
        struct {
            double input_low;
            double input_high;
            double input_midpoint;
            double input_ptp;
            int input_low_range; /* derived from above */
            double offset_low;
            double offset_high;
            int enable;
            int data[SCOPE_NUM_SAMPLE];
        } channel[SCOPE_NUM_CHAN];
    } scope;
    struct {
        long chan_mask;
        char mode;
        long current;
        long max;
        struct digital_event *data;
        int output_pending;
    } event;
    struct {
        double frequency;
        double duty_cycle;
    } pwm;
};

struct cgr101_w_interp {
    size_t len;
    double *data;
};

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

    if (result) {
        info->device->sent = 1;
    }
    /*
     * Seems that we need to be gentle, otherwise one gets various
     * error responses if commands are sent too quickly back to back.
     *
     * Start with a simple delay. 10000uS seems to be the minimum.
     */

    usleep(10000);

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
 * Digital Event Support
 */

static char cgr101_digital_event_mode(const char *int_sel)
{
    struct cgr101_event_mode_map_s *p;
    char mode = 0;

    assert(int_sel);
    for (p = cgr101_event_mode_map; p != NULL; p++) {
        if (!strcasecmp(int_sel, p->name)) {
            mode = p->mode;
            break;
        }
    }
    assert(p != NULL);

    return mode;
}

static void cgr101_digital_event_pending(struct info *info)
{
    info->block_input = 1;
    info->device->event.output_pending = 1;
}

static void cgr101_digital_event_output(struct info *info)
{
    long idx;
    double seconds;

    for (idx = 0; idx < info->device->event.current; idx++) {
        /* show timestamp in Unix Epoch Time in seconds. */
        seconds =
            (double)info->device->event.data[idx].tv.tv_sec +
            ((double)info->device->event.data[idx].tv.tv_usec/1000000.0);
        scpi_output_fp(info->output, seconds);
        scpi_output_int(info->output, info->device->event.data[idx].data);
    }
}

static void cgr101_interrupt_mode(struct info *info)
{
    int err;

    err = cgr101_device_printf(info, "D ! %c\n", info->device->event.mode);
    assert(!err);
}

static void cgr101_digital_event_done(struct info *info)
{
    info->device->event.chan_mask = 0;
    info->digital_event_status = 0;
    event_send(info->event, EVENT_UNBLOCK);
}

static void cgr101_digital_event(struct info *info, int data)
{
    struct timeval tv;
    int err;
    long current = info->device->event.current;
    assert(current < info->device->event.max);
    assert(data >= 0);
    assert(data <= DIGITAL_EVENT_INTERRUPT);
    assert(info->device->event.data);

    err = gettimeofday(&tv, NULL);
    assert(!err);

    info->device->event.data[current].tv = tv;
    if (data == DIGITAL_EVENT_INTERRUPT) {
        info->device->event.data[current].data = DIGITAL_EVENT_INTERRUPT;
    } else {
        info->device->event.data[current].data = data;
    }
    current++;
    info->device->event.current = current;
    if (current == info->device->event.max) {
        if (info->device->event.output_pending) {
            cgr101_digital_event_output(info);
        }
        cgr101_digital_event_done(info);
    } else if (data == DIGITAL_EVENT_INTERRUPT) {
        cgr101_interrupt_mode(info); /* Re-arm */
    }
}

static void cgr101_digital_auto_update(struct info *info)
{
    int err;

    err = cgr101_device_printf(info, "D A\n");
    assert(!err);
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
        info->device->identify_state = STATE_IDENTIFY_PENDING;
        /* Variable length: terminated by <cr><lf> */
        err = 0;
        break;
    case 'I':
        info->device->rcv_state = DIGITAL_READ;
        info->device->digital_read_state = STATE_DIGITAL_READ_PENDING;
        /* I<uint8_t> */
        err = 0;
        break;
    case 'O':
        info->device->rcv_state = SCOPE_OFFSET;
        info->device->scope.offset_state = STATE_SCOPE_OFFSET_EXPECT_A_HIGH;
        /* O<int8_t>*4 */
        err = 0;
        break;
    case 'S':
        info->device->rcv_state = SCOPE_STATUS;
        info->device->scope.status_state = STATE_SCOPE_STATUS_PENDING;
        /* 'Status N' */
        err = 0;
        break;
    case 'A':
        info->device->rcv_state = SCOPE_ADDR;
        info->device->scope.addr_state = STATE_SCOPE_ADDR_EXPECT_HIGH;
        info->device->scope.addr = 0;
        /* O<int8_t>*2 */
        err = 0;
        break;
    case 'D':
        info->device->rcv_state = SCOPE_DATA;
        info->device->scope.data_state = STATE_SCOPE_DATA_EXPECT_A_HIGH;
        info->device->scope.data_count = 0;
        /* 'DA1a1B1b2A2a2B2b2...' */
        err = 0;
        break;
    case 'E':
        info->device->rcv_state = ERROR_MSG;
        /* Variable length: terminated by <cr><lf> */
        info->device->error_state = STATE_ERROR_PENDING;
        info->device->error_msg_len = 0;
        /* Save the 'E' in 'Error...' */
        info->device->error_msg[info->device->error_msg_len++] = c;
        err = 0;
        break;
    case '!':
        /* '!\r\n' */
        info->device->rcv_state = INTERRUPT_MSG;
        err = 0;
        break;
    default:
        /* ...liberal in what we receive. */
        err = 0;
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
    event_send(info->event, EVENT_OUTPUT_FLUSH);
}

static int cgr101_digital_read_start(struct info *info)
{
    int err = cgr101_device_send(info, "D I\n");

    return err;
}

static int cgr101_rcv_digital_read(struct info *info, char c)
{
    int err = 0;

    /* Done receiving. */
    info->device->digital_read_data = (unsigned char)c;
    if (info->device->event.chan_mask & DIGITAL_DATA_CHANNEL_MASK) {
        cgr101_digital_event(info, info->device->digital_read_data);
    }
    event_send(info->event, EVENT_DIGITAL_READ_COMPLETE);
    cgr101_rcv_idle(info);

    return err;
}

static void cgr101_digital_read_completion(void *arg)
{
    struct info *info = arg;

    /* By default, the device sends updates when the inputs change, so
     * completion may happen when in the PENDING state *or* the
     * COMPLETION state.
     */
    info->device->digital_read_state = STATE_DIGITAL_READ_COMPLETE;
}

static void cgr101_digital_read_output(void *arg)
{
    struct info *info = arg;

    switch (info->device->digital_read_state) {
    case STATE_DIGITAL_READ_IDLE:
    case STATE_DIGITAL_READ_PENDING:
        /* Reschedule */
        event_send(info->event, EVENT_DIGITAL_READ_OUTPUT);
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
 *
 * Offsets are stored as 8 bit unsigned values with a midpoint at 128.
 * They are converted here to voltages so they can be directly applied
 * to the scope data conversion. This also facilitates external
 * applications adjusting the offsets. This approach is different that
 * the Syscomp scope.tcl application but is computationaly the same
 * (see cea.c for the error analysis).
 */


/* Convert an ADC value from an unsigned biased (at midpoint)
 * format to a signed integer roughly += midpoint/2. Note that the
 * forward and reverse transformation is the same in this case.
 */
static int cgr101_adc2c(int midpoint, int value)
{
    int result = (midpoint - value);

    return result;
}

/* raw data to value */
static double cgr101_digitizer_d2v(int data,
                                   int midpoint,
                                   double step,
                                   double offset)
{
    return ((double)cgr101_adc2c(midpoint, data) * step) - offset;
}

/* value to raw data */
static int cgr101_digitizer_v2d(double value,
                                int midpoint,
                                double step,
                                double offset)
{
    return cgr101_adc2c(midpoint, (int)round((value + offset) / step));
}

static int cgr101_rcv_scope_offset(struct info *info, char c)
{
    int err = 0;
    int data = (unsigned char)c;
    double value;

    switch (info->device->scope.offset_state) {
    case STATE_SCOPE_OFFSET_EXPECT_A_HIGH:
        value = cgr101_digitizer_d2v(data, MP8, STEP_HIGH, 0.0);
        info->device->scope.channel[0].offset_high = value;
        info->device->scope.offset_state = STATE_SCOPE_OFFSET_EXPECT_A_LOW;
        break;
    case STATE_SCOPE_OFFSET_EXPECT_A_LOW:
        value = cgr101_digitizer_d2v(data, MP8, STEP_LOW, 0.0);
        info->device->scope.channel[0].offset_low = value;
        info->device->scope.offset_state = STATE_SCOPE_OFFSET_EXPECT_B_HIGH;
        break;
    case STATE_SCOPE_OFFSET_EXPECT_B_HIGH:
        value = cgr101_digitizer_d2v(data, MP8, STEP_HIGH, 0.0);
        info->device->scope.channel[1].offset_high = value;
        info->device->scope.offset_state = STATE_SCOPE_OFFSET_EXPECT_B_LOW;
        break;
    case STATE_SCOPE_OFFSET_EXPECT_B_LOW:
        value = cgr101_digitizer_d2v(data, MP8, STEP_LOW, 0.0);
        info->device->scope.channel[1].offset_low = value;
        info->device->scope.offset_state = STATE_SCOPE_OFFSET_COMPLETE;
        info->offset_status = 0;
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
            event_send(info->event, EVENT_SCOPE_STATUS_COMPLETE);

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

static void cgr101_scope_status_output(void *arg)
{
    struct info *info = arg;

    switch (info->device->scope.status_state) {
    case STATE_SCOPE_STATUS_IDLE:
    {
        int err = cgr101_scope_status_start(info);
        assert(!err);
    }
        break;
    case STATE_SCOPE_STATUS_PENDING:
        /* Reschedule */
        event_send(info->event, EVENT_SCOPE_STATUS_OUTPUT);
        break;
    case STATE_SCOPE_STATUS_COMPLETE:
        scpi_output_int(info->output, info->device->scope.status);
        break;
    default:
        assert(0);
        break;
    }
}

static void cgr101_scope_status_completion(void *arg)
{
    struct info *info = arg;

    event_send(info->event, EVENT_SCOPE_STATUS_OUTPUT);
}

/*
 * Oscilloscope Offset Handling
 */

static void cgr101_scope_offset_start(void *arg)
{
    struct info *info = arg;
    int err;

    err = cgr101_device_send(info, "S O\n"); /* Get offsets. */
    assert(!err);
}

/*
 * Oscilloscope Data Handling
 */

static void cgr101_digitizer_step_offset(struct info *info,
                                         int chan,
                                         int low_range,
                                         double *step,
                                         double *offset)
{
    assert(chan >= 0);
    assert(chan < SCOPE_NUM_CHAN);

    if (low_range) {
        *offset = info->device->scope.channel[chan].offset_low;
        *step = STEP_LOW;
    } else {
        *offset = info->device->scope.channel[chan].offset_high;
        *step = STEP_HIGH;
    }
}

/* Convert an internal integral representation to a display voltage. */
static double cgr101_digitizer_data_to_voltage(struct info *info,
                                               int chan,
                                               int low_range,
                                               int midpoint,
                                               int data)
{
    double offset;
    double step;
    double value;

    cgr101_digitizer_step_offset(info, chan, low_range, &step, &offset);

    value = cgr101_digitizer_d2v(data, midpoint, step, offset);

    return value;
}

/* Convert a display voltage value to internal integral representation. */
static int cgr101_digitizer_voltage_to_data(struct info *info,
                                            int chan,
                                            int low_range,
                                            int midpoint,
                                            double value)
{
    double offset;
    double step;
    int data;

    cgr101_digitizer_step_offset(info, chan, low_range, &step, &offset);
    data = cgr101_digitizer_v2d(value, midpoint, step, offset);

    return data;
}


static double cgr101_digitizer_value(struct info *info,
                                     int chan,
                                     unsigned int idx)
{
    int data;
    double value;
    int range;

    assert(idx < SCOPE_NUM_SAMPLE);

    range = info->device->scope.channel[chan].input_low_range;
    data = info->device->scope.channel[chan].data[idx];

    value = cgr101_digitizer_data_to_voltage(info, chan, range, MP10, data);

    return value;
}

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

        /* Update control */
        cgr101_digitizer_update_control(info);

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

        info->overlapped = 1;
        info->sweep_status = 1;

    } while (0);

    return err;
}

static int cgr101_rcv_scope_addr(struct info *info, char c)
{
    int err = 0;
    unsigned char uc = (unsigned char)c;

    switch (info->device->scope.addr_state) {
    case STATE_SCOPE_ADDR_EXPECT_HIGH:
        assert(c >= 0);
        assert(c <= 3);
        info->device->scope.addr = (unsigned int)(uc<<8);
        assert(info->device->scope.addr < SCOPE_NUM_SAMPLE);
        info->device->scope.addr_state = STATE_SCOPE_ADDR_EXPECT_LOW;
        break;
    case STATE_SCOPE_ADDR_EXPECT_LOW:
        info->device->scope.addr |= (unsigned int)(uc);
        assert(info->device->scope.addr < SCOPE_NUM_SAMPLE);
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

static void cgr101_digitizer_data_output(struct info *info, long chan_mask)
{
    int chan;
    unsigned int j;
    unsigned int idx;
    double data;

    assert(info->device->scope.addr_state == STATE_SCOPE_ADDR_COMPLETE);
    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        if (!(chan_mask & 1<<chan)) {
            continue;
        }
        for (j=0; j<SCOPE_NUM_SAMPLE; j++) {
            /* scope.addr is where the capture *ended*, so the start
             * is just past that point.
             */
            idx = info->device->scope.addr + 1;
            idx += j;
            if (idx >= SCOPE_NUM_SAMPLE) {
                /* wrapped */
                idx -= SCOPE_NUM_SAMPLE;
            }
            data = cgr101_digitizer_value(info, chan, idx);
            scpi_output_fp(info->output, data);
        }
    }
}

static void cgr101_scope_data_output(struct info *info)
{
    assert(info->device->scope.output_pending);
    cgr101_digitizer_data_output(info,info->device->scope.output_mask);
    info->device->scope.output_pending = 0;
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
        info->device->scope.channel[chan].data[idx] =
            (int)((unsigned char)c<<8);
    } else {
        info->device->scope.channel[chan].data[idx] +=
            (int)(unsigned char)c;
    }
}

static void cgr101_scope_data_done(struct info *info,
                                   enum cgr101_scope_data_state state)
{
    assert(info->device->scope.addr_state == STATE_SCOPE_ADDR_COMPLETE);
    info->device->scope.data_state = state;
    info->overlapped = 0;
    info->sweep_status = 0;
    event_send(info->event, EVENT_UNBLOCK);
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
            if (info->device->scope.output_pending) {
                cgr101_scope_data_output(info);
            }
            cgr101_scope_data_done(info, STATE_SCOPE_DATA_COMPLETE);
            /* Done receiving. */
            cgr101_rcv_idle(info);
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
 * Device Error handler
 */

static int cgr101_rcv_error_msg(struct info *info, char c)
{
    int err = 0;

    if (c == '\n') {
        /* Done receiving. */
        info->device->error_msg[info->device->error_msg_len++] = 0;
        scpi_error(info->error,
                   SCPI_ERR_HARDWARE_ERROR,
                   info->device->error_msg);
        cgr101_rcv_idle(info);
    } else if (c != '\r') {
        info->device->error_msg[info->device->error_msg_len++] = c;
    }

    return err;
}

/*
 * Device Interrupt handler
 */

static int cgr101_rcv_interrupt_msg(struct info *info, char c)
{
    int err = 0;

    switch (c) {
    case '\n':
        /* Done receiving. */
        if (info->device->event.chan_mask & INTERRUPT_CHANNEL_MASK) {
            cgr101_digital_event(info, DIGITAL_EVENT_INTERRUPT);
        }
        cgr101_rcv_idle(info);
    case '\r':
    case '!':
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
    int err = cgr101_device_send(info, "i\n");

    return err;
}

static void cgr101_identify_done(struct info *info)
{
    event_send(info->event, EVENT_IDENTIFY_COMPLETE);
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
    event_send(info->event, EVENT_IDENTIFY_OUTPUT);
    return 0;
}

static void cgr101_identify_completion(void *arg)
{
    struct info *info = arg;

    assert(info->device->identify_state == STATE_IDENTIFY_PENDING);
    info->device->identify_state = STATE_IDENTIFY_COMPLETE;
    if (info->device->identify_output_requested) {
        event_send(info->event, EVENT_IDENTIFY_OUTPUT);
    }
}

static void cgr101_identify_output(void *arg)
{
    struct info *info = arg;

    switch (info->device->identify_state) {
    case STATE_IDENTIFY_IDLE:
    {
        int err = cgr101_identify_start(info);
        assert(!err);
    }
        break;
    case STATE_IDENTIFY_PENDING:
        /* Reschedule */
        event_send(info->event, EVENT_IDENTIFY_OUTPUT);
        break;
    case STATE_IDENTIFY_COMPLETE:
        scpi_output_printf(info->output,
                           "GMP,CGR101-SCPI,1.0,%s",
                           info->device->device_id);
        event_send(info->event, EVENT_OUTPUT_FLUSH);
        break;
    default:
        assert(0);
        break;
    }
}

/*
 * Event Registration
 */

static void cgr101_event_init(struct info *info)
{
    int err;

    err = event_add(info->event,
                    EVENT_IDENTIFY_COMPLETE,
                    cgr101_identify_completion,
                    info);
    assert(!err);

    err = event_add(info->event,
                    EVENT_IDENTIFY_OUTPUT,
                    cgr101_identify_output,
                    info);
    assert(!err);

    err = event_add(info->event,
                    EVENT_DIGITAL_READ_COMPLETE,
                    cgr101_digital_read_completion,
                    info);
    assert(!err);

    err = event_add(info->event,
                    EVENT_DIGITAL_READ_OUTPUT,
                    cgr101_digital_read_output,
                    info);
    assert(!err);

    err = event_add(info->event,
                    EVENT_SCOPE_STATUS_COMPLETE,
                    cgr101_scope_status_completion,
                    info);
    assert(!err);

    err = event_add(info->event,
                    EVENT_SCOPE_STATUS_OUTPUT,
                    cgr101_scope_status_output,
                    info);
    assert(!err);

    err = event_add(info->event,
                    EVENT_SCOPE_OFFSET_START,
                    cgr101_scope_offset_start,
                    info);
    assert(!err);

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
    case ERROR_MSG:
        err = cgr101_rcv_error_msg(info, c);
        break;
    case INTERRUPT_MSG:
        err = cgr101_rcv_interrupt_msg(info, c);
        break;
    default:
        assert(0);
    }

    assert(!err); /*FIXME: DEBUG*/
    return err;
}

static int cgr101_rcv_data(struct info *info, const char *buf, size_t len)
{
    int err = 1;

    while (len--) {
        info->device->rcv_data_ptr = buf; /* For rcv debug */
        err = cgr101_rcv_sm(info, *buf++);
        if (err) {
            assert(0); /*FIXME: DEBUG*/

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
            assert(0); /*FIXME: DEBUG*/
            err = 1;
        }
    } else if (len != 0 && info->device->sent) {
        /* Only receive data once a command has been sent to flush any
         pending stale data. */
        info->device->rcv_data_len = (size_t)len;
        err = cgr101_rcv_data(info,
                              info->device->rcv_data,
                              info->device->rcv_data_len);
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
        if (!strcasecmp(value, p->name)) {
            *shape = p->shape;
            break;
        }
    }
    assert(p != NULL);
}

static const char *cgr101_waveform_name(enum cgr101_waveform_shape shape)
{
    struct cgr101_waveform_map_s *p;
    const char *name = NULL;

    for (p = cgr101_waveform_map; p != NULL; p++) {
        if (shape == p->shape) {
            name = p->name;
            break;
        }
    }
    assert(name != NULL);

    return name;
}

/*
 * Waveform functions:
 *  phase: 0.0 .. 1.0
 *  amplitude: -1.0 .. 1.0
 */

static double cgr101_fn_sin(double phase, void *arg)
{
    double angle = 2.0 * M_PI * phase;
    (void)arg;
    return ((double)sin(angle));
}


static double cgr101_fn_squ(double phase, void *arg)
{
    (void)arg;
    return (double)((phase < 0.5) ? 1.0 : -1.0);
}

static double cgr101_fn_tri(double phase, void *arg)
{
    (void)arg;
    phase *= 4.0;
    return (phase < 2.0) ? (phase - 1.0) : (3.0 - phase);
}

static double cgr101_fn_const(double phase, void *arg)
{
    double value = *(double *)arg;
    (void)phase;
    return (double)value;
}

/*
 * Interpolate a set of points. 'phase' ranges from 0.0 to (almost) 1.0.
 */
static double cgr101_fn_interp(double phase, void *arg)
{
    struct cgr101_w_interp *interp = (struct cgr101_w_interp  *)arg;
    double value = 0.0;
    double in_phase;
    double start_phase;
    double end_phase;
    double interp_phase;
    double delta;
    double v1;
    double v2;
    size_t start_idx;
    size_t end_idx;
    size_t segments;

    assert(interp);
    assert(interp->data);
    assert(interp->len > 1);
    assert(interp->len <= WAVEFORM_USER_MAX);
    /* Number of line segments to use. */
    segments = interp->len - 1;
    in_phase = phase * (double)segments;
    /* Indexes for starting and ending interpolation points */
    start_phase = floor(in_phase);
    end_phase = start_phase + 1.0;
    start_idx = (size_t)start_phase;
    end_idx = (size_t)end_phase;
    assert(start_idx < segments);
    assert(end_idx < interp->len);
    assert(end_idx > start_idx);
    interp_phase = in_phase - start_phase;
    assert(interp_phase >= 0);
    assert(interp_phase < 1.0);

    /* Get the endpoints of the interpolation segment. */
    v1 = interp->data[start_idx];
    v2 = interp->data[end_idx];
    /* Calculate the interpolated value. */
    delta = v2 - v1;
    value = v1 + delta*interp_phase;

    return value;
}

typedef double (*wff_t)(double phase, void *arg);

static void cgr101_waveform_create_fn(double *data,
                                      size_t points,
                                      wff_t wfn,
                                      void *arg)
{
    size_t i;
    double phase = 0.0;
    double phase_incr = 1.0/((double)points);
    for (i=0; i<points; i++) {
        data[i] = (double)wfn(phase, arg);
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
                                  COUNT_OF(info->device->waveform.user),
                                  cgr101_fn_sin,
                                  NULL);
        break;
    case WAV_SQU:
        cgr101_waveform_create_fn(info->device->waveform.user,
                                  COUNT_OF(info->device->waveform.user),
                                  cgr101_fn_squ,
                                  NULL);
        break;
    case WAV_TRI:
        cgr101_waveform_create_fn(info->device->waveform.user,
                                  COUNT_OF(info->device->waveform.user),
                                  cgr101_fn_tri,
                                  NULL);
        break;
    case WAV_NONE:
        /* Nothing to do. */
        break;
    default:
        assert(0);
        break;
    }

    /* FIXME: only set if no error. */
    info->device->waveform.shape = shape;

}

static int cgr101_waveform_conv(double value)
{
    return (255 - (int)ceil((value * 127.5) + 127.0));
}

static void cgr101_waveform_program(struct info *info)
{
    int err;
    size_t i;
    double f;
    int val;

    if (info->device->waveform.shape == WAV_RAND) {
        err = cgr101_device_printf(info, "W N\n");
        assert(!err);
    } else {
        for (i=0; i<COUNT_OF(info->device->waveform.user); i++) {
            f = info->device->waveform.user[i];
            val = cgr101_waveform_conv(f);
            assert(val >= 0);
            assert(val <= 255);
            err = cgr101_device_printf(info, "W S %zu %d\n", i, val);
            assert(!err);
        }
        err = cgr101_device_printf(info, "W P\n");
        assert(!err);
    }
}

static void cgr101_waveform_user(struct info *info,
                                 size_t len,
                                 double *data)
{
    if (len == 1) {
        cgr101_waveform_create_fn(info->device->waveform.user,
                                  COUNT_OF(info->device->waveform.user),
                                  cgr101_fn_const,
                                  data);
    } else if (len <= COUNT_OF(info->device->waveform.user)) {
        struct cgr101_w_interp interp = {
            .len = len,
            .data = data
        };
        cgr101_waveform_create_fn(info->device->waveform.user,
                                  COUNT_OF(info->device->waveform.user),
                                  cgr101_fn_interp,
                                  &interp);
    } else {
        /* Handle this case someday or return an error. */
        assert(0);
    }
}

static void cgr101_waveform_userq(struct info *info)
{
    size_t i;

    for (i=0; i<COUNT_OF(info->device->waveform.user); i++) {
        scpi_output_fp(info->output, info->device->waveform.user[i]);
    }
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
    info->device->scope.channel[chan].input_low_range = low_range;
    err = cgr101_device_printf(info, "S P %c\n",
                               cgr101_range_cmd[chan][low_range]);
    assert(!err);
}

static int cgr101_sweep_time(struct info *info, double time)
{
    double sweep_time = SCOPE_MIN_SWEEP_TIME;
    double sweep_time2 = sweep_time * 2.0;
    int sweep_div;
    int err = 1;

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
        err = 0;
    }

    return err;
}

static void cgr101_digitizer_data_pending(struct info *info, long chan_mask)
{
    info->block_input = 1;
    info->device->scope.output_pending = 1;
    info->device->scope.output_mask = chan_mask;
}


/*
 * Initialization
 */

static void cgr101_device_reset(struct info *info)
{
    int chan;
    double midpoint = (double)SCOPE_NUM_SAMPLE/2;
    int err;

    /*Consistency check*/
    assert(COUNT_OF(info->device->scope.channel) == SCOPE_NUM_CHAN);
    /* Set Defaults */
    info->device->scope.trigger_offset = 0;
    info->device->scope.trigger_ref = midpoint;
    err = cgr101_sweep_time(info, SCOPE_MIN_SWEEP_TIME);
    assert(!err);
    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        info->device->scope.channel[chan].enable = 0;
        info->device->scope.channel[chan].input_low = SCOPE_DEFAULT_LOW;
        info->device->scope.channel[chan].input_high = SCOPE_DEFAULT_HIGH;
        info->device->scope.channel[chan].input_midpoint =
            SCOPE_DEFAULT_MIDPOINT;
        info->device->scope.channel[chan].input_ptp = SCOPE_DEFAULT_PTP;
        /* Force */
        info->device->scope.channel[chan].input_low_range = 0;
    }
}

static void cgr101_device_init(struct info *info)
{
    int chan;

    cgr101_device_reset(info);
    for (chan=0; chan<SCOPE_NUM_CHAN; chan++) {
        cgr101_digitizer_set_range(info, chan);
    }

    /* Query device for offsets. Send as an event to allow any stale
     data still being sent by the device to be flushed. */
    if (info->device->scope.offset_state != STATE_SCOPE_OFFSET_EXPECT_A_HIGH) {
        event_send(info->event, EVENT_SCOPE_OFFSET_START);
    }
    info->offset_status = 1; /* Cleared when all offsets received. */
}

/*
 * PWM Programming
 */

static void cgr101_pwm_duty_cycle(struct info *info, double value)
{
    int err;
    int n;

    n = (int)floor(value*255.0);
    info->device->pwm.duty_cycle = (double)n/255.0;
    err = cgr101_device_printf(info, "D D %d\n", n);
    assert(!err);
}

static void cgr101_pwm_frequency(struct info *info, double value)
{
    int err;
    size_t idx;
    size_t last;
    double f1;
    double f2;

    /* Don't include sentinel in iteration. */
    last = COUNT_OF(cgr101_pwm_freq)-1;
    for (idx=0; idx < last; idx++) {
        f1 = cgr101_pwm_freq[idx];
        f2 = cgr101_pwm_freq[idx+1];
        if (value <= f1 && value > f2) {
            info->device->pwm.frequency = f1;
            break;
        }
    }
    err = cgr101_device_printf(info, "D F %zu\n", idx);
    assert(!err);

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

        cgr101_event_init(info);

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

    if (info->device->event.chan_mask & INTERRUPT_CHANNEL_MASK) {
        cgr101_interrupt_mode(info);
    }

    if (info->device->event.chan_mask & DIGITAL_DATA_CHANNEL_MASK) {
        cgr101_digital_auto_update(info);
    }

    if ((info->device->event.chan_mask) != 0) {
        info->digital_event_status = 1;
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
    event_send(info->event, EVENT_DIGITAL_READ_OUTPUT);
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

void cgr101_source_waveform_level(struct info *info, double value)
{
    int err;
    int amp;

    if (value >= 0.0 && value <= 1.0) {
        info->device->waveform.amplitude = value;
        amp = (int)floor(value * 255.0);
        assert(amp >= 0);
        assert(amp <= 255);
        err = cgr101_device_printf(info, "W A %d\n", amp);
        assert(!err);
    } else {
        /* Range error */
    }
}

void cgr101_source_waveform_levelq(struct info *info)
{
    scpi_output_fp(info->output, info->device->waveform.amplitude);
}

void cgr101_source_waveform_function(struct info *info,
                                     const char *value)
{
    enum cgr101_waveform_shape shape;

    cgr101_waveform_lookup(value, &shape);
    if (shape != WAV_NONE) {
        cgr101_waveform_create(info, shape);
        cgr101_waveform_program(info);
    }
}

void cgr101_source_waveform_functionq(struct info *info)
{
    const char *name;

    name = cgr101_waveform_name(info->device->waveform.shape);
    assert(name);
    scpi_output_str(info->output, name);
}

void cgr101_source_waveform_user(struct info *info, size_t len, double *data)
{
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
    cgr101_pwm_duty_cycle(info, value);
}

void cgr101_source_pwm_duty_cycleq(struct info *info)
{
    scpi_output_fp(info->output, info->device->pwm.duty_cycle);
}

void cgr101_source_pwm_frequency(struct info *info, double value)
{
    cgr101_pwm_frequency(info, value);
}

void cgr101_source_pwm_frequencyq(struct info *info)
{
    scpi_output_fp(info->output, info->device->pwm.frequency);
}

void cgr101_digitizer_coupling(struct info *info, const char *value)
{
    (void)info;
    (void)value;
}

void cgr101_digitizer_dataq(struct info *info, long chan_mask)
{
    if (info->sweep_status) {
        cgr101_digitizer_data_pending(info, chan_mask);
    } else {
        cgr101_digitizer_data_output(info, chan_mask);
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
    if (cgr101_sweep_time(info, value)) {
        /* some error */
    }
}

void cgr101_digitizer_sweep_timeq(struct info *info)
{
    double value = info->device->scope.sweep_time;
    scpi_output_fp(info->output, value);

}

void cgr101_digitizer_sweep_interval(struct info *info, double value)
{
    if (cgr101_sweep_time(info, value * SCOPE_NUM_SAMPLE)) {
        /* some error */
    }
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
    event_send(info->event, EVENT_SCOPE_STATUS_OUTPUT);
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

void cgr101_digitizer_input_offset(struct info *info,
                                   double f1,
                                   double f2,
                                   double f3,
                                   double f4)
{
    info->device->scope.channel[0].offset_low  = f1;
    info->device->scope.channel[0].offset_high = f2;
    info->device->scope.channel[1].offset_low  = f3;
    info->device->scope.channel[1].offset_high = f4;
}

void cgr101_digitizer_input_offset_store(struct info *info)
{
    int v1;
    int v2;
    int v3;
    int v4;
    int err;

    v1 = cgr101_digitizer_v2d(info->device->scope.channel[0].offset_high,
                              MP8,
                              STEP_HIGH,
                              0.0);
    v2 = cgr101_digitizer_v2d(info->device->scope.channel[0].offset_low,
                              MP8,
                              STEP_LOW,
                              0.0);
    v3 = cgr101_digitizer_v2d(info->device->scope.channel[1].offset_high,
                              MP8,
                              STEP_HIGH,
                              0.0);
    v4 = cgr101_digitizer_v2d(info->device->scope.channel[1].offset_low,
                              MP8,
                              STEP_LOW,
                              0.0);

    if (info->enable_flash_writes) {
        err = cgr101_device_printf(info, "S F %d %d %d %d\n", v1, v2, v3, v4);
        assert(!err);
    }

    if (info->verbose) {
        fprintf(stderr, "Flash Write: %d %d %d %d\n", v1, v2, v3, v4);
    }
}

void cgr101_digitizer_input_offsetq(struct info *info)
{
    scpi_output_fp(info->output, info->device->scope.channel[0].offset_low);
    scpi_output_fp(info->output, info->device->scope.channel[0].offset_high);
    scpi_output_fp(info->output, info->device->scope.channel[1].offset_low);
    scpi_output_fp(info->output, info->device->scope.channel[1].offset_high);
}

void cgr101_trigger_coupling(struct info *info, const char *value)
{
    /* Ignore for now. */
    (void)info;
    (void)value;
}

void cgr101_trigger_level(struct info *info, double value)
{
    int trigger_value;
    int chan;
    int range;
    int err;

    info->device->scope.trigger_level = value;
    if (info->device->scope.trigger_external) {
        chan = 1;
    } else {
        chan = info->device->scope.internal_trigger_source;
    }

    range = info->device->scope.channel[chan].input_low_range;
    trigger_value = cgr101_digitizer_voltage_to_data(info,
                                                     chan,
                                                     range,
                                                     MP10,
                                                     value);
    assert(trigger_value >= 0);
    assert(trigger_value < 1024);
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
    if (!strcasecmp(value, "POS")) {
        info->device->scope.trigger_polarity = 0;
    } else if (!strcasecmp(value, "NEG")) {
        info->device->scope.trigger_polarity = 1;
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
    if (!strcasecmp(value, "IMM")) {
        info->device->scope.trigger_source = SCOPE_TRIGGER_SOURCE_IMM;
    } else if (!strcasecmp(value, "INT")) {
        info->device->scope.trigger_source = SCOPE_TRIGGER_SOURCE_INT;
    } else if (!strcasecmp(value, "EXT")) {
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

void cgr101_rst(struct info *info)
{
    cgr101_device_reset(info);
}

void cgr101_abort(struct info *info)
{
    /* Scope */
    if (info->sweep_status) {
        cgr101_scope_data_done(info, STATE_SCOPE_DATA_IDLE);
    }

    /* Digital Event */
    if (info->digital_event_status) {
        cgr101_digital_event_done(info);
    }
}

void cgr101_configure_digital_event(struct info *info,
                                    const char *int_sel,
                                    long count,
                                    long chan_mask)
{
    char mode = 0;
    size_t data_size;

    if ((chan_mask & INTERRUPT_CHANNEL_MASK) != 0) {
        assert(int_sel);
        mode = cgr101_digital_event_mode(int_sel);
    } else {
        mode = cgr101_digital_event_mode("NONE");
    }

    /* Error if bogus channels */

    data_size = (size_t)count * sizeof(info->device->event.data[0]);

    info->device->event.chan_mask = chan_mask;
    info->device->event.mode = mode;
    info->device->event.current = 0;
    info->device->event.max = count;
    info->device->event.output_pending = 0;
    info->device->event.data = realloc(info->device->event.data,data_size);
    assert(info->device->event.data);
}

void cgr101_fetch_digital_event(struct info *info)
{
    if (info->digital_event_status) {
        cgr101_digital_event_pending(info);
    } else {
        cgr101_digital_event_output(info);
    }
}
