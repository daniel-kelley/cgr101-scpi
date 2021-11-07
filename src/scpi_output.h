/*
   scpi_output.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCPI_OUTPUT_H_
#define   SCPI_OUTPUT_H_

#include <stdint.h>

#define OUTPUT_SIZE 1024
struct scpi_output {
    int                 num_sep;
    int                 num_elem;
    int                 overflow;
    size_t              len;
    uint8_t             buf[OUTPUT_SIZE];
};

extern int scpi_output_printf(struct scpi_output *output,
                              const char *format,
                              ...);
extern int scpi_output_int(struct scpi_output *output, int value);
extern int scpi_output_str(struct scpi_output *output, const char *value);
extern int scpi_output_cmd_sep(struct scpi_output *output);
extern void scpi_output_clear(struct scpi_output *output);
extern int scpi_output_get(struct scpi_output *output,
                           uint8_t **buf,
                           size_t *sz);

#endif /* SCPI_OUTPUT_H_ */
