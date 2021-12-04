/*
   scpi_core.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCPI_CORE_H_
#define   SCPI_CORE_H_

#include "info.h"
#include "scpi_error.h"
#include "scpi_output.h"

#define SCPI_SESR_OPC (1u<<0) /* SESR bit 0 Operation Complete */
#define SCPI_SBR_ERQ  (1u<<2) /* SBR  bit 2 Error queue not empty */
#define SCPI_SBR_QUES (1u<<3) /* SBR  bit 3 SCPI QUEStionable status */
#define SCPI_SBR_SESR (1u<<5) /* SBR  bit 6 SESR Status */
#define SCPI_SBR_MSS  (1u<<6) /* SBR  bit 6 Master Summary Status */
#define SCPI_SBR_OPER (1u<<7) /* SBR  bit 7 SCPI OPERation status */

struct scpi_core {
    /* SCPI Registers */
    uint16_t            ques_event;
    uint16_t            ques_enable;
    uint16_t            oper_event;
    uint16_t            oper_enable;
    /* 488.2 Registers */
    uint8_t             seser;  /* Standard Event Status Enable Register */
    uint8_t             sesr;   /* Standard Event Status Register */
    uint8_t             sbr;    /* Status Byte Register */
    uint8_t             srer;   /* Service Request Enable Register */
    /* Outputs */
    struct scpi_errq    error;
};

extern int scpi_core_init(struct info *info);
extern int scpi_core_done(struct info *info);
extern int scpi_core_send(struct info *info, char *buf, int len);

#endif /* SCPI_CORE_H_ */
