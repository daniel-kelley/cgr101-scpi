/*
   misc.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <fcntl.h>
#include "misc.h"

int cloexec(int fd)
{
    return fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
}

