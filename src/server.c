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
#define SERVER_CLI (1<<0)
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

static int server_conf_get(struct info *info)
{
    int conf_in_fd = 0;
    int err = -1;
    const char *file = NULL;

#if 0
    if (conf_valid(&info->conf)) {
        file = conf_get(&info->conf);
    }
#endif

    if (file) {
        conf_in_fd = open(file, O_CLOEXEC);
    } else {
        conf_in_fd = STDIN_FILENO;
    }

    if (conf_in_fd >= 0) {
        if (info->cli_in_fd != STDIN_FILENO) {
            /* Close current CLI fd if not STDIN*/
            close(info->cli_in_fd);
        }
        info->cli_in_fd = conf_in_fd;
        if (info->cli_out_fd == 0) {
            info->cli_out_fd = STDOUT_FILENO; /* default response to stdout */
        }
        err = 0;
    } else {
        assert(file != NULL);
        perror(file);
    }

    return err;
}

static int server_init(struct info *info)
{
    int err = 0;
    int on = 1;
    int conf_out_fd;

    /* default in/out from console */
    info->cli_in_fd = STDIN_FILENO;
    info->cli_out_fd = STDOUT_FILENO;

    /* Open conf_in */
    (void)server_conf_get(info);

    /* Open conf_rsp */
    if (info->conf_rsp) {
        conf_out_fd = open(info->conf_rsp, O_CREAT|O_WRONLY|O_TRUNC|O_CLOEXEC);
        if (conf_out_fd > 0) {
            info->cli_out_fd = conf_out_fd;
        } else {
            perror(info->conf_rsp);
        }
    }

    err = ioctl(info->cli_in_fd, FIONBIO, (char *) &on);
    if (err < 0) {
        perror("ioctl() failed");
        exit(-1);
    }

    err = ioctl(info->cli_out_fd, FIONBIO, (char *) &on);
    if (err < 0) {
        perror("ioctl() failed");
        exit(-1);
    }

    return err;
}

static int server_done(struct info *info)
{
    int err = 0;

    if (info->cli_in_fd != STDIN_FILENO) {
        close(info->cli_in_fd);
    }
    if (info->cli_out_fd != STDOUT_FILENO) {
        close(info->cli_out_fd);
    }
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

static int server_timed_block(struct info *info)
{
    struct timeval tv;
    int err;
    int rv = 0;

    err = gettimeofday(&tv, NULL);
    assert(!err);
    rv = timercmp(&tv, &info->block_until.tv, <);
    if (!rv) {
        info->block_until.active = 0;
    }

    return rv;
}

static int server_block_input(struct info *info)
{
    int rv = 0;

    if (info->block_until.active) {
        rv = server_timed_block(info);
    } else {
        rv = info->block_input;
    }

    return rv;
}

static int server_select(struct info *info)
{
    fd_set fds;
    struct timeval timeout;
    int rc;
    int max_fd;
    int srv_event = 0; /* flags: SERVER_* */
    int idx;
    int workers;
    int wfd;

    FD_ZERO(&fds);

    /* CLI fd */
    max_fd = info->cli_in_fd;
    FD_SET(info->cli_in_fd, &fds);

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

    if (FD_ISSET(info->cli_in_fd, &fds) &&
        !info->cli_line &&
        !server_block_input(info)) {
        srv_event |= SERVER_CLI;
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

        if (info->cli_in_fd != STDIN_FILENO) {
            /* Close current CLI fd if not STDIN*/
            close(info->cli_in_fd);
        }
        info->cli_in_fd = clifd;
        err = 0;
    }

    return err;
}

static int server_rsp(struct info *info)
{
    ssize_t outlen;

    outlen = write(info->cli_out_fd, info->rsp.buf, info->rsp.len);

    /* Handle truncated writes. */

    return (outlen <= 0);
}

static void server_scpi_recv(struct info *info)
{
    int err;

    if (scpi_core_recv_ready(info)) {
        scpi_core_recv(info);
        if (info->rsp.valid) {
            err = server_rsp(info);
            if (err && info->verbose) {
                fprintf(stderr, "server_rsp() failed.\n");
            }
        }
    }
}

static int server_scpi_send(struct info *info, char *buf, size_t len)
{
    int rc = 0;

    rc = scpi_core_send(info, buf, (int)len);
    server_scpi_recv(info);

    return rc;
}

static int server_cli_line(struct info *info)
{
    char *eol;
    size_t llen;
    int rc = 0;

    assert(info->cli_line);
    assert(info->cli_line_len > 0);
    eol = strchr(info->cli_line, '\n');
    if (eol) {
        llen = (size_t)(eol - info->cli_line);
        assert(llen < INFO_CLI_LEN);
        if (llen) {
            rc = server_scpi_send(info, info->cli_line, llen);
        }
        info->cli_line = eol + 1;
        info->cli_line_len -= (llen + 1);
        if (info->cli_line_len == 0) {
            info->cli_line = NULL;
        }
    } else {
        /* Do something with the leftovers */
        assert(info->cli_line_len < INFO_CLI_LEN);
        memmove(info->cli_buf, info->cli_line, info->cli_line_len);
        info->cli_offset = info->cli_line_len;
        info->cli_line = NULL;
        info->cli_line_len = 0;
    }

    return rc;
}

static int server_cli_read(struct info *info)
{
    int rc;
    char *cli_buf;
    size_t cli_len;

    /* Append to leftovers from the previous pass. */
    cli_buf = info->cli_buf;
    cli_len = sizeof(info->cli_buf);
    cli_buf += info->cli_offset;
    cli_len -= info->cli_offset;

    memset(cli_buf, 0, cli_len);

    do {
        rc = (int)read(info->cli_in_fd, cli_buf, cli_len);
    } while (rc < 0 && errno == EAGAIN);

    if (rc < 0) {
        if (errno != EWOULDBLOCK) {
            perror("read() failed");
        }
    } else if (rc == 0) {
        /* Get the next conf file, if any. */
        if (!server_conf_get(info)) {
            if (info->verbose) {
                printf("Done.\r\n");
            }
            info->quit = 1;
        }
    } else {
        info->cli_line = cli_buf;
        info->cli_line_len = (size_t)rc;
        rc = 0;
    }

    return rc;
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

    if (event & SERVER_CLI) {
        if (server_cli_read(info)) {
            err = 1;
            if (info->verbose) {
                fprintf(stderr, "server_cli_read() failed.\n");
            }
        }
    }

    if (info->cli_line && !server_block_input(info)) {
        server_cli_line(info);
    }

    if (event & SERVER_WORKER) {
        if (worker_run_ready(info->worker)) {
            err = 1;
            if (info->verbose) {
                fprintf(stderr, "worker_run_ready() failed.\n");
            }
        }
    }
    server_scpi_recv(info);

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
        if (server_init(info)) {
            if (info->verbose) {
                fprintf(stderr, "server_init() failed.\n");
            }
            break;
        }
        rc = server_loop(info);
    } while (0);

    server_done(info);
    scpi_core_done(info);

    return (rc < 0);
}

