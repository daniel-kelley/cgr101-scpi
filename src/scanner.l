
%option 8bit
%option reentrant
%option bison-bridge
%option bison-locations
%option warn
/*%option nodefault*/
/*%option yylineno*/
%option case-insensitive
%option noyywrap
%option noyyrealloc
%option fast
%option debug
/*%option perf-report*/
%option outfile="scanner.c" header-file="scanner.h"

%x dstr
%x sstr

%{
#include <assert.h>
#include "scanner_intf.h"

#pragma GCC diagnostic ignored "-Wunused-function"

/*
 * Use our own yyrealloc to suppress the scan-build complaint of using a zero
 * length buffer. We simply assert that the realloc size is non-zero.
 */
void *yyrealloc(void *ptr, size_t size, yyscan_t yyscanner)
{
    (void)yyscanner;
    assert(size);
    return realloc(ptr, size);
}

%}

%%

\"                      { parser_dstr_new(yytext, yylval, yylloc); BEGIN(dstr); }
<dstr>\"                {
                          BEGIN(INITIAL);
                          return parser_dstr(yytext, yylval, yylloc, STRING);
                        }
<dstr>\"\"              { parser_dstr_quote(yytext, yylval, yylloc); }
<dstr>.                 { parser_dstr_add(yytext, yylval, yylloc); }

\'                      { parser_dstr_new(yytext, yylval, yylloc); BEGIN(sstr); }
<sstr>\'                {
                          BEGIN(INITIAL);
                          return parser_dstr(yytext, yylval, yylloc, STRING);
                        }
<sstr>\'\'              { parser_dstr_quote(yytext, yylval, yylloc); }
<sstr>.                 { parser_dstr_add(yytext, yylval, yylloc); }

\*CLS                   { return parser_ident(yytext, yylval, yylloc, CLS); }
\*ESE                   { return parser_ident(yytext, yylval, yylloc, ESE); }
\*ESE\?                 { return parser_ident(yytext, yylval, yylloc, ESEQ); }
\*ESR\?                 { return parser_ident(yytext, yylval, yylloc, ESRQ); }
\*IDN\?                 { return parser_ident(yytext, yylval, yylloc, IDNQ); }
\*OPC                   { return parser_ident(yytext, yylval, yylloc, OPC); }
\*OPC\?                 { return parser_ident(yytext, yylval, yylloc, OPCQ); }
\*RST                   { return parser_ident(yytext, yylval, yylloc, RST); }
\*SRE                   { return parser_ident(yytext, yylval, yylloc, SRE); }
\*SRE\?                 { return parser_ident(yytext, yylval, yylloc, SREQ); }
\*STB\?                 { return parser_ident(yytext, yylval, yylloc, STBQ); }
\*TST\?                 { return parser_ident(yytext, yylval, yylloc, TSTQ); }
\*WAI                   { return parser_ident(yytext, yylval, yylloc, WAI); }
ABOR|ABORt              { return parser_ident(yytext, yylval, yylloc, ABOR); }
ALL                     { return parser_ident(yytext, yylval, yylloc, ALL); }
ASC|ASCii               { return parser_ident(yytext, yylval, yylloc, ASC); }
BIN|BINary              { return parser_ident(yytext, yylval, yylloc, BIN); }
CAL|CALibrate           { return parser_ident(yytext, yylval, yylloc, CAL); }
(CAP|CAPability)\?      { return parser_ident(yytext, yylval, yylloc, CAPQ); }
COMM|COMMunicate        { return parser_ident(yytext, yylval, yylloc, COMM); }
CONC|CONCurrent         { return parser_ident(yytext, yylval, yylloc, CONC); }
(COND|CONDition)\?      { return parser_ident(yytext, yylval, yylloc, CONDQ); }
(CONF|CONFigure)        { return parser_ident(yytext, yylval, yylloc, CONF); }
(CONF|CONFigure)\?      { return parser_ident(yytext, yylval, yylloc, CONFQ); }
(CONT|CONTrol)\?        { return parser_ident(yytext, yylval, yylloc, CONTQ); }
(COUN|COUNt)\?          { return parser_ident(yytext, yylval, yylloc, COUNQ); }
COUP|COUPling           { return parser_ident(yytext, yylval, yylloc, COUP); }
CW                      { return parser_ident(yytext, yylval, yylloc, CW); }
(DAT|DATa)              { return parser_ident(yytext, yylval, yylloc, DAT); }
(DAT|DATa)\?            { return parser_ident(yytext, yylval, yylloc, DATQ); }
DC                      { return parser_ident(yytext, yylval, yylloc, DC); }
(DCYC|DCYcle)           { return parser_ident(yytext, yylval, yylloc, DCYC); }
(DCYC|DCYcLe)\?         { return parser_ident(yytext, yylval, yylloc, DCYCQ); }
DEF                     { return parser_ident(yytext, yylval, yylloc, DEF); }
(DIG|DIGital)           { return parser_ident(yytext, yylval, yylloc, DIG); }
ECHO                    { return parser_ident(yytext, yylval, yylloc, ECHO_); }
(ENAB|ENABle)           { return parser_ident(yytext, yylval, yylloc, ENAB); }
(ENAB|ENABle)\?         { return parser_ident(yytext, yylval, yylloc, ENABQ); }
(ERR|ERRor)             { return parser_ident(yytext, yylval, yylloc, ERR); }
(ERR|ERRor)\?           { return parser_ident(yytext, yylval, yylloc, ERRQ); }
(EVEN|EVENt)            { return parser_ident(yytext, yylval, yylloc, EVEN); }
(EVEN|EVENt)\?          { return parser_ident(yytext, yylval, yylloc, EVENQ); }
(EXT|EXTernal)          { return parser_ident(yytext, yylval, yylloc, EXT); }
(FETC|FETCh)            { return parser_ident(yytext, yylval, yylloc, FETC); }
(FIX|FIXed)             { return parser_ident(yytext, yylval, yylloc, FIX); }
(FORM|FORMat)           { return parser_ident(yytext, yylval, yylloc, FORM); }
(FORM|FORMat)\?         { return parser_ident(yytext, yylval, yylloc, FORMQ); }
(FREQ|FREQuency)        { return parser_ident(yytext, yylval, yylloc, FREQ); }
(FREQ|FREQuency)\?      { return parser_ident(yytext, yylval, yylloc, FREQQ); }
(FUNC|FUNCtion)         { return parser_ident(yytext, yylval, yylloc, FUNC); }
(FUNC|FUNCtion)\?       { return parser_ident(yytext, yylval, yylloc, FUNCQ); }
(HEX|HEXadecimal)       { return parser_ident(yytext, yylval, yylloc, HEX); }
(IMM|IMMediate)         { return parser_ident(yytext, yylval, yylloc, IMM); }
(INCL|INCLUDE)          { return parser_ident(yytext, yylval, yylloc, INCL); }
(INIT|INITiate)         { return parser_ident(yytext, yylval, yylloc, INIT); }
(INP|INPut)             { return parser_ident(yytext, yylval, yylloc, INP); }
INT                     { return parser_ident(yytext, yylval, yylloc, INT); }
INTeger                 { return parser_ident(yytext, yylval, yylloc, INTEGER); }
INTernal                { return parser_ident(yytext, yylval, yylloc, INTERNAL); }
(LEV|LEVel)             { return parser_ident(yytext, yylval, yylloc, LEV); }
(LEV|LEVel)\?           { return parser_ident(yytext, yylval, yylloc, LEVQ); }
(LOC|LOCation)          { return parser_ident(yytext, yylval, yylloc, LOC); }
(LOC|LOCation)\?        { return parser_ident(yytext, yylval, yylloc, LOCQ); }
(LOW|LOWer)             { return parser_ident(yytext, yylval, yylloc, LOW); }
(LOW|LOWer)\?           { return parser_ident(yytext, yylval, yylloc, LOWQ); }
MAX                     { return parser_ident(yytext, yylval, yylloc, MAX); }
(MEAS|MEASure)          { return parser_ident(yytext, yylval, yylloc, MEAS); }
MIN                     { return parser_ident(yytext, yylval, yylloc, MIN); }
(NEG|NEGative)          { return parser_ident(yytext, yylval, yylloc, NEG); }
NEXT\?                  { return parser_ident(yytext, yylval, yylloc, NEXTQ); }
(OCT|OCTal)             { return parser_ident(yytext, yylval, yylloc, OCT); }
NONE                    { return parser_ident(yytext, yylval, yylloc, NONE); }
OFF                     { return parser_ident(yytext, yylval, yylloc, OFF); }
(OFFS|OFFSet)           { return parser_ident(yytext, yylval, yylloc, OFFS); }
(OFFS|OFFSet)\?         { return parser_ident(yytext, yylval, yylloc, OFFSQ); }
ON                      { return parser_ident(yytext, yylval, yylloc, ON); }
(OPER|OPERation)        { return parser_ident(yytext, yylval, yylloc, OPER); }
(OPER|OPERation)\?      { return parser_ident(yytext, yylval, yylloc, OPERQ); }
(OREF|OREFERENCE)       { return parser_ident(yytext, yylval, yylloc, OREF); }
PACK                    { return parser_ident(yytext, yylval, yylloc, PACK); }
(POIN|POINts)           { return parser_ident(yytext, yylval, yylloc, POIN); }
(POIN|POINts)\?         { return parser_ident(yytext, yylval, yylloc, POINQ); }
(POS|POSitive)          { return parser_ident(yytext, yylval, yylloc, POS); }
(PRES|PRESet)           { return parser_ident(yytext, yylval, yylloc, PRES); }
(PTP|PTPeak)            { return parser_ident(yytext, yylval, yylloc, PTP); }
(PTP|PTPeak)\?          { return parser_ident(yytext, yylval, yylloc, PTPQ); }
(PULS|PULSe)            { return parser_ident(yytext, yylval, yylloc, PULS); }
(QUES|QUEStionable)     { return parser_ident(yytext, yylval, yylloc, QUES); }
(QUES|QUEStionable)\?   { return parser_ident(yytext, yylval, yylloc, QUESQ); }
QUIT                    { return parser_ident(yytext, yylval, yylloc, QUIT); }
(RAND|RANDom)           { return parser_ident(yytext, yylval, yylloc, RAND); }
(RANG|RANGe)            { return parser_ident(yytext, yylval, yylloc, RANG); }
(RANG|RANGe)\?          { return parser_ident(yytext, yylval, yylloc, RANGQ); }
READ                    { return parser_ident(yytext, yylval, yylloc, READ); }
REAL                    { return parser_ident(yytext, yylval, yylloc, REAL); }
(RES|RESet)             { return parser_ident(yytext, yylval, yylloc, RES); }
(SENS|SENSe)            { return parser_ident(yytext, yylval, yylloc, SENS); }
(SET|SETup)\?           { return parser_ident(yytext, yylval, yylloc, SETUQ); }
(SHAP|SHAPe)            { return parser_ident(yytext, yylval, yylloc, SHAP); }
SHOW\?                  { return parser_ident(yytext, yylval, yylloc, SHOWQ); }
(SIN|SINusoid)          { return parser_ident(yytext, yylval, yylloc, SIN); }
(SLE|SLEep)             { return parser_ident(yytext, yylval, yylloc, SLE); }
(SLOP|SLOPe)            { return parser_ident(yytext, yylval, yylloc, SLOP); }
(SLOP|SLOPe)\?          { return parser_ident(yytext, yylval, yylloc, SLOPQ); }
(SOUR|SOURce)           { return parser_ident(yytext, yylval, yylloc, SOUR); }
(SOUR|SOURce)\?         { return parser_ident(yytext, yylval, yylloc, SOURQ); }
(SQU|SQUare)            { return parser_ident(yytext, yylval, yylloc, SQU); }
STATe                   { return parser_ident(yytext, yylval, yylloc, STATE); }
STATe\?                 { return parser_ident(yytext, yylval, yylloc, STATEQ); }
STAT                    { return parser_ident(yytext, yylval, yylloc, STAT); }
STAT\?                  { return parser_ident(yytext, yylval, yylloc, STATQ); }
STATUS                  { return parser_ident(yytext, yylval, yylloc, STATUS); }
(STOR|STORe)            { return parser_ident(yytext, yylval, yylloc, STOR); }
(SWE|SWEep)             { return parser_ident(yytext, yylval, yylloc, SWE); }
(SYST|SYSTem)           { return parser_ident(yytext, yylval, yylloc, SYST); }
(TCP|TCPip)             { return parser_ident(yytext, yylval, yylloc, TCP); }
TIME                    { return parser_ident(yytext, yylval, yylloc, TIME); }
TIME\?                  { return parser_ident(yytext, yylval, yylloc, TIMEQ); }
(TINT|TINTerval)        { return parser_ident(yytext, yylval, yylloc, TINT); }
(TINT|TINTerval)\?      { return parser_ident(yytext, yylval, yylloc, TINTQ); }
(TRI|TRIangle)          { return parser_ident(yytext, yylval, yylloc, TRI); }
(TRIG|TRIGger)          { return parser_ident(yytext, yylval, yylloc, TRIG); }
UINT                    { return parser_ident(yytext, yylval, yylloc, UINT); }
(UPP|UPPer)             { return parser_ident(yytext, yylval, yylloc, UPP); }
(UPP|UPPer)\?           { return parser_ident(yytext, yylval, yylloc, UPPQ); }
USER                    { return parser_ident(yytext, yylval, yylloc, USER); }
USER\?                  { return parser_ident(yytext, yylval, yylloc, USERQ); }
(VERS|VERSion)\?        { return parser_ident(yytext, yylval, yylloc, VERSQ); }
(VOLT|VOLTage)          { return parser_ident(yytext, yylval, yylloc, VOLT); }
\(                      { return parser_punct(yytext, yylval, yylloc, LPAREN); }
\)                      { return parser_punct(yytext, yylval, yylloc, RPAREN); }
,                       { return parser_punct(yytext, yylval, yylloc, COMMA); }
;                       { return parser_punct(yytext, yylval, yylloc, SEMIS); }
:                       { return parser_punct(yytext, yylval, yylloc, COLON); }
-                       { return parser_punct(yytext, yylval, yylloc, DASH); }
@                       { return parser_punct(yytext, yylval, yylloc, AT); }

^[[:blank:]]*#.*        /* # comment */
[+-]*[[:digit:]]*\.[[:digit:]]+ { return parser_num(yytext, yylval, yylloc, FLOAT); }

[+-]*[[:digit:]]+\.[[:digit:]]* { return parser_num(yytext, yylval, yylloc, FLOAT); }

[+-]*[[:digit:]]*\.[[:digit:]]+[[:blank:]]*[eE]?[[:blank:]]*[+-]*[[:digit:]]+ { return parser_num(yytext, yylval, yylloc, FLOAT); }

[+-]*[[:digit:]]+\.[[:digit:]]*[[:blank:]]*[eE]?[[:blank:]]*[+-]*[[:digit:]]+ { return parser_num(yytext, yylval, yylloc, FLOAT); }

[+-]*[[:digit:]]+            { return parser_num(yytext, yylval, yylloc, NUM); }
[[:alpha:]]+[[:alnum:]]* { return parser_ident(yytext, yylval, yylloc, IDENT); }
[[:blank:]]*            { parser_blank(yytext, yylval, yylloc); }
\n                      { parser_eol(yytext, yylval, yylloc); }
\0                      { return parser_eof(yyextra, yytext, EOF_); }
.                       { return parser_punct(yytext, yylval, yylloc, OTHER); }
