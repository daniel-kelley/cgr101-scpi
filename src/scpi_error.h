/*
   scpi_error.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCPI_ERROR_H_
#define   SCPI_ERROR_H_

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

#endif /* SCPI_ERROR_H_ */
