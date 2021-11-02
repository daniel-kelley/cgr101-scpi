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
    } type;
    union {
        long ival;
    } val;
    const char *src;
};

#endif /* SCPI_TYPE_H_ */
