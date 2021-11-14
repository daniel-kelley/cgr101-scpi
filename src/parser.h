/*
   parser.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   PARSER_H_
#define   PARSER_H_

#include <stddef.h>
#include "info.h"
#include "scpi_type.h"

extern void parser_input(
    struct info *info,
    char *buf,
    int result,
    int max_size);

extern int parser_init(struct info *info);
extern int parser_done(struct info *info);
extern int parser_send(struct info *info, char *line, int len);
extern int parser_num(const char *s, struct scpi_type *val, int token);
extern int parser_error_get(struct info *info, const char **msg, int *trace);

#endif /* PARSER_H_ */
