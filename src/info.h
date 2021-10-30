/*
   info.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   INFO_H_
#define   INFO_H_

#define INFO_CLI_LEN 80

struct info {
    int verbose;
    int no_trap;
    int quit;
    int cli_in_fd;
    int cli_out_fd;
    int listen_fd;
    int rsp_fd;
    int bus;
    int dev;
    int port;
    const char *debug;
    char cli_buf[INFO_CLI_LEN];
    const char *conf_rsp;
    void *hdl;
    struct lexer *lexer;
    struct parser *parser;
    /* others..., like command queue */
};

#endif /* INFO_H_ */
