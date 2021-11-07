/*
   scpi_error.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scpi_error.h"

#define COUNT(a) (sizeof((a))/sizeof((a)[0]))

static struct {
    enum scpi_err_num error;
    char *msg;
} scpi_err_msg[] = {
    { SCPI_ERR_NONE,
      "No error"
    },
    { SCPI_ERR_DATA_OUT_OF_RANGE,
      "Data out of range"
    },
    { SCPI_ERR_QUEUE_OVERFLOW,
      "Queue Overflow"
    },
    { SCPI_ERR_NONE,
      NULL,
    }
};

int scpi_error(struct scpi_errq *errq,
               enum scpi_err_num num,
               const char *syndrome)
{
    uint32_t head1 = errq->head + 1;
    if (head1 == ERRQ_SIZE) {
        head1 = 0;
    }

    if (head1 == errq->tail) {
        /* queue full */
        errq->overflow = true;
    } else {
        errq->q[errq->head].error = num;
        if (syndrome) {
            errq->q[errq->head].syndrome = strdup(syndrome);
            if (errq->q[errq->head].syndrome) {
                /* internal error! */
            }
        }
        errq->head++;
        if (errq->head == ERRQ_SIZE) {
            errq->head = 0;
        }
    }
    return 0;
}


int scpi_error_count(struct scpi_errq *errq)
{
    uint32_t count = 0;

    if (errq->head != errq->tail) {
        if (errq->head > errq->tail) {
            count = errq->head - errq->tail;
        } else if (errq->head < errq->tail) {
            count =  errq->head - errq->tail + ERRQ_SIZE;
        }

        if (errq->overflow) {
            count++;
        }
    }

    return (int)count;
}

static const char *scpi_errmsg(enum scpi_err_num num)
{
    size_t i;
    const char *msg = "???";

    for (i=0; i<COUNT(scpi_err_msg); i++) {
        if (scpi_err_msg[i].error == num) {
            msg = scpi_err_msg[i].msg;
            break;
        }
    }

    return msg;
}

/* Format: <num>,"<msg>[,<syndrome>]"*/
int scpi_error_get(struct scpi_errq *errq, char *buf, size_t len)
{
    const char *msg;
    const char *syndrome;
    uint32_t cur = errq->tail;
    int plen;
    enum scpi_err_num error;
    int err = 0;

    if (errq->head != errq->tail) {
        error = errq->q[cur].error;
        errq->tail = errq->tail + 1;
        if (errq->tail == ERRQ_SIZE) {
            errq->tail = 0;
        }
    } else if (errq->overflow) {
        error = SCPI_ERR_QUEUE_OVERFLOW;
        errq->overflow = !errq->overflow;
    } else {
        error = SCPI_ERR_NONE;
    }

    msg = scpi_errmsg(error);

    plen = snprintf(buf, len, "%d", (int)error);
    len -= (size_t)plen;
    buf += plen;

    /*
     * Note: may need to add quote escapes if any string has
     * embedded quote characters.
     */
    syndrome = errq->q[cur].syndrome;
    if (syndrome) {
        snprintf(buf, len, "\"%s;%s\"", msg, syndrome);
        free((void *)syndrome);
    } else {
        snprintf(buf, len, "\"%s\"", msg);
    }

    return err;
}

