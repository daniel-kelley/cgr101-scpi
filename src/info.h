/*
   info.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   INFO_H_
#define   INFO_H_

#include <stddef.h>
#include <stdint.h>

#define INFO_CLI_LEN 80

struct scpi_output;
struct scpi_errq;
struct worker;
struct cgr101;

struct response {
    int valid;
    uint8_t *buf;
    size_t len;
};

struct info {
    int verbose;
    int emulation;
    int no_trap;
    int quit;
    int cli_in_fd;
    int cli_out_fd;
    int listen_fd;
    int rsp_fd;
    int bus;
    int dev;
    int port;
    int busy;
    const char *debug;
    char cli_buf[INFO_CLI_LEN];
    struct response rsp;
    const char *conf_rsp;
    void *hdl;
    struct lexer *lexer;
    struct parser *parser;
    struct scpi_core *scpi;
    struct scpi_output *output;
    struct scpi_errq *error;
    struct worker *worker;
    struct cgr101 *device;
    int sweep_status;
};

#endif /* INFO_H_ */
