/*
   server.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SERVER_H_
#define   SERVER_H_

#include "info.h"

extern int server_run(struct info *info);
extern int server_listen(struct info *info);

#endif /* SERVER_H_ */
