/*
   scpi_output.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCPI_OUTPUT_H_
#define   SCPI_OUTPUT_H_

#include <stdint.h>

extern struct scpi_output *scpi_output_init(void);
extern int scpi_output_done(struct scpi_output *output);
extern void scpi_output_reset(struct scpi_output *output);
extern int scpi_output_printf(struct scpi_output *output,
                              const char *format,
                              ...);
extern int scpi_output_int(struct scpi_output *output, int value);
extern int scpi_output_fp(struct scpi_output *output, double value);
extern int scpi_output_str(struct scpi_output *output, const char *value);
extern int scpi_output_cmd_sep(struct scpi_output *output);
extern void scpi_output_clear(struct scpi_output *output);
extern void scpi_output_flush(struct scpi_output *output, int fd);

#endif /* SCPI_OUTPUT_H_ */
