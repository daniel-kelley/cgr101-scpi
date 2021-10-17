/* SCPI -*-text-*- */
/*

  SCPI grammar for CGR-101

*/

%{
    /*PROLOGUE*/

%}

%token IDENT
%token NUM
%token LPAREN  "("
%token RPAREN  ")"
%token COMMA   ","
%token SEMIS   ";"
%token COLON   ":"

%token ABOR
%token CAL
%token CAPQ
%token CLS
%token COMM
%token CONDQ
%token CONF
%token CONFQ
%token CONTQ
%token COUNQ
%token DAT
%token DATQ
%token DIG
%token ENAB
%token ENABQ
%token ERR
%token ERRQ
%token ESE
%token ESEQ
%token ESRQ
%token EVENQ
%token FETC
%token IDNQ
%token INIT
%token INT
%token MEAS
%token NEXTQ
%token OPC
%token OPCQ
%token OPER
%token OPERQ
%token PRES
%token QUIT
%token READ
%token RST
%token SETUQ
%token SHOWQ
%token SRE
%token SREQ
%token STAT
%token STBQ
%token SYST
%token TCPI
%token TSTQ
%token VERSQ
%token WAI

%start top

%locations

%define api.pure full
%define api.push-pull push
%define api.prefix {scpi_yy}
%define parse.assert
%define parse.trace
%define parse.error detailed
%define parse.lac full

%{
    #include "scpi.h"
%}

%%

top
    : cmd-list
    ;


cmd-list
    : cmd
    | cmd-list SEMIS cmd
    ;


cmd
    : sys-cmd
    | dev-cmd
    ;

/*
 * IEEE 488.2 elements
 *
 * PROGRAM_DATA
 *    7.7.1.2 CHARACTER (IDENT: [A-Za-z]+[A-Za-z0-9_]*)
 *    7.7.2.2 DECIMAL_NUMERIC
 *            MANTISSA|MANTISSA[ ]*EXPONENT
 *              MANTISSA [+-]*([0-9]+ | [0-9]*\.[0-9]+)
 *              EXPONENT [Ee][ ]*[+-]*[0-9]+
 *    7.7.3.2 SUFFIX (ignore for now)
 *    7.7.4.2 NONDECIMAL_NUMERIC
 *              #[Hh][A-Fa-f0-9]+
 *              #[Qq][0-7]+
 *              #[Bb][0-1]+
 *
 *    7.7.5.2 STRING
 *              '[^']|('')'
 *              "[^"]|("")"
 *    7.7.6.2 ARBITRARY_BLOCK (#0 disallowed by SCPI)
 *              #[0-9]+<byte>+
 *    7.7.7.2 EXPRESSION (ignore for now)
 */


sys-cmd
    /* COMMAND_PROGRAM_HEADER */
    : CLS
    { /*scpi_common_cls();*/ }
/* COMMAND PROGRAM HEADER ROGRAM HEADER SEPARATOR */
    | ESE NUM
    { /*scpi_common_cls();*/ }
    | ESEQ
    | ESRQ
    | IDNQ
    | OPC
    | OPCQ
    | RST
    | SRE
    | SREQ
    | STBQ
    | TSTQ
    | WAI
    | SYST COLON ERRQ
    | SYST COLON ERR COLON NEXTQ
    | SYST COLON ERR COLON COUNQ
    | SYST COLON VERSQ
    | SYST COLON CAPQ
    | STAT COLON OPERQ
    | STAT COLON OPER COLON EVENQ
    | STAT COLON OPER COLON CONDQ
    | STAT COLON OPER COLON ENAB
    | STAT COLON OPER COLON ENABQ
    | STAT COLON PRES
    ;

dev-cmd
    : MEAS COLON DIG COLON DATQ
    | ABOR
    | CONFQ
    | CONF COLON DIG COLON DAT
    | READ COLON DIG COLON DATQ
    | FETC COLON DIG COLON DATQ
    | INIT
    | SYST COLON COMM COLON TCPI COLON CONTQ
    | SYST COLON INT COLON QUIT
    | SYST COLON INT COLON CAL
    | SYST COLON INT COLON CONF
    | SYST COLON INT COLON SHOWQ
    | SYST COLON INT COLON SETUQ
    ;

%%

/*EPILOGUE*/

