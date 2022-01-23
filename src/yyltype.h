/*
   yyltype.h

   Copyright (c) 2022 by Daniel Kelley

*/

#ifndef   YYLTYPE_H_
#define   YYLTYPE_H_

#define YYLTYPE YYLTYPE
typedef struct YYLTYPE
{
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} YYLTYPE;

#endif /* YYLTYPE_H_ */
