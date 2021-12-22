/*
   cgr101.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   CGR101_H_
#define   CGR101_H_

#include "info.h"

extern int cgr101_identify(struct info *info);
extern int cgr101_open(struct info *);
extern int cgr101_close(struct info *);

#endif /* CGR101_H_ */
