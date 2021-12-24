/*
   cgr101.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   CGR101_H_
#define   CGR101_H_

#include "info.h"

#define CGR101_MIN_CHAN 1
#define CGR101_MAX_CHAN 21

extern int cgr101_identify(struct info *info);
extern int cgr101_open(struct info *info);
extern int cgr101_close(struct info *info);
extern int cgr101_initiate(struct info *info);
extern int cgr101_configure_digital_data(struct info *info);
extern int cgr101_digital_data_valid(struct info *info);
extern int cgr101_fetch_digital_data(struct info *info);

#endif /* CGR101_H_ */
