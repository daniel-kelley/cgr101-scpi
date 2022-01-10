/*
   scpi_type.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCPI_TYPE_H_
#define   SCPI_TYPE_H_

struct scpi_type {
    enum {
        SCPI_TYPE_UNKNOWN,
        SCPI_TYPE_INT,
        SCPI_TYPE_FLOAT,
        SCPI_TYPE_CHAN,
    } type;
    union {
        long ival;
        double fval;
        long chan;
    } val;
    const char *src;
};

#endif /* SCPI_TYPE_H_ */
