/*
   yyerror.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   YYERROR_H_
#define   YYERROR_H_

#include "yyltype.h"

extern void yyerror(const YYLTYPE *loc, struct info *info, const char *s);

#endif /* YYERROR_H_ */
