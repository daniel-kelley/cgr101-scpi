/*
   scanner_intf.h

   Copyright (c) 2021 by Daniel Kelley

*/

#ifndef   SCANNER_INTF_H_
#define   SCANNER_INTF_H_

#include "scpi.tab.h"
#include "parser.h"

#define YYSTYPE SCPI_YYSTYPE
#define YYLTYPE SCPI_YYLTYPE
#define YY_EXTRA_TYPE struct info *
#define YY_INPUT(buf,result,max_size) \
    parser_input(yyextra, buf,result,max_size)

#endif /* SCANNER_INTF_H_ */
