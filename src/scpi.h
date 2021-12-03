/*
   scpi.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCPI_H_
#define   SCPI_H_

#include "info.h"
#include "scpi_type.h"
#include "yyerror.h"

extern void scpi_common_ese(struct info *info, struct scpi_type *val);
extern void scpi_common_eseq(struct info *info);
extern void scpi_common_esrq(struct info *info);
extern void scpi_common_idnq(struct info *info);
extern void scpi_common_cls(struct info *info);
extern void scpi_common_opc(struct info *info);
extern void scpi_common_opcq(struct info *info);
extern void scpi_common_rst(struct info *info);
extern void scpi_common_sre(struct info *info, struct scpi_type *val);
extern void scpi_common_sreq(struct info *info);
extern void scpi_common_stbq(struct info *info);
extern void scpi_common_tstq(struct info *info);
extern void scpi_common_wai(struct info *info);

extern void scpi_system_internal_quit(struct info *info);
extern void scpi_system_internal_setupq(struct info *info);
extern void scpi_system_versionq(struct info *info);
extern void scpi_system_capabilityq(struct info *info);
extern void scpi_system_error_countq(struct info *info);
extern void scpi_system_error_nextq(struct info *info);


extern void scpi_status_operation_eventq(struct info *info);
extern void scpi_status_operation_conditionq(struct info *info);
extern void scpi_status_operation_enable(struct info *info,
                                         struct scpi_type *val);
extern void scpi_status_operation_enableq(struct info *info);
extern void scpi_status_operation_preset(struct info *info);

extern void scpi_status_questionableq(struct info *info);
extern void scpi_status_questionable_enable(struct info *info,
                                            struct scpi_type *val);
extern void scpi_status_questionable_enableq(struct info *info);

extern void scpi_core_top(struct info *info);
extern void scpi_core_cmd_sep(struct info *info);

extern void scpi_dev_abort(struct info *info);

extern struct scpi_type *scpi_dev_channel_num(struct info *info,
                                              struct scpi_type *val);
extern struct scpi_type *scpi_dev_channel_range(struct info *info,
                                                struct scpi_type *v1,
                                                struct scpi_type *v2);
extern struct scpi_type *scpi_dev_channel_range_append(struct info *info,
                                                       struct scpi_type *v1,
                                                       struct scpi_type *v2);

extern void scpi_dev_measure_digital_dataq(struct info *info);

extern struct scpi_type *scpi_core_format_type(struct info *info,
                                               struct scpi_type *v1,
                                               struct scpi_type *v2);

extern struct scpi_type *scpi_core_nrf_list(struct info *info,
                                            struct scpi_type *v);


extern struct scpi_type *scpi_core_symbolic_value(struct info *info,
                                                  struct scpi_type *v);

extern void scpi_dev_conf_digital_data(struct info *info);
extern void scpi_dev_confq(struct info *info);
extern void scpi_dev_fetch_digital_dataq(struct info *info);
extern void scpi_core_format(struct info *info, struct scpi_type *v);
extern void scpi_core_format(struct info *info, struct scpi_type *v);
extern void scpi_core_formatq(struct info *info);
extern void scpi_core_formatq(struct info *info);
extern void scpi_core_initiate(struct info *info);
extern void scpi_dev_input_coupling(struct info *info, struct scpi_type *v);
extern void scpi_dev_read_digital_dataq(struct info *info);
extern void scpi_dev_sense_dataq(struct info *info, struct scpi_type *v);
extern void scpi_dev_sense_function_concurrent(struct info *info,
                                               struct scpi_type *v);
extern void scpi_dev_sense_function_off(struct info *info,
                                        struct scpi_type *v);
extern void scpi_dev_sense_function_stateq(struct info *info,
                                           struct scpi_type *v);
extern void scpi_dev_sense_function_on(struct info *info,
                                       struct scpi_type *v);
extern void scpi_dev_sense_sweep_point(struct info *info,
                                       struct scpi_type *v);
extern void scpi_dev_sense_sweep_time(struct info *info, struct scpi_type *v);
extern void scpi_dev_sense_sweep_time_interval(struct info *info,
                                               struct scpi_type *v);
extern void scpi_dev_sense_sweep_pretrigger(struct info *info,
                                            struct scpi_type *v);
extern void scpi_dev_sense_voltage_low(struct info *info,
                                       struct scpi_type *v1,
                                       struct scpi_type *v2);
extern void scpi_dev_sense_voltage_offset(struct info *info,
                                          struct scpi_type *v1,
                                          struct scpi_type *v2);
extern void scpi_dev_sense_voltage_ptp(struct info *info,
                                       struct scpi_type *v1,
                                       struct scpi_type *v2);
extern void scpi_dev_sense_voltage_range(struct info *info,
                                         struct scpi_type *v1,
                                         struct scpi_type *v2);
extern void scpi_dev_sense_voltage_up(struct info *info,
                                      struct scpi_type *v1,
                                      struct scpi_type *v2);
extern void scpi_dev_sense_voltage_lowq(struct info *info,
                                       struct scpi_type *v);
extern void scpi_dev_sense_voltage_offsetq(struct info *info,
                                          struct scpi_type *v);
extern void scpi_dev_sense_voltage_ptpq(struct info *info,
                                       struct scpi_type *v1);
extern void scpi_dev_sense_voltage_rangeq(struct info *info,
                                         struct scpi_type *v);
extern void scpi_dev_sense_voltage_upq(struct info *info,
                                      struct scpi_type *v);

#endif /* SCPI_H_ */
