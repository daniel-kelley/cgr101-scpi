/*
   scpi.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCPI_H_
#define   SCPI_H_

#include "info.h"
#include "scpi_type.h"

extern void yyerror(void *loc, struct info *info, const char *s);
extern void scpi_common_ese(struct scpi_type *val, struct info *info);
extern void scpi_common_eseq(struct info *info);
extern void scpi_common_cls(struct info *info);
extern void scpi_system_internal_quit(struct info *info);
extern void scpi_system_versionq(struct info *info);
extern void scpi_system_error_countq(struct info *info);
extern int scpi_core_init(struct info *info);
extern int scpi_core_done(struct info *info);
extern int scpi_core_send(struct info *info, char *buf, int len);
extern void scpi_core_top(struct info *info);
extern void scpi_core_cmd_sep(struct info *info);

#endif /* SCPI_H_ */
