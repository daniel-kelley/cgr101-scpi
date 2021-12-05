/*
   worker.h

   Copyright (c) 2021 by Daniel Kelley

   Worker functions. This uses a file descriptor to indicate via
   select() when some work is ready to be done.

*/

#ifndef   WORKER_H_
#define   WORKER_H_

struct worker;

typedef int (*wfunc)(void *arg);

extern struct worker *worker_init(void);
extern void worker_done(struct worker *worker);
extern int worker_add(struct worker *worker, int fd, wfunc func, void *arg);
extern int worker_count(struct worker *worker);
extern int worker_getfd(struct worker *worker, int idx);
extern int worker_ready(struct worker *worker, int idx);
extern int worker_remove(struct worker *worker, int idx);
extern int worker_run_ready(struct worker *worker);

#endif /* WORKER_H_ */
