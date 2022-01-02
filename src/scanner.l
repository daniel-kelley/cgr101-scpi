
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
ALL                     { return ALL; }
ASC|ASCii               { return ASC; }
BIN|BINary              { return BIN; }
CAL|CALibrate           { return CAL; }
(CAP|CAPability)\?      { return CAPQ; }
COMM|COMMunicate        { return COMM; }
CONC|CONCurrent         { return CONC; }
(COND|CONDition)\?      { return CONDQ; }
(CONF|CONFigure)        { return CONF; }
(CONF|CONFigure)\?      { return CONFQ; }
(CONT|CONTrol)\?        { return CONTQ; }
(COUN|COUNt)\?          { return COUNQ; }
COUP|COUPling           { return COUP; }
CW                      { return CW; }
(DAT|DATa)              { return DAT; }
(DAT|DATa)\?            { return DATQ; }
DC                      { return DC; }
(DCYC|DCYcle)           { return DCYC; }
(DCYC|DCYcLe)\?         { return DCYCQ; }
DEF                     { return DEF; }
(DIG|DIGital)           { return DIG; }
(ENAB|ENABle)           { return ENAB; }
(ENAB|ENABle)\?         { return ENABQ; }
(ERR|ERRor)             { return ERR; }
(ERR|ERRor)\?           { return ERRQ; }
(EVEN|EVENt)\?          { return EVENQ; }
(EXT|EXTernal)          { return EXT; }
(FETC|FETCh)            { return FETC; }
(FIX|FIXed)             { return FIX; }
(FORM|FORMat)           { return FORM; }
(FORM|FORMat)\?         { return FORMQ; }
(FREQ|FREQuency)        { return FREQ; }
(FREQ|FREQuency)\?      { return FREQQ; }
(FUNC|FUNCtion)         { return FUNC; }
(FUNC|FUNCtion)\?       { return FUNCQ; }
(HEX|HEXadecimal)       { return HEX; }
(IMM|IMMediate)         { return IMM; }
(INIT|INITiate)         { return INIT; }
(INP|INPut)             { return INP; }
INT                     { return INT; }
INTeger                 { return INTEGER; }
INTernal                { return INTERNAL; }
(LEV|LEVel)             { return LEV; }
(LEV|LEVel)\?           { return LEVQ; }
(LOW|LOWer)             { return LOW; }
(LOW|LOWer)\?           { return LOWQ; }
MAX                     { return MAX; }
(MEAS|MEASure)          { return MEAS; }
MIN                     { return MIN; }
(NEG|NEGative)          { return NEG; }
NEXT\?                  { return NEXTQ; }
(OCT|OCTal)             { return OCT; }
OFF                     { return OFF; }
(OFFS|OFFSet)           { return OFFS; }
(OFFS|OFFSet)\?         { return OFFSQ; }
ON                      { return ON; }
(OPER|OPERation)        { return OPER; }
(OPER|OPERation)\?      { return OPERQ; }
PACK                    { return PACK; }
(POIN|POINts)           { return POIN; }
(POS|POSitive)          { return POS; }
(PRES|PRESet)           { return PRES; }
(PTP|PTPeak)            { return PTP; }
(PTP|PTPeak)\?          { return PTPQ; }
(PULS|PULSe)            { return PULS; }
(QUES|QUEStionable)     { return QUES; }
(QUES|QUEStionable)\?   { return QUESQ; }
QUIT                    { return QUIT; }
(RAND|RANDom)           { return RAND; }
(RANG|RANGe)            { return RANG; }
(RANG|RANGe)\?          { return RANGQ; }
READ                    { return READ; }
REAL                    { return REAL; }
(RES|RESet)             { return RES; }
(SENS|SENSe)            { return SENS; }
(SET|SETup)\?           { return SETUQ; }
(SHAP|SHAPe)            { return SHAP; }
SHOW\?                  { return SHOWQ; }
(SIN|SINusoid)          { return SIN; }
(SLOP|SLOPe)            { return SLOP; }
(SLOP|SLOPe)\?          { return SLOPQ; }
(SOUR|SOURce)           { return SOUR; }
(SOUR|SOURce)\?         { return SOURQ; }
(SQU|SQUare)            { return SQU; }
STATe                   { return STATE; }
STATe\?                 { return STATEQ; }
STAT                    { return STAT; }
STAT\?                  { return STATQ; }
STATUS                  { return STATUS; }
(SWE|SWEep)             { return SWE; }
(SYST|SYSTem)           { return SYST; }
(TCP|TCPip)             { return TCP; }
TIME                    { return TIME; }
(TINT|TINTerval)        { return TINT; }
(TRI|TRIangle)          { return TRI; }
(TRIG|TRIGger)          { return TRIG; }
UINT                    { return UINT; }
(UPP|UPPer)             { return UPP; }
(UPP|UPPer)\?           { return UPPQ; }
USER                    { return USER; }
USER\?                  { return USERQ; }
(VERS|VERSion)\?        { return VERSQ; }
(VOLT|VOLTage)          { return VOLT; }
\(                      { return LPAREN; }
\)                      { return RPAREN; }
,                       { return COMMA; }
;                       { return SEMIS; }
:                       { return COLON; }
-                       { return DASH; }
@                       { return AT; }
[+-]+[[:digit:]]+       { return parser_num(yytext, yylval, FLOAT); }
[+-]+[[:digit:]]+\.[[:digit:]]* { return parser_num(yytext, yylval, FLOAT); }
[+-]+[[:digit:]]+\.[[:digit:]]*[eE]?[[:blank:]]*[+-]+[[:digit:]]* { return parser_num(yytext, yylval, FLOAT); }
[[:digit:]]+            { return parser_num(yytext, yylval, NUM); }
[[:alpha:]]+[[:alnum:]]* { return IDENT; }
[[:blank:]]*
\n
\0                      { return YYEOF; }
.                       { return OTHER; }
