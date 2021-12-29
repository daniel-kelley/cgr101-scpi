/*
   cgr101.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   CGR101_H_
#define   CGR101_H_

#include "info.h"

#define CGR101_MIN_CHAN 1
#define CGR101_MAX_CHAN 21

extern int cgr101_identify(struct info *info);
extern int cgr101_open(struct info *info);
extern int cgr101_close(struct info *info);
extern int cgr101_initiate(struct info *info);
extern int cgr101_configure_digital_data(struct info *info);
extern int cgr101_digital_data_configured(struct info *info);
extern void cgr101_fetch_digital_data(struct info *info);
extern void cgr101_source_digital_data(struct info *info, int value);
extern void cgr101_source_digital_dataq(struct info *info);
extern void cgr101_source_waveform_frequency(struct info *info, float value);
extern void cgr101_source_waveform_frequencyq(struct info *info);
extern void cgr101_source_waveform_function(struct info *info,
                                            const char *value);
extern void cgr101_source_waveform_functionq(struct info *info);
extern void cgr101_source_waveform_user(struct info *info,
                                        size_t len,
                                        float *data);
extern void cgr101_source_waveform_userq(struct info *info);
extern void cgr101_source_pwm_duty_cycle(struct info *info, float value);
extern void cgr101_source_pwm_duty_cycleq(struct info *info);
extern void cgr101_source_pwm_frequency(struct info *info, float value);
extern void cgr101_source_pwm_frequencyq(struct info *info);
extern void cgr101_digitizer_coupling(struct info *info, const char *value);
extern void cgr101_digitizer_dataq(struct info *info, long chan_mask);
extern void cgr101_digitizer_concurrent(struct info *info, int value);
extern void cgr101_digitizer_channel_state(struct info *info,
                                           long chan_mask,
                                           int value);
extern void cgr101_digitizer_channel_stateq(struct info *info, long chan_mask);
extern void cgr101_digitizer_sweep_point(struct info *info, float value);
extern void cgr101_digitizer_sweep_time(struct info *info, float value);
extern void cgr101_digitizer_sweep_interval(struct info *info, float value);
extern void cgr101_digitizer_sweep_pretrigger(struct info *info, float value);
extern void cgr101_digitizer_voltage_low(struct info *info,
                                         long chan_mask,
                                         float value);
extern void cgr101_digitizer_voltage_offset(struct info *info,
                                            long chan_mask,
                                            float value);
extern void cgr101_digitizer_voltage_ptp(struct info *info,
                                         long chan_mask,
                                         float value);
extern void cgr101_digitizer_voltage_range(struct info *info,
                                           long chan_mask,
                                           float value);
extern void cgr101_digitizer_voltage_up(struct info *info,
                                        long chan_mask,
                                        float value);
extern void cgr101_digitizer_lowq(struct info *info, long chan_mask);
extern void cgr101_digitizer_offsetq(struct info *info, long chan_mask);
extern void cgr101_digitizer_ptpq(struct info *info, long chan_mask);
extern void cgr101_digitizer_rangeq(struct info *info, long chan_mask);
extern void cgr101_digitizer_upq(struct info *info, long chan_mask);

#endif /* CGR101_H_ */
