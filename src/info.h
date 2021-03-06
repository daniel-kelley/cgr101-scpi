/*
   info.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   INFO_H_
#define   INFO_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

#define INFO_CLI_LEN (16*1024)

struct scpi_output;
struct scpi_errq;
struct worker;
struct event;
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
    int overlapped;
    int block_input;
    int enable_flash_writes;
    const char *debug;
    size_t cli_offset;
    char cli_buf[INFO_CLI_LEN];
    char *cli_line;
    size_t cli_line_len;
    struct response rsp;
    const char *conf_rsp;
    void *hdl;
    struct lexer *lexer;
    struct parser *parser;
    struct scpi_core *scpi;
    struct scpi_output *output;
    struct scpi_errq *error;
    struct worker *worker;
    struct event *event;
    struct cgr101 *device;
    int sweep_status;
    int digital_event_status;
    int offset_status;
};

#endif /* INFO_H_ */
