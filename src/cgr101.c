/*
   cgr101.c

   Copyright (c) 2021 by Daniel Kelley

*/

#include <string.h>
#include "cgr101.h"

int cgr101_open(struct info *info)
{
    if (info->debug && strchr(info->debug,'E')) {
        info->emulation = 1;
    }

    /* Only emulation for now. */
    return !(info->emulation == 1);
}

int cgr101_close(struct info *info)
{
    (void)info;
    return 0;
}

