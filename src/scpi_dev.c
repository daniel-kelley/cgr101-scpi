/*
   scpi_dev.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "scpi.h"
#include "scpi_core.h"
#include "scpi_input.h"
#include "cgr101.h"

int scpi_dev_abort(struct info *info)
{
    cgr101_abort(info);

    return 0;
}

int scpi_dev_initiate(struct info *info)
{
    return cgr101_initiate(info);
}

void scpi_dev_initiate_immediate(struct info *info)
{
    cgr101_initiate_immediate(info);
}

struct scpi_type *scpi_dev_channel_num(struct info *info,
                                       struct scpi_type *val)
{
    (void)info;

    if (val->type == SCPI_TYPE_INT &&
        val->val.ival >= CGR101_MIN_CHAN &&
        val->val.ival <= CGR101_MAX_CHAN)
    {
        val->val.chan = 1<<(val->val.ival-1);
        val->type = SCPI_TYPE_CHAN;
    } else {
        /*scpi_error(info->scpi, SCPI_ERR_DATA_OUT_OF_RANGE, val->src);*/
    }
    return val;
}

struct scpi_type *scpi_dev_channel_range(struct info *info,
                                         struct scpi_type *v1,
                                         struct scpi_type *v2)
{
    long chan_start;
    long chan_end;
    (void)info;

    assert(v1->type == SCPI_TYPE_CHAN);
    assert(v2->type == SCPI_TYPE_CHAN);

    if (v1->val.chan < v2->val.chan) {
        chan_start = v1->val.chan;
        chan_end   = v2->val.chan;
    } else {
        chan_start = v2->val.chan;
        chan_end   = v1->val.chan;
    }

    assert(chan_start != 0);
    assert(chan_end != 0);
    assert(chan_start < chan_end);
    /* Set a range of channels */
    while (chan_start != chan_end) {
        v1->val.chan |= chan_start;
        chan_start <<= 1;
        assert(chan_start != 0);
    }

    return v1;
}

struct scpi_type *scpi_dev_channel_range_append(struct info *info,
                                                struct scpi_type *v1,
                                                struct scpi_type *v2)
{
    (void)info;

    assert(v1->type == SCPI_TYPE_CHAN);
    assert(v2->type == SCPI_TYPE_CHAN);

    v1->val.chan |= v2->val.chan;

    return v1;
}

static int scpi_dev_chan(struct scpi_type *v, long *chan_mask)
{
    assert(v->type == SCPI_TYPE_CHAN);

    *chan_mask = v->val.chan;

    return 0;
}

void scpi_dev_read_digital_dataq(struct info *info)
{
    do {
        if (scpi_dev_abort(info)) {
            break;
        }
        if (scpi_dev_initiate(info)) {
            break;
        }
        scpi_dev_fetch_digital_dataq(info);
    } while (0);
}

void scpi_dev_measure_digital_dataq(struct info *info)
{
    if (!scpi_dev_conf_digital_data(info)) {
        scpi_dev_read_digital_dataq(info);
    }
}

int scpi_dev_conf_digital_data(struct info *info)
{
    return cgr101_configure_digital_data(info);
}

void scpi_dev_confq(struct info *info)
{
    (void)info;
}

void scpi_dev_fetch_digital_dataq(struct info *info)
{
    if (cgr101_digital_data_configured(info)) {
        cgr101_fetch_digital_data(info);
    } else {
        /*scpi_error(info->scpi, SCPI_ERR_UNAVAIL???, val->src);*/
    }
}

void scpi_dev_input_coupling(struct info *info, struct scpi_type *v)
{
    const char *value;

    if (!scpi_input_str(info, v, &value)) {
        cgr101_digitizer_coupling(info, value);
    }
}

void scpi_dev_sense_dataq(struct info *info, struct scpi_type *v)
{
    long chan_mask;

    if (!scpi_dev_chan(v, &chan_mask)) {
        cgr101_digitizer_dataq(info, chan_mask);
    }
}

void scpi_dev_sense_function_concurrent(struct info *info,
                                        struct scpi_type *v)
{
    int value;

    if (!scpi_input_boolean(info, v, &value)) {
        cgr101_digitizer_concurrent(info, value);
    }
}

void scpi_dev_sense_function_off(struct info *info,
                                 struct scpi_type *v)
{
    long chan_mask;

    if (!scpi_dev_chan(v, &chan_mask)) {
        cgr101_digitizer_channel_state(info, chan_mask, 0);
    }
}

void scpi_dev_sense_function_stateq(struct info *info,
                                    struct scpi_type *v)
{
    long chan_mask;

    if (!scpi_dev_chan(v, &chan_mask)) {
        cgr101_digitizer_channel_stateq(info, chan_mask);
    }
}

void scpi_dev_sense_function_on(struct info *info,
                                struct scpi_type *v)
{
    long chan_mask;

    if (!scpi_dev_chan(v, &chan_mask)) {
        cgr101_digitizer_channel_state(info, chan_mask, 1);
    }
}

void scpi_dev_sense_sweep_pointq(struct info *info)
{
    cgr101_digitizer_sweep_pointq(info);
}

void scpi_dev_sense_sweep_offset_point(struct info *info,
                                       struct scpi_type *v)
{
    double value;

    if (!scpi_input_fp(info, v, &value)) {
        cgr101_digitizer_offset_point(info, value);
    }
}

void scpi_dev_sense_sweep_offset_pointq(struct info *info)
{
    cgr101_digitizer_offset_pointq(info);
}

void scpi_dev_sense_sweep_offset_time(struct info *info,
                                      struct scpi_type *v)
{
    double value;

    if (!scpi_input_fp(info, v, &value)) {
        cgr101_digitizer_offset_time(info, value);
    }
}

void scpi_dev_sense_sweep_offset_timeq(struct info *info)
{
    cgr101_digitizer_offset_timeq(info);
}

void scpi_dev_sense_sweep_oref_loc(struct info *info,
                                   struct scpi_type *v)
{
    double value;

    if (!scpi_input_fp(info, v, &value)) {
        cgr101_digitizer_oref_loc(info, value);
    }
}

void scpi_dev_sense_sweep_oref_locq(struct info *info)
{
    cgr101_digitizer_oref_locq(info);
}

void scpi_dev_sense_sweep_oref_point(struct info *info,
                                     struct scpi_type *v)
{
    double value;

    if (!scpi_input_fp(info, v, &value)) {
        cgr101_digitizer_oref_point(info, value);
    }
}

void scpi_dev_sense_sweep_oref_pointq(struct info *info)
{
    cgr101_digitizer_oref_pointq(info);
}

void scpi_dev_sense_sweep_time(struct info *info, struct scpi_type *v)
{
    double value;

    if (!scpi_input_fp(info, v, &value)) {
        cgr101_digitizer_sweep_time(info, value);
    }
}

void scpi_dev_sense_sweep_timeq(struct info *info)
{
    cgr101_digitizer_sweep_timeq(info);
}

void scpi_dev_sense_sweep_time_interval(struct info *info,
                                               struct scpi_type *v)
{
    double value;

    if (!scpi_input_fp(info, v, &value)) {
        cgr101_digitizer_sweep_interval(info, value);
    }
}

void scpi_dev_sense_sweep_time_intervalq(struct info *info)
{
    cgr101_digitizer_sweep_time_intervalq(info);
}

void scpi_dev_sense_voltage_low(struct info *info,
                                       struct scpi_type *v,
                                       struct scpi_type *chan)
{
    double value;
    long chan_mask;
    int err_v;
    int err_c;

    err_v = scpi_input_fp(info, v, &value);
    err_c = scpi_dev_chan(chan, &chan_mask);
    if (!err_v && !err_c) {
        cgr101_digitizer_voltage_low(info, chan_mask, value);
    }
}

void scpi_dev_sense_voltage_offset(struct info *info,
                                          struct scpi_type *v,
                                          struct scpi_type *chan)
{
    double value;
    long chan_mask;
    int err_v;
    int err_c;

    err_v = scpi_input_fp(info, v, &value);
    err_c = scpi_dev_chan(chan, &chan_mask);
    if (!err_v && !err_c) {
        cgr101_digitizer_voltage_offset(info, chan_mask, value);
    }
}

void scpi_dev_sense_voltage_ptp(struct info *info,
                                       struct scpi_type *v,
                                       struct scpi_type *chan)
{
    double value;
    long chan_mask;
    int err_v;
    int err_c;

    err_v = scpi_input_fp(info, v, &value);
    err_c = scpi_dev_chan(chan, &chan_mask);
    if (!err_v && !err_c) {
        cgr101_digitizer_voltage_ptp(info, chan_mask, value);
    }
}

void scpi_dev_sense_voltage_up(struct info *info,
                                      struct scpi_type *v,
                                      struct scpi_type *chan)
{
    double value;
    long chan_mask;
    int err_v;
    int err_c;

    err_v = scpi_input_fp(info, v, &value);
    err_c = scpi_dev_chan(chan, &chan_mask);
    if (!err_v && !err_c) {
        cgr101_digitizer_voltage_up(info, chan_mask, value);
    }
}

void scpi_dev_sense_voltage_lowq(struct info *info,
                                 struct scpi_type *chan)
{
    long chan_mask;

    if (!scpi_dev_chan(chan, &chan_mask)) {
        cgr101_digitizer_lowq(info, chan_mask);
    }
}

void scpi_dev_sense_voltage_offsetq(struct info *info,
                                    struct scpi_type *chan)
{
    long chan_mask;

    if (!scpi_dev_chan(chan, &chan_mask)) {
        cgr101_digitizer_offsetq(info, chan_mask);
    }
}

void scpi_dev_sense_voltage_ptpq(struct info *info,
                                 struct scpi_type *chan)
{
    long chan_mask;

    if (!scpi_dev_chan(chan, &chan_mask)) {
        cgr101_digitizer_ptpq(info, chan_mask);
    }
}

void scpi_dev_sense_voltage_upq(struct info *info,
                                struct scpi_type *chan)
{
    long chan_mask;

    if (!scpi_dev_chan(chan, &chan_mask)) {
        cgr101_digitizer_upq(info, chan_mask);
    }
}

void scpi_dev_source_digital_data(struct info *info,
                                  struct scpi_type *v)
{
    uint8_t value;

    if (!scpi_input_uint8(info, v, &value)) {
        cgr101_source_digital_data(info, value);
    }
}

void scpi_dev_source_digital_dataq(struct info *info)
{
    cgr101_source_digital_dataq(info);
}

void scpi_dev_source_waveform_frequency(struct info *info,
                                        struct scpi_type *v)
{
    double value;

    if (!scpi_input_fp(info, v, &value)) {
        cgr101_source_waveform_frequency(info, value);
    }
}

void scpi_dev_source_waveform_frequencyq(struct info *info)
{
    cgr101_source_waveform_frequencyq(info);
}

void scpi_dev_source_waveform_level(struct info *info,
                                    struct scpi_type *v)
{
    double value;

    if (!scpi_input_fp(info, v, &value)) {
        cgr101_source_waveform_level(info, value);
    }
}

void scpi_dev_source_waveform_levelq(struct info *info)
{
    cgr101_source_waveform_levelq(info);
}

void scpi_dev_source_waveform_function(struct info *info,
                                       struct scpi_type *v)
{
    const char *value;

    if (!scpi_input_str(info, v, &value)) {
        cgr101_source_waveform_function(info, value);
    }
}

void scpi_dev_source_waveform_functionq(struct info *info)
{
    cgr101_source_waveform_functionq(info);
}

void scpi_dev_source_waveform_user(struct info *info,
                                   struct scpi_type *v)
{
    size_t len;
    double *value;

    if (!scpi_input_fp_block(info, v, &len, &value)) {
        cgr101_source_waveform_user(info, len, value);
    }
}

void scpi_dev_source_waveform_userq(struct info *info)
{
    cgr101_source_waveform_userq(info);
}

void scpi_dev_source_pwm_duty_cycle(struct info *info,
                                    struct scpi_type *v)
{
    double value;

    if (!scpi_input_fp_range(info, v, 0.0, 1.0, &value)) {
        cgr101_source_pwm_duty_cycle(info, value);
    }
}

void scpi_dev_source_pwm_duty_cycleq(struct info *info)
{
    cgr101_source_pwm_duty_cycleq(info);
}

void scpi_dev_source_pwm_frequency(struct info *info,
                                   struct scpi_type *v)
{
    double value;

    if (!scpi_input_fp_range(info, v, 0.0, 72.0e3, &value)) {
        cgr101_source_pwm_frequency(info, value);
    }
}

void scpi_dev_source_pwm_frequencyq(struct info *info)
{
    cgr101_source_pwm_frequencyq(info);
}

void scpi_dev_sense_statq(struct info *info)
{
    cgr101_digitizer_statq(info);
}

void scpi_dev_sense_reset(struct info *info)
{
    cgr101_digitizer_reset(info);
}

void scpi_dev_trigger_coupling(struct info *info,
                               struct scpi_type *v)
{
    const char *value;

    if (!scpi_input_str(info, v, &value)) {
        cgr101_trigger_coupling(info, value);
    }
}

void scpi_dev_trigger_level(struct info *info,
                            struct scpi_type *v)
{
    double value;

    if (!scpi_input_fp(info, v, &value)) {
        cgr101_trigger_level(info, value);
    }
}

void scpi_dev_trigger_levelq(struct info *info)
{
        cgr101_trigger_levelq(info);
}

void scpi_dev_trigger_slope(struct info *info,
                            struct scpi_type *v)
{
    const char *value;

    if (!scpi_input_str(info, v, &value)) {
        cgr101_trigger_slope(info, value);
    }
}

void scpi_dev_trigger_slopeq(struct info *info)
{
    cgr101_trigger_slopeq(info);
}

void scpi_dev_trigger_source(struct info *info,
                             struct scpi_type *v)
{
    const char *value;

    if (!scpi_input_str(info, v, &value)) {
        cgr101_trigger_source(info, value);
    }
}

void scpi_dev_trigger_sourceq(struct info *info)
{
    cgr101_trigger_sourceq(info);
}

void scpi_dev_identify(struct info *info)
{
    cgr101_identify(info);
}

void scpi_dev_rst(struct info *info)
{
    cgr101_rst(info);
}

static int scpi_dev_input_digital_event(struct info *info,
                                        struct scpi_type *v1,
                                        struct scpi_type *v2,
                                        struct scpi_type *v3,
                                        const char **int_sel,
                                        long *count,
                                        long *chan_mask)
{
    int err;

    do {
        if (v1) {
            err = scpi_input_str(info, v1, int_sel);
            if (err) {
                break;
            }
        }

        err = scpi_input_int(info, v2, 0, 0x00ffffff, count);
        if (err) {
            break;
        }

        err = scpi_dev_chan(v3, chan_mask);

        if (err) {
            break;
        }
    } while(0);

    return err;
}

int scpi_dev_conf_digital_event(struct info *info,
                                struct scpi_type *v1,
                                struct scpi_type *v2,
                                struct scpi_type *v3)
{
    const char *int_sel = NULL;
    long count;
    long chan_mask;
    int err;

    err = scpi_dev_input_digital_event(info,
                                       v1,
                                       v2,
                                       v3,
                                       &int_sel,
                                       &count,
                                       &chan_mask);


    if (!err) {
        cgr101_configure_digital_event(info, int_sel, count, chan_mask);
    }

    return err;
}

void scpi_dev_fetch_digital_eventq(struct info *info)
{
    cgr101_fetch_digital_event(info);
}

void scpi_dev_measure_digital_eventq(struct info *info,
                                     struct scpi_type *v1,
                                     struct scpi_type *v2,
                                     struct scpi_type *v3)
{
    if (!scpi_dev_conf_digital_event(info, v1, v2, v3)) {
        scpi_dev_read_digital_eventq(info);
    }
}

void scpi_dev_read_digital_eventq(struct info *info)
{
    do {
        if (scpi_dev_abort(info)) {
            break;
        }
        if (scpi_dev_initiate(info)) {
            break;
        }
        scpi_dev_fetch_digital_eventq(info);
    } while (0);
}

void scpi_system_internal_offset(struct info *info,
                                 struct scpi_type *v1,
                                 struct scpi_type *v2,
                                 struct scpi_type *v3,
                                 struct scpi_type *v4)
{
    double f1;
    double f2;
    double f3;
    double f4;
    int ok = 0;

    if (!scpi_input_fp(info, v1, &f1)) {
        ok += 1;
    }

    if (!scpi_input_fp(info, v2, &f2)) {
        ok += 1;
    }

    if (!scpi_input_fp(info, v3, &f3)) {
        ok += 1;
    }

    if (!scpi_input_fp(info, v4, &f4)) {
        ok += 1;
    }

    if (ok == 4) {
        cgr101_digitizer_input_offset(info, f1, f2, f3, f4);
    }
}

void scpi_system_internal_offset_store(struct info *info)
{
    cgr101_digitizer_input_offset_store(info);
}

void scpi_system_internal_offsetq(struct info *info)
{
    cgr101_digitizer_input_offsetq(info);
}
