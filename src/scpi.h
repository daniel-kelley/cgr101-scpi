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

extern void scpi_dev_measure_digital_dataq(struct info *info,
                                           struct scpi_type *val);

#endif /* SCPI_H_ */
