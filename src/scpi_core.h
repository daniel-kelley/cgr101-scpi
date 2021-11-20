/*
   scpi_core.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCPI_CORE_H_
#define   SCPI_CORE_H_

#include "info.h"
#include "scpi_error.h"
#include "scpi_output.h"

#define SCPI_SESR_OPC 1 /* SESR Operation Complete */

struct scpi_core {
    /* Standard Status Registers */
    uint16_t            ques;
    uint16_t            oper;
    uint8_t             seser;  /* Standard Event Status Enable Register */
    uint8_t             sesr;   /* Standard Event Status Register */
    uint8_t             sbr;    /* Status Byte Register */
    uint8_t             srer;   /* Service Request Enable Register */
    /* Outputs */
    struct scpi_errq    error;
    struct scpi_output  output;
};

extern int scpi_core_init(struct info *info);
extern int scpi_core_done(struct info *info);
extern int scpi_core_send(struct info *info, char *buf, int len);

#endif /* SCPI_CORE_H_ */
