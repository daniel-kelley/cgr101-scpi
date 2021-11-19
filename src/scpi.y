/* SCPI -*-text-*- */
/*

  SCPI grammar for CGR-101

*/

%{
    /*PROLOGUE*/

%}

%token IDENT
%token NUM
%token LPAREN
%token RPAREN
%token COMMA
%token SEMIS
%token COLON
%token DASH
%token AT

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
%token TCP
%token TSTQ
%token VERSQ
%token WAI
%token OTHER

%start top

%locations

%define api.pure full
%define api.push-pull push
%define parse.assert
%define parse.trace
%define parse.error detailed
%define parse.lac full
%param {struct info *info}
%define api.value.type {struct scpi_type}

%{
    #include "scpi.h"
%}

%%

top
    : cmd-list
    { scpi_core_top(info); }
    ;

cmd-list
    : %empty
    | cmd-path
    | cmd-list semis cmd-path
    ;

semis
    : SEMIS
    { scpi_core_cmd_sep(info); }
    ;

cmd-path
    : cmd-first
    | cmd-first cmd-rest
    ;

cmd-first
    : cmd-abs
    | cmd
    ;

cmd-rest
    : cmd-abs
    | cmd-rest cmd-abs
    ;

cmd-abs
    : COLON cmd
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
    /* 488.2 10.3 */
    : CLS
    { scpi_common_cls(info); }

    /* 488.2 10.10 */
    | ESE NUM
    { scpi_common_ese(&$2, info); }

    /* 488.2 10.11 */
    | ESEQ
    { scpi_common_eseq(info); }

    /* 488.2 10.11 */
    | ESRQ
    { /*scpi_common_esrq();*/ }

    /* 488.2 10.14 */
    | IDNQ
    { /*scpi_common_idnq();*/ }

    /* 488.2 10.18 */
    | OPC
    { /*scpi_common_opc();*/ }

    /* 488.2 10.19 */
    | OPCQ
    { /*scpi_common_opcq();*/ }

    /* 488.2 10.32 */
    | RST
    { /*scpi_common_rst();*/ }

    /* 488.2 10.34 */
    | SRE NUM
    { /*scpi_common_sre($2);*/ }


    /* 488.2 10.35 */
    | SREQ
    { /*scpi_common_sreq($2);*/ }

    /* 488.2 10.36 */
    | STBQ
    { /*scpi_common_stbq($2);*/ }

    /* 488.2 10.38 */
    | TSTQ
    { /*scpi_common_tstq($2);*/ }

    /* 488.2 10.39 */
    | WAI
    { /*scpi_common_wai($2);*/ }

    | SYST COLON ERRQ
    { scpi_system_error_nextq(info); }

    | SYST COLON ERR COLON NEXTQ
    { scpi_system_error_nextq(info); }

    | SYST COLON ERR COLON COUNQ
    { scpi_system_error_countq(info); }

    | SYST COLON VERSQ
    { scpi_system_versionq(info); }

    | SYST COLON CAPQ
    { scpi_system_capabilityq(info); }

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
    | SYST COLON COMM COLON TCP COLON CONTQ

    | SYST COLON INT COLON QUIT
    { scpi_system_internal_quit(info); }

    | SYST COLON INT COLON CAL
    | SYST COLON INT COLON CONF
    | SYST COLON INT COLON SHOWQ

    | SYST COLON INT COLON SETUQ
    { scpi_system_internal_setupq(info); }
    ;

%%

/*EPILOGUE*/

