/*
   scpi_core.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <stdint.h>
#include <stdlib.h>
#include "scpi.h"
#include "parser.h"

struct scpi_err {
    int error;
    char *syndrome;
};

#define ERRQ_SIZE 32
struct scpi_errq {
    uint32_t            head;
    uint32_t            tail;
    struct scpi_err     q[ERRQ_SIZE];
};

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


void yyerror(void *loc, struct info *info, const char *s)
{
    (void)loc;
    (void)info;
    (void)s;
}

void scpi_common_cls(struct info *info)
{
    struct scpi_core *scpi = info->scpi;

    scpi->ques = 0;
    scpi->oper = 0;
    scpi->event = 0;
    scpi->status = 0;
    scpi->error.head = 0;
    scpi->error.tail = 0;
    scpi->output.len = 0;
}

void scpi_system_internal_quit(struct info *info)
{
    info->quit = 1;
}

int scpi_core_send(struct info *info)
{
    return parser_send(info, info->cli_buf, sizeof(info->cli_buf));
}

int scpi_core_init(struct info *info)
{
    int err;

    do {
        info->scpi = calloc(1,sizeof(*info->scpi));
        if (!info->scpi) {
            err = -1;
            break;
        }
        err = parser_init(info);
    } while (0);

    return err;
}

int scpi_core_done(struct info *info)
{
    int err;

    do {
        if (info->scpi) {
            free(info->scpi);
        }
        err = parser_done(info);
    } while (0);

    return err;
}
