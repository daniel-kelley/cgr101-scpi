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
#include "cgr101.h"

int scpi_dev_abort(struct info *info)
{
    (void)info;

    return 0;
}

struct scpi_type *scpi_dev_channel_num(struct info *info,
                                       struct scpi_type *val)
{
    (void)info;

    if (val->type == SCPI_TYPE_INT &&
        val->val.ival >= CGR101_MIN_CHAN &&
        val->val.ival <= CGR101_MAX_CHAN)
    {
        val->val.chan = 1<<val->val.ival;
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
        v2->val.chan |= chan_start;
        chan_start <<= 1;
        assert(chan_start != 0);
    }

    return v2;
}

struct scpi_type *scpi_dev_channel_range_append(struct info *info,
                                                struct scpi_type *v1,
                                                struct scpi_type *v2)
{
    (void)info;

    assert(v1->type == SCPI_TYPE_CHAN);
    assert(v2->type == SCPI_TYPE_CHAN);

    v2->val.chan |= v1->val.chan;

    return v2;
}

void scpi_dev_read_digital_dataq(struct info *info)
{
    do {
        if (scpi_dev_abort(info)) {
            break;
        }
        if (scpi_core_initiate(info)) {
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
    (void)info;
    (void)v;
}

void scpi_dev_sense_dataq(struct info *info, struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_dev_sense_function_concurrent(struct info *info,
                                               struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_dev_sense_function_off(struct info *info,
                                        struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_dev_sense_function_stateq(struct info *info,
                                           struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_dev_sense_function_on(struct info *info,
                                       struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_dev_sense_sweep_point(struct info *info,
                                       struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_dev_sense_sweep_time(struct info *info, struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_dev_sense_sweep_time_interval(struct info *info,
                                               struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_dev_sense_sweep_pretrigger(struct info *info,
                                            struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_dev_sense_voltage_low(struct info *info,
                                       struct scpi_type *v1,
                                       struct scpi_type *v2)
{
    (void)info;
    (void)v1;
    (void)v2;
}

void scpi_dev_sense_voltage_offset(struct info *info,
                                          struct scpi_type *v1,
                                          struct scpi_type *v2)
{
    (void)info;
    (void)v1;
    (void)v2;
}

void scpi_dev_sense_voltage_ptp(struct info *info,
                                       struct scpi_type *v1,
                                       struct scpi_type *v2)
{
    (void)info;
    (void)v1;
    (void)v2;
}

void scpi_dev_sense_voltage_range(struct info *info,
                                         struct scpi_type *v1,
                                         struct scpi_type *v2)
{
    (void)info;
    (void)v1;
    (void)v2;
}

void scpi_dev_sense_voltage_up(struct info *info,
                                      struct scpi_type *v1,
                                      struct scpi_type *v2)
{
    (void)info;
    (void)v1;
    (void)v2;
}

void scpi_dev_sense_voltage_lowq(struct info *info,
                                 struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_dev_sense_voltage_offsetq(struct info *info,
                                    struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_dev_sense_voltage_ptpq(struct info *info,
                                 struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_dev_sense_voltage_rangeq(struct info *info,
                                   struct scpi_type *v)
{
    (void)info;
    (void)v;
}

void scpi_dev_sense_voltage_upq(struct info *info,
                                struct scpi_type *v)
{
    (void)info;
    (void)v;
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
    float value;

    if (!scpi_input_float(info, v, &value)) {
        cgr101_source_waveform_frequency(info, value);
    }
}

void scpi_dev_source_waveform_frequencyq(struct info *info)
{
    cgr101_source_waveform_frequencyq(info);
}

void scpi_dev_source_waveform_function(struct info *info,
                                       struct scpi_type *v)
{
    char *value;

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
    float *value;

    if (!scpi_input_float_block(info, v, &len, &value)) {
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
    float value;

    if (!scpi_input_float(info, v, &value)) {
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
    float value;

    if (!scpi_input_float(info, v, &value)) {
        cgr101_source_pwm_frequency(info, value);
    }
}

void scpi_dev_source_pwm_frequencyq(struct info *info)
{
    cgr101_source_pwm_frequencyq(info);
}

