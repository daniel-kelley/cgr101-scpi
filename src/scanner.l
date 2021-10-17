
%option 8bit
%option reentrant
%option bison-bridge
%option bison-locations
%option warn
/*%option nodefault*/
%option yylineno
%option case-insensitive
%option stack
%option noyywrap
%option prefix="scanner"
%option fast
%option debug
/*%option perf-report*/
%option outfile="scanner.c" header-file="scanner.h"

%{
#include "parser.h"

#pragma GCC diagnostic ignored "-Wunused-function"
%}

%%
