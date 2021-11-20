
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
%option fast
%option debug
/*%option perf-report*/
%option outfile="scanner.c" header-file="scanner.h"

%{
#include "scanner_intf.h"

#pragma GCC diagnostic ignored "-Wunused-function"
%}

%%

\*CLS                   { return CLS; }
\*ESE                   { return ESE; }
\*ESE\?                 { return ESEQ; }
\*ESR\?                 { return ESRQ; }
\*IDN\?                 { return IDNQ; }
\*OPC                   { return OPC; }
\*OPC\?                 { return OPCQ; }
\*RST                   { return RST; }
\*SRE                   { return SRE; }
\*SRE\?                 { return SREQ; }
\*STB\?                 { return STBQ; }
\*TST\?                 { return TSTQ; }
\WAI*                   { return WAI; }
ABOR|ABORt              { return ABOR; }
CAL|CALibrate           { return CAL; }
(CAP|CAPability)\?      { return CAPQ; }
COMM|COMMunicate        { return COMM; }
(COND|CONDition)\?      { return CONDQ; }
(CONF|CONFigure)        { return CONF; }
(CONF|CONFigure)\?      { return CONFQ; }
(CONT|CONTrol)\?        { return CONTQ; }
(COUN|COUNt)\?          { return COUNQ; }
(DAT|DATa)              { return DAT; }
(DAT|DATa)\?            { return DATQ; }
(DIG|DIGtal)            { return DIG; }
(ENAB|ENABle)           { return ENAB; }
(ENAB|ENABle)\?         { return ENABQ; }
(ERR|ERRor)             { return ERR; }
(ERR|ERRor)\?           { return ERRQ; }
(EVEN|EVENt)\?          { return EVENQ; }
(FETC|FETCh)            { return FETC; }
(INIT|INITiate)         { return INIT; }
(INT|INTernal)          { return INT; }
(MEAS|MEASure)          { return MEAS; }
NEXT\?                  { return NEXTQ; }
(OPER|OPERation)        { return OPER; }
(OPER|OPERation)\?      { return OPERQ; }
(PRES|PRESet)           { return PRES; }
(QUES|QUEStionable)     { return QUES; }
(QUES|QUEStionable)\?   { return QUESQ; }
QUIT                    { return QUIT; }
READ                    { return READ; }
(SET|SETup)\?           { return SETUQ; }
SHOW\?                  { return SHOWQ; }
(STAT|STATus)           { return STAT; }
(SYST|SYSTem)           { return SYST; }
(TCP|TCPip)             { return TCP; }
(VERS|VERSion)\?        { return VERSQ; }
\(                      { return LPAREN; }
\)                      { return RPAREN; }
,                       { return COMMA; }
;                       { return SEMIS; }
:                       { return COLON; }
-                       { return DASH; }
@                       { return AT; }
[[:digit:]]+            { return parser_num(yytext, yylval, NUM); }
[[:alpha:]]+[[:alnum:]]* { return IDENT; }
[[:blank:]]*
\n
\0                      { return YYEOF; }
.                       { return OTHER; }
