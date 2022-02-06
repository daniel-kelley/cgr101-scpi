/*
   server.c

   Copyright (c) 2021 by Daniel Kelley

   SCPI server loop
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>

/* CGR-101 stuff here... */
#include "scpi_core.h"
#include "server.h"
#include "worker.h"

/* bitwise flags */
#define SERVER_ACCEPT (1<<1)
#define SERVER_WORKER (1<<2)

static volatile int server_exit;

static void ctrlc_action(int sig)
{
    (void)sig;
    server_exit = 1;
}

static int server_trap(struct info *info)
{
    struct sigaction action;

    (void)info;
    memset(&action, 0, sizeof(action));
    action.sa_handler = ctrlc_action;
    if (sigaction(SIGINT, &action, NULL) < 0) {
        return -1;
    }

    return 0;
}

static int server_done(struct info *info)
{
    int err = 0;

    if (info->listen_fd) {
        close(info->listen_fd);
    }

    return err;
}

/* This runs from a SCPI command. */
int server_listen(struct info *info)
{
    int fd;
    int rc = -1;
    int on = 1;
    struct sockaddr_in servaddr;

    /* Configure TCP Server */
    memset(&servaddr, 0, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((uint16_t)info->port);

    /* Create socket */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket() failed");
        exit(-1);
    }

    /* Set address reuse enable */
    rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof (on));
    if (rc < 0) {
        perror("setsockopt() failed");
        close(fd);
        exit(-1);
    }

    /* Set non blocking */
    rc = ioctl(fd, FIONBIO, (char *) &on);
    if (rc < 0) {
        perror("ioctl() failed");
        close(fd);
        exit(-1);
    }

    /* Bind to socket */
    rc = bind(fd, (struct sockaddr *) &servaddr, sizeof (servaddr));
    if (rc < 0) {
        perror("bind() failed");
        close(fd);
        exit(-1);
    }

    /* Set close-on-exec. */
    fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);

    info->listen_fd = fd;
    /* Listen on socket. Does this block? */
    rc = listen(fd, 1);
    if (rc < 0) {
        perror("listen() failed");
        close(fd);
        exit(-1);
    }

    return rc;
}

static int server_select(struct info *info)
{
    fd_set fds;
    struct timeval timeout;
    int rc;
    int max_fd = -1;
    int srv_event = 0; /* flags: SERVER_* */
    int idx;
    int workers;
    int wfd;

    FD_ZERO(&fds);

    /* Listen fd */
    if (info->listen_fd) {
        if (max_fd < info->listen_fd) {
            max_fd = info->listen_fd;
        }
        FD_SET(info->listen_fd, &fds);
    }

    workers = worker_count(info->worker);
    for (idx = 0; idx < workers; idx++) {
        wfd = worker_getfd(info->worker, idx);
        if (max_fd < wfd) {
            max_fd = wfd;
        }
        FD_SET(wfd, &fds);
    }

    memset(&timeout, 0, sizeof(timeout));

    if (timeout.tv_sec == 0 && timeout.tv_usec == 0) {
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;
    }

    rc = select(max_fd + 1, &fds, NULL, NULL, &timeout);
    if (rc < 0 && errno != EINTR) {
        return rc;
    }

    if (info->listen_fd && FD_ISSET(info->listen_fd, &fds)) {
        srv_event |= SERVER_ACCEPT;
    }

    for (idx = 0; idx < workers; idx++) {
        wfd = worker_getfd(info->worker, idx);
        if (FD_ISSET(wfd, &fds)) {
            worker_ready(info->worker, idx);
            srv_event |= SERVER_WORKER;
        }
    }

    return srv_event;
}

static int server_accept(struct info *info)
{
    int clifd;
    struct sockaddr_in cliaddr;
    socklen_t clilen;
    int err = -1;

    clilen = sizeof (cliaddr);
    clifd = accept(info->listen_fd, (struct sockaddr *) &cliaddr, &clilen);

    if (clifd > 0) {

        if (info->verbose) {
            printf("Connection established %s\r\n", inet_ntoa(cliaddr.sin_addr));
        }
        /* Push clifd socket (IO) to parser incl stack. */
        err = 0;
    }

    return err;
}

static int server_quit(const struct info *info)
{
    return (server_exit || info->quit);
}

static int server_action(struct info *info, int event)
{
    int err = 0;

    if (event & SERVER_ACCEPT) {
        server_accept(info);
    }

    if (event & SERVER_WORKER) {
        if (worker_run_ready(info->worker)) {
            err = 1;
            if (info->verbose) {
                fprintf(stderr, "worker_run_ready() failed.\n");
            }
        }
    }

    return err;
}

static int server_loop(struct info *info)
{
    int event;
    int err = 0;

    while (!server_quit(info)) {
        event = server_select(info);
        assert(event >= 0);
        err = server_action(info, event);
        if (err) {
            if (info->verbose) {
                fprintf(stderr, "server_action() failed (%d).\n", err);
            }
            break;
        }
    }

    return err;
}

int server_run(struct info *info)
{
    int rc = 1;

    if (!info->no_trap) {
        server_trap(info);
    }

    do {
        if (scpi_core_init(info)) {
            if (info->verbose) {
                fprintf(stderr, "scpi_core_init() failed.\n");
            }
            break;
        }
        rc = server_loop(info);
    } while (0);

    server_done(info);
    scpi_core_done(info);

    return (rc < 0);
}

