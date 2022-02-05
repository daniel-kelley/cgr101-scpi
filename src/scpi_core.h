/*
   scpi_core.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCPI_CORE_H_
#define   SCPI_CORE_H_

#include "info.h"
#include "scpi_error.h"
#include "scpi_output.h"
#include "scpi_type.h"

#define SCPI_SESR_OPC (1u<<0) /* SESR bit 0 Operation Complete */
#define SCPI_SBR_ERQ  (1u<<2) /* SBR  bit 2 Error queue not empty */
#define SCPI_SBR_QUES (1u<<3) /* SBR  bit 3 SCPI QUEStionable status */
#define SCPI_SBR_SESR (1u<<5) /* SBR  bit 5 SESR Status */
#define SCPI_SBR_MSS  (1u<<6) /* SBR  bit 6 Master Summary Status */
#define SCPI_SBR_OPER (1u<<7) /* SBR  bit 7 SCPI OPERation status */
#define SCPI_OPER_SWE (1u<<3) /* OPER bit 3 SCPI OPERation SWEEP */
#define SCPI_OPER_DE  (1u<<8) /* OPER bit 8 SCPI OPERation Digital Event */

struct scpi_reg {
    uint16_t            cond;   /* Condition Register */
    uint16_t            pos;    /* Positive Transition Filter Register */
    uint16_t            neg;    /* Negative Transition Filter Register */
    uint16_t            event;  /* Event Register */
    uint16_t            enable; /* Event Enable Register */
};

struct scpi_core {
    /* SCPI Registers */
    struct scpi_reg     ques;
    struct scpi_reg     oper;
    /* 488.2 Registers */
    uint8_t             seser;  /* Standard Event Status Enable Register */
    uint8_t             sesr;   /* Standard Event Status Register */
    uint8_t             sbr;    /* Status Byte Register */
    uint8_t             srer;   /* Service Request Enable Register */
    /* Internal memory pool(s) */
    void *pool;
    /* Internal flags */
    int opcq;
};

extern int scpi_core_init(struct info *info);
extern int scpi_core_done(struct info *info);
extern int scpi_core_send(struct info *info, char *buf, int len);
extern int scpi_core_recv(struct info *info);
extern int scpi_core_recv_ready(struct info *info);
extern int scpi_core_cli_read(struct info *info);
extern int scpi_core_block_input(struct info *info);
extern void scpi_core_scpi_recv(struct info *info);
extern void scpi_core_line(struct info *info);

#endif /* SCPI_CORE_H_ */
