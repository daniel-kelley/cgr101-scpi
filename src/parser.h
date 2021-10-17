/*
   parser.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   PARSER_H_
#define   PARSER_H_

#include "info.h"

extern void parser_input(
    struct info *info,
    char *buf,
    int result,
    int max_size);

#endif /* PARSER_H_ */
