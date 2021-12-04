/*
   worker.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <assert.h>
#include <stdlib.h>
#include "worker.h"

#define MAXW 10

struct worker {
    int count;
    struct {
        int fd;
        wfunc func;
        void *arg;
    } w[MAXW];
};

struct worker *worker_init(void)
{
    struct worker *worker;

    worker = calloc(1,sizeof(*worker));
    assert(worker);

    return worker;
}

void worker_done(struct worker *worker)
{
    if (worker) {
        free(worker);
    }
}

int worker_add(struct worker *worker, int fd, wfunc func, void *arg)
{
    int err = 1;

    assert(worker);
    if (worker->count < MAXW) {
        worker->w[worker->count].fd = fd;
        worker->w[worker->count].func = func;
        worker->w[worker->count].arg = arg;
    }
    return err;
}

int worker_count(struct worker *worker)
{
    assert(worker);
    return worker->count;
}

int worker_getfd(struct worker *worker, int idx)
{
    int fd;
    assert(worker);
    assert(idx >= 0 && idx < worker->count);
    fd = worker->w[idx].fd;
    assert(fd > 0);
    return fd;
}

int worker_call(struct worker *worker, int idx)
{
    assert(worker);
    assert(idx >= 0 && idx < worker->count);
    assert(worker->w[idx].fd > 0);
    return worker->w[idx].func(worker->w[idx].arg);
}

int worker_remove(struct worker *worker, int idx)
{
    assert(worker);
    assert(idx >= 0 && idx < worker->count);
    worker->w[idx].fd = -1;
    if (idx == (worker->count-1)) {
        /* last */
        --worker->count;
    }


    return 0;
}
