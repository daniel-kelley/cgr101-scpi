/*
   scpi_error.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCPI_ERROR_H_
#define   SCPI_ERROR_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* All errors - standard and device specific. */
enum scpi_err_num {
    SCPI_ERR_NONE = 0,
    SCPI_ERR_DATA_OUT_OF_RANGE = -222,
    SCPI_ERR_QUEUE_OVERFLOW = -350,
};


struct scpi_err {
    enum scpi_err_num error;
    char *syndrome;
};

#define ERRQ_SIZE 32
struct scpi_errq {
    uint32_t            head;
    uint32_t            tail;
    bool                overflow;
    struct scpi_err     q[ERRQ_SIZE];
};

extern int scpi_error(struct scpi_errq *errq,
                      enum scpi_err_num num,
                      const char *syndrome);

extern int scpi_error_count(struct scpi_errq *errq);

extern int scpi_error_get(struct scpi_errq *errq,
                          enum scpi_err_num *num,
                          const char **msg,
                          const char **syndrome);

#endif /* SCPI_ERROR_H_ */
