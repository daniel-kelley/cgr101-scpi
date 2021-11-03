/*
   scpi_core.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCPI_CORE_H_
#define   SCPI_CORE_H_

#include "scpi_error.h"

#define OUTBUF_SIZE 1024
struct scpi_outbuf {
    uint32_t            len;
    uint8_t             buf[OUTBUF_SIZE];
};

struct scpi_core {
    /* Standard Status Registers */
    uint16_t            ques;
    uint16_t            oper;
    uint8_t             event;
    uint8_t             status;
    struct scpi_errq    error;
    struct scpi_outbuf  output;
};

#endif /* SCPI_CORE_H_ */
