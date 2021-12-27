/*
   spawn.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "misc.h"
#include "spawn.h"

#define MAXARG 10

/* Context needed for signal handling. One at a time, please! */
static struct spawn *spawn_sigdata;

static void spawn_sigchld(int sig)
{
    pid_t pid;
    int status;

    (void)sig;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (pid == spawn_sigdata->pid) {
            spawn_sigdata->status = status;
            spawn_sigdata->pid = 0;
        }
    }
}

int spawn(char * const argv[], struct spawn *spawn)
{
    int pstdin[2];      /* 0:read 1:write */
    int pstdout[2];
    int pstderr[2];
    int err = 1;

    assert(argv);
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
        /* parent */
        struct sigaction sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = spawn_sigchld;
        sa.sa_flags = SA_RESTART;
        err = sigaction(SIGCHLD, &sa, NULL);
    } else {
        /* child */
        close(pstdin[1]);
        close(pstdout[0]);
        close(pstderr[0]);
        dup2(pstdin[0], 0);
        dup2(pstdout[1], 1);
        dup2(pstderr[1], 2);
        err = execvp(argv[0], argv);
        assert(!err);
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
