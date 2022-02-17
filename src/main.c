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
#include "worker.h"
#include "event.h"

#define SCPI_PORT 5025

static struct info info_ = {
    .port = SCPI_PORT,
    .bus = -1,
};

static int init(struct info *info)
{
    int err = 0;

    do {
        info->worker = worker_init();
        if (!info->worker) {
            err = -1;
            break;
        }

        info->event = event_init(info->worker);
        if (!info->event) {
            err = -1;
            break;
        }

        err = cgr101_open(info);

        if (!err) {
            err = server_run(info);
        } else {
            fprintf(stderr, "CGR-101 open failed.\n");
        }
    } while (0);

    return err;
}

static int done(struct info *info)
{
    int err = 0;

    cgr101_close(info);

    if (info->event) {
        event_done(info->event);
    }

    if (info->worker) {
        worker_done(info->worker);
    }

#if 0
    conf_done(&info.conf);
#endif

    return err;
}

static void usage(const char *prog)
{
    fprintf(stderr,"%s [-b bus] [-d dev] [-p port] [-vh]\n", prog);
    fprintf(stderr,"  -h        Print this message\n");
    fprintf(stderr,"  -b        USB Bus (default 0)\n");
    fprintf(stderr,"  -d        USB Device (default 0)\n");
    fprintf(stderr,"  -p        server port (default %d)\n", SCPI_PORT);
    fprintf(stderr,"  -v        Verbose mode\n");
    fprintf(stderr,"  -W        Enable flash writes\n");
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

    while ((c = getopt(argc, argv, "b:c:d:p:r:D:vxhW")) != EOF) {
        switch (c) {
        case 'b':
            info_.bus = (int)strtol(optarg, NULL, 0);
            break;
        case 'd':
            info_.dev = (int)strtol(optarg, NULL, 0);
            break;
        case 'p':
            info_.port = (int)strtol(optarg, NULL, 0);
            break;
        case 'r':
            info_.conf_rsp = optarg;
            break;
        case 'W':
            info_.enable_flash_writes = 1;
            break;
        case 'c':
            /* FIXME */
#if 0
            conf_add(&info_.conf, optarg);
#endif
            break;
        case 'v':
            info_.verbose = 1;
            break;
        case 'x':
            info_.no_trap = 1;
            break;
        case 'D':
            info_.debug = optarg;
            break;
        case 'h':
            usage(argv[0]);
            return EXIT_SUCCESS;
            break;
        default:
            break;
        }
    }

    rc = init(&info_);
    done(&info_);

    return rc;
}

