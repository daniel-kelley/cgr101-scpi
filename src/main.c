/*
   main.c

   Copyright (c) 2021 by Daniel Kelley

   SCPI server main
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "server.h"
#include "cgr101.h"

static struct info info;

#define SCPI_PORT 5025

static void usage(const char *prog)
{
    fprintf(stderr,"%s [-b bus] [-d dev] [-p port] [-vh]\n", prog);
    fprintf(stderr,"  -h        Print this message\n");
    fprintf(stderr,"  -b        USB Bus (default 0)\n");
    fprintf(stderr,"  -d        USB Device (default 0)\n");
    fprintf(stderr,"  -p        server port (default %d)\n", SCPI_PORT);
    fprintf(stderr,"  -v        Verbose mode\n");
    fprintf(stderr,"  -c        Configuration file\n");
    fprintf(stderr,"  -D[flag]  Debug flags\n");
}

/*
 *
 */
int main(int argc, char *argv[])
{
    int rc = 1;
    int c;

    info.port = SCPI_PORT;
    info.bus = -1;
    while ((c = getopt(argc, argv, "b:c:d:p:r:D:vxh")) != EOF) {
        switch (c) {
        case 'b':
            info.bus = (int)strtol(optarg, NULL, 0);
            break;
        case 'd':
            info.dev = (int)strtol(optarg, NULL, 0);
            break;
        case 'p':
            info.port = (int)strtol(optarg, NULL, 0);
            break;
        case 'r':
            info.conf_rsp = optarg;
            break;
        case 'c':
            /* FIXME */
#if 0
            conf_add(&info.conf, optarg);
#endif
            break;
        case 'v':
            info.verbose = 1;
            break;
        case 'x':
            info.no_trap = 1;
            break;
        case 'D':
            info.debug = optarg;
            break;
        case 'h':
            usage(argv[0]);
            rc = EXIT_SUCCESS;
            break;
        default:
            break;
        }
    }

    rc = cgr101_open(&info);

    if (!rc) {
        rc = server_run(&info);
    } else {
        fprintf(stderr, "CGR-101 open failed.\n");
    }

    if (info.hdl) {
        cgr101_close(&info);
    }
#if 0
    conf_done(&info.conf);
#endif
    return rc;
}

