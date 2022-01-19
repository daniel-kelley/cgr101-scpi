/*
   cgr101.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   CGR101_H_
#define   CGR101_H_

#include "info.h"

#define CGR101_MIN_CHAN 1
#define CGR101_MAX_CHAN 7

extern int cgr101_identify(struct info *info);
extern int cgr101_open(struct info *info);
extern int cgr101_close(struct info *info);
extern int cgr101_initiate(struct info *info);
extern int cgr101_initiate_immediate(struct info *info);
extern int cgr101_configure_digital_data(struct info *info);
extern int cgr101_digital_data_configured(struct info *info);
extern void cgr101_fetch_digital_data(struct info *info);
extern void cgr101_source_digital_data(struct info *info, int value);
extern void cgr101_source_digital_dataq(struct info *info);
extern void cgr101_source_waveform_frequency(struct info *info, double value);
extern void cgr101_source_waveform_frequencyq(struct info *info);
extern void cgr101_source_waveform_function(struct info *info,
                                            const char *value);
extern void cgr101_source_waveform_functionq(struct info *info);
extern void cgr101_source_waveform_level(struct info *info, double value);
extern void cgr101_source_waveform_levelq(struct info *info);
extern void cgr101_source_waveform_user(struct info *info,
                                        size_t len,
                                        double *data);
extern void cgr101_source_waveform_userq(struct info *info);
extern void cgr101_source_pwm_duty_cycle(struct info *info, double value);
extern void cgr101_source_pwm_duty_cycleq(struct info *info);
extern void cgr101_source_pwm_frequency(struct info *info, double value);
extern void cgr101_source_pwm_frequencyq(struct info *info);
extern void cgr101_digitizer_coupling(struct info *info, const char *value);
extern void cgr101_digitizer_dataq(struct info *info, long chan_mask);
extern void cgr101_digitizer_concurrent(struct info *info, int value);
extern void cgr101_digitizer_channel_state(struct info *info,
                                           long chan_mask,
                                           int value);
extern void cgr101_digitizer_channel_stateq(struct info *info, long chan_mask);
extern void cgr101_digitizer_offset_point(struct info *info, double value);
extern void cgr101_digitizer_offset_pointq(struct info *info);
extern void cgr101_digitizer_offset_time(struct info *info, double value);
extern void cgr101_digitizer_offset_timeq(struct info *info);
extern void cgr101_digitizer_oref_loc(struct info *info, double value);
extern void cgr101_digitizer_oref_locq(struct info *info);
extern void cgr101_digitizer_oref_point(struct info *info, double value);
extern void cgr101_digitizer_oref_pointq(struct info *info);
extern void cgr101_digitizer_sweep_pointq(struct info *info);
extern void cgr101_digitizer_sweep_time(struct info *info, double value);
extern void cgr101_digitizer_sweep_timeq(struct info *info);
extern void cgr101_digitizer_sweep_interval(struct info *info, double value);
extern void cgr101_digitizer_sweep_time_intervalq(struct info *info);
extern void cgr101_digitizer_voltage_low(struct info *info,
                                         long chan_mask,
                                         double value);
extern void cgr101_digitizer_voltage_offset(struct info *info,
                                            long chan_mask,
                                            double value);
extern void cgr101_digitizer_voltage_ptp(struct info *info,
                                         long chan_mask,
                                         double value);
extern void cgr101_digitizer_voltage_range(struct info *info,
                                           long chan_mask,
                                           double value);
extern void cgr101_digitizer_voltage_up(struct info *info,
                                        long chan_mask,
                                        double value);
extern void cgr101_digitizer_lowq(struct info *info, long chan_mask);
extern void cgr101_digitizer_offsetq(struct info *info, long chan_mask);
extern void cgr101_digitizer_ptpq(struct info *info, long chan_mask);
extern void cgr101_digitizer_rangeq(struct info *info, long chan_mask);
extern void cgr101_digitizer_upq(struct info *info, long chan_mask);
extern void cgr101_digitizer_statq(struct info *info);
extern void cgr101_digitizer_reset(struct info *info);
extern void cgr101_digitizer_immediate(struct info *info);
extern void cgr101_trigger_coupling(struct info *info,const char *value);
extern void cgr101_trigger_level(struct info *info, double value);
extern void cgr101_trigger_levelq(struct info *info);
extern void cgr101_trigger_slope(struct info *info, const char *value);
extern void cgr101_trigger_slopeq(struct info *info);
extern void cgr101_trigger_source(struct info *info, const char *value);
extern void cgr101_trigger_sourceq(struct info *info);
extern void cgr101_rst(struct info *info);
extern void cgr101_abort(struct info *info);

extern void cgr101_configure_digital_event(struct info *info,
                                           const char *int_sel,
                                           long count,
                                           long chan_mask);
extern void cgr101_fetch_digital_event(struct info *info);

#endif /* CGR101_H_ */
