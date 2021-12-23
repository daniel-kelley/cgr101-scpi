/*
   spawn.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SPAWN_H_
#define   SPAWN_H_

struct spawn {
    pid_t pid;
    int stdin;
    int stdout;
    int stderr;
    int status;
};

extern int spawn(char *const argv[], struct spawn *spawn);
extern int unspawn(struct spawn *spawn);

#endif /* SPAWN_H_ */
