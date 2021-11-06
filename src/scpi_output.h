/*
   scpi_output.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCPI_OUTPUT_H_
#define   SCPI_OUTPUT_H_

#include <stdint.h>

#define OUTPUT_SIZE 1024
struct scpi_output {
    int                 num_rsp;
    int                 num_elem;
    uint32_t            len;
    uint8_t             buf[OUTPUT_SIZE];
};

#endif /* SCPI_OUTPUT_H_ */
