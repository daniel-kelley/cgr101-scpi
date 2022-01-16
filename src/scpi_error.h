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
    SCPI_ERR_INTERNAL_PARSER_ERROR = 100,
    SCPI_ERR_UNDEFINED_HEADER = -113,
    SCPI_ERR_DATA_OUT_OF_RANGE = -222,
    SCPI_ERR_HARDWARE_ERROR = -240,
    SCPI_ERR_QUEUE_OVERFLOW = -350,
};

struct scpi_errq;

extern int scpi_error(struct scpi_errq *errq,
                      enum scpi_err_num num,
                      const char *syndrome);

extern int scpi_error_count(struct scpi_errq *errq);

extern int scpi_error_get(struct scpi_errq *errq,
                          enum scpi_err_num *num,
                          const char **msg,
                          const char **syndrome);

extern struct scpi_errq *scpi_error_init(void);
extern void scpi_error_reset(struct scpi_errq *errq);
extern int scpi_error_done(struct scpi_errq *errq);

#endif /* SCPI_ERROR_H_ */
