/*
   spawn.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SPAWN_H_
#define   SPAWN_H_

struct spawn {
    pid_t child;
    int stdin;
    int stdout;
    int stderr;
    int status;
};

extern int spawn(const char *path, struct spawn *spawn);

#endif /* SPAWN_H_ */
