/*
   scpi_type.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCPI_TYPE_H_
#define   SCPI_TYPE_H_

enum scpi_type_name {
    SCPI_TYPE_UNKNOWN,
    SCPI_TYPE_INT,
    SCPI_TYPE_FLOAT,
    SCPI_TYPE_STR,
    SCPI_TYPE_CHAN,
    SCPI_TYPE_LIST,
};

union scpi_type_val {
    long ival;
    double fval;
    long chan;
    struct scpi_type_list *list;
};

struct scpi_type_list {
    enum scpi_type_name type;
    size_t len;
    size_t max;
    union scpi_type_val *val;
};

struct scpi_type {
    enum scpi_type_name type;
    union scpi_type_val val;
    const char *src;
    int token;
};

#endif /* SCPI_TYPE_H_ */
