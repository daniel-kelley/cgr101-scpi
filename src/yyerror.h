/*
   yyerror.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   YYERROR_H_
#define   YYERROR_H_

#define YYLTYPE YYLTYPE
typedef struct YYLTYPE
{
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} YYLTYPE;

extern void yyerror(const YYLTYPE *loc, struct info *info, const char *s);

#endif /* YYERROR_H_ */
