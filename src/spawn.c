/*
   spawn.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "spawn.h"

#define MAXARG 10

/* Context needed for signal handling. One at a time, please! */
static struct spawn *spawn_sigdata;

static int cloexec(int fd)
{
    return fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
}

static void spawn_sigchld(int sig, siginfo_t *info, void *ucontext)
{
    (void)sig;
    (void)info;
    (void)ucontext;

    wait(&spawn_sigdata->status);
    spawn_sigdata->pid = 0;
}

/* Simple. Needs to be more robust. */
static void spawn_exec(const char *path)
{
    char *s = strdup(path);
    char *p;
    char *argv[MAXARG+1];
    int n = 0;
    int err;

    assert(s);

    memset(argv, 0, sizeof(argv));
    argv[n] = p = s;
    n++;
    while ((p = strchr(p, ' ')) != NULL) {
        /* Terminate arg word. */
        *p++ = 0;
        assert(n<=MAXARG);
        argv[n] = p;
        n++;
    }

    err = execvp(argv[0], argv);
    assert(!err);
    free(s);
}

int spawn(const char *path, struct spawn *spawn)
{
    int pstdin[2];      /* 0:read 1:write */
    int pstdout[2];
    int pstderr[2];
    int err = 1;

    assert(path);
    assert(spawn);

    /* Save for signal handling */
    assert(!spawn_sigdata);
    spawn_sigdata = spawn;

    /* Create pipes for IO */
    err = pipe(pstdin);
    assert(!err);
    err = pipe(pstdout);
    assert(!err);
    err = pipe(pstderr);
    assert(!err);

    /* caller (parent) side pipes */
    spawn->stdin = pstdin[1];
    spawn->stdout = pstdout[0];
    spawn->stderr = pstderr[0];

    assert(!cloexec(pstdin[1]));
    assert(!cloexec(pstdout[0]));
    assert(!cloexec(pstderr[0]));

    spawn->pid = fork();
    if (spawn->pid) {
        struct sigaction sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = spawn_sigchld;
        sa.sa_flags = SA_SIGINFO | SA_RESTART;
        err = sigaction(SIGCHLD, &sa, NULL);

        /* parent */
    } else {
        /* child */
        close(pstdin[1]);
        close(pstdout[0]);
        close(pstderr[0]);
        dup2(pstdin[0], 0);
        dup2(pstdout[1], 1);
        dup2(pstderr[1], 2);
        spawn_exec(path);
    }

    return err;
}

int unspawn(struct spawn *spawn)
{
    assert(spawn);
    if (spawn->pid) {
        kill(spawn->pid, SIGTERM);
        close(spawn->stdin);
        close(spawn->stdout);
        close(spawn->stderr);
    }

    return 0;
}
