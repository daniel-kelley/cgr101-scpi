/*
   scpi_input.h

   Copyright (c) 2022 by Daniel Kelley

*/

#ifndef   SCPI_INPUT_H_
#define   SCPI_INPUT_H_

#include <stdint.h>
#include "scpi_type.h"

extern int scpi_input_int(struct info *info,
                          struct scpi_type *in,
                          long low,
                          long high,
                          long *out);
extern int scpi_input_boolean(struct info *info,
                              struct scpi_type *in,
                              int *out);
extern int scpi_input_uint8(struct info *info,
                            struct scpi_type *in,
                            uint8_t *out);
extern int scpi_input_uint16(struct info *info,
                             struct scpi_type *in,
                             uint16_t *out);
extern int scpi_input_fp(struct info *info,
                         struct scpi_type *in,
                         double *out);
extern int scpi_input_fp_block(struct info *info,
                               struct scpi_type *in,
                               size_t *out_len,
                               double **out_block);
extern int scpi_input_fp_range(struct info *info,
                               struct scpi_type *in,
                               double min,
                               double max,
                               double *out);
extern int scpi_input_str(struct info *info,
                          struct scpi_type *in,
                          const char **out);

#endif /* SCPI_INPUT_H_ */
