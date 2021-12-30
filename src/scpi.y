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
%token OTHER

%token ABOR
%token ASC
%token BIN
%token CAL
%token CAPQ
%token CLS
%token COMM
%token CONC
%token CONDQ
%token CONF
%token CONFQ
%token CONTQ
%token COUNQ
%token COUP
%token CW
%token DAT
%token DATQ
%token DC
%token DCYC
%token DCYCQ
%token DEF
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
%token FIX
%token FLOAT
%token FORM
%token FORMQ
%token FREQ
%token FREQQ
%token FUNC
%token FUNCQ
%token HEX
%token IDNQ
%token INIT
%token INP
%token INT
%token INTEGER
%token INTERNAL
%token LOW
%token LOWQ
%token MAX
%token MEAS
%token MIN
%token NEXTQ
%token OCT
%token OFF
%token OFFS
%token OFFSQ
%token ON
%token OPC
%token OPCQ
%token OPER
%token OPERQ
%token PACK
%token POIN
%token PRES
%token PRET
%token PTP
%token PTPQ
%token PULS
%token QUES
%token QUESQ
%token QUIT
%token RAND
%token RANG
%token RANGQ
%token READ
%token REAL
%token RST
%token SENS
%token SETUQ
%token SHAP
%token SHOWQ
%token SIN
%token SOUR
%token SRE
%token SREQ
%token STAT
%token STATQ
%token STATE
%token STATEQ
%token STATUS
%token STBQ
%token SQU
%token SWE
%token SYST
%token TCP
%token TIME
%token TINT
%token TRI
%token TSTQ
%token UINT
%token UPP
%token UPPQ
%token USER
%token USERQ
%token VERSQ
%token VOLT
%token WAI

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
    : cmd
    | COLON cmd
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

nr1: NUM
   ;

nrf: FLOAT
   ;

nrf-list
    : nrf
    | nrf-list COMMA nrf
    { $$ = *scpi_core_nrf_list(info, &$3); }
    ;

symbolic_value
    : MIN
    | MAX
    { $$ = *scpi_core_symbolic_value(info, &$1); }
    ;

numeric_value
    : symbolic_value
    | nr1
    | nrf
    ;

/*
 * A number of keywords have identical short forms, so the
 * long and short forms are handled here in the grammar.
 */
status
    : STATUS
    | STAT
    ;

stateq
    : STATEQ
    | STATQ
    ;

internal
    : INTERNAL
    | INT
    ;

integer
    : INTEGER
    | INT
    ;

boolean
    : ON
    | OFF
    ;

sys-cmd
    /* 488.2 10.3 */
    : CLS
    { scpi_common_cls(info); }

    /* 488.2 10.10 */
    | ESE nr1
    { scpi_common_ese(info, &$2); }

    /* 488.2 10.11 */
    | ESEQ
    { scpi_common_eseq(info); }

    /* 488.2 10.11 */
    | ESRQ
    { scpi_common_esrq(info); }

    /* 488.2 10.14 */
    | IDNQ
    { scpi_common_idnq(info); }

    /* 488.2 10.18 */
    | OPC
    { scpi_common_opc(info); }

    /* 488.2 10.19 */
    | OPCQ
    { scpi_common_opcq(info); }

    /* 488.2 10.32 */
    | RST
    { scpi_common_rst(info); }

    /* 488.2 10.34 */
    | SRE nr1
    { scpi_common_sre(info, &$2); }


    /* 488.2 10.35 */
    | SREQ
    { scpi_common_sreq(info); }

    /* 488.2 10.36 */
    | STBQ
    { scpi_common_stbq(info); }

    /* 488.2 10.38 */
    | TSTQ
    { scpi_common_tstq(info); }

    /* 488.2 10.39 */
    | WAI
    { scpi_common_wai(info); }

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

    /* SCPI Command Reference Chapter 20 */
    | status COLON OPERQ
    { scpi_status_operation_eventq(info); }

    | status COLON OPER COLON EVENQ
    { scpi_status_operation_eventq(info); }

    | status COLON OPER COLON CONDQ
    { scpi_status_operation_conditionq(info); }

    | status COLON OPER COLON ENAB nr1
    { scpi_status_operation_enable(info, &$6); }

    | status COLON OPER COLON ENABQ
    { scpi_status_operation_enableq(info); }

    | status COLON PRES
    { scpi_status_operation_preset(info); }

    | status COLON QUESQ
    { scpi_status_questionableq(info); }

    | status COLON QUES COLON ENAB nr1
    { scpi_status_questionable_enable(info, &$6); }

    | status COLON QUES COLON ENABQ
    { scpi_status_questionable_enableq(info); }

    ;

channel-spec
    : nr1
    { $$ = *scpi_dev_channel_num(info, &$1); }
    ;

channel-range
    : channel-spec
    | channel-spec COLON channel-spec
    { $$ = *scpi_dev_channel_range(info, &$1, &$3); }
    ;

channel-list
    : channel-range
    | channel-list COMMA channel-range
    { $$ = *scpi_dev_channel_range_append(info, &$1, &$3); }
    ;

channel
    : LPAREN AT channel-list RPAREN
    { $$ = $3; }
    ;

format_type
    : ASC
    | BIN
    | HEX
    | integer
    | OCT
    | PACK
    | REAL
    | UINT
    { $$ = $1; }
    ;

format_arg
    : format_type
    { $$ = *scpi_core_format_type(info, &$1, NULL); }
    | format_type COMMA nrf
    { $$ = *scpi_core_format_type(info, &$1, &$3); }
    ;

coupling_arg
    : DC
    { $$ = $1; }
    ;

source_function
    : RAND
    | SIN
    | SQU
    | TRI
    { $$ = $1; }
    ;

block
    : nrf-list
    ;

dev-cmd
    : ABOR
    { scpi_dev_abort(info); }

    | CONF COLON DIG COLON DAT
    { scpi_dev_conf_digital_data(info); }

    | CONFQ
    { scpi_dev_confq(info); }

    | FETC COLON DIG COLON DATQ
    { scpi_dev_fetch_digital_dataq(info); }

    | FORM format_arg
    { scpi_core_format(info, &$2); }

    | FORM COLON DAT format_arg
    { scpi_core_format(info, &$4); }

    | FORMQ
    { scpi_core_formatq(info); }

    | FORM COLON DATQ
    { scpi_core_formatq(info); }


    | INIT
    { scpi_core_initiate(info); }


    | INP COLON COUP coupling_arg
    { scpi_dev_input_coupling(info, &$4); }

    | MEAS COLON DIG COLON DATQ
    { scpi_dev_measure_digital_dataq(info); }

    | READ COLON DIG COLON DATQ
    { scpi_dev_read_digital_dataq(info); }

    | SENS COLON DATQ channel
    { scpi_dev_sense_dataq(info, &$4); }

    | SENS COLON FUNC COLON CONC boolean
    { scpi_dev_sense_function_concurrent(info, &$6); }

    | SENS COLON FUNC COLON OFF channel
    { scpi_dev_sense_function_off(info, &$6); }

    | SENS COLON FUNC COLON stateq channel
    { scpi_dev_sense_function_stateq(info, &$6); }

    | SENS COLON FUNC COLON ON channel
    { scpi_dev_sense_function_on(info, &$6); }

    | SENS COLON SWE COLON POIN numeric_value
    { scpi_dev_sense_sweep_point(info, &$6); }

    | SENS COLON SWE COLON TIME numeric_value
    { scpi_dev_sense_sweep_time(info, &$6); }

    | SENS COLON SWE COLON TINT numeric_value
    { scpi_dev_sense_sweep_time_interval(info, &$6); }

    | SENS COLON SWE COLON PRET numeric_value
    { scpi_dev_sense_sweep_pretrigger(info, &$6); }

    | SENS COLON VOLT COLON LOW numeric_value channel
    { scpi_dev_sense_voltage_low(info, &$6, &$7); }

    | SENS COLON VOLT COLON DC COLON LOW numeric_value channel
    { scpi_dev_sense_voltage_low(info, &$6, &$7); }

    | SENS COLON VOLT COLON OFFS numeric_value channel
    { scpi_dev_sense_voltage_offset(info, &$6, &$7); }

    | SENS COLON VOLT COLON DC COLON OFFS numeric_value channel
    { scpi_dev_sense_voltage_offset(info, &$6, &$7); }

    | SENS COLON VOLT COLON PTP numeric_value channel
    { scpi_dev_sense_voltage_ptp(info, &$6, &$7); }

    | SENS COLON VOLT COLON DC COLON PTP numeric_value channel
    { scpi_dev_sense_voltage_ptp(info, &$6, &$7); }

    | SENS COLON VOLT COLON RANG numeric_value channel
    { scpi_dev_sense_voltage_up(info, &$6, &$7); }

    | SENS COLON VOLT COLON DC COLON RANG numeric_value channel
    { scpi_dev_sense_voltage_up(info, &$6, &$7); }

    | SENS COLON VOLT COLON UPP numeric_value channel
    { scpi_dev_sense_voltage_up(info, &$6, &$7); }

    | SENS COLON VOLT COLON DC COLON UPP numeric_value channel
    { scpi_dev_sense_voltage_up(info, &$6, &$7); }

    | SENS COLON VOLT COLON LOWQ channel
    { scpi_dev_sense_voltage_lowq(info, &$6); }

    | SENS COLON VOLT COLON DC COLON LOWQ channel
    { scpi_dev_sense_voltage_lowq(info, &$6); }

    | SENS COLON VOLT COLON OFFSQ  channel
    { scpi_dev_sense_voltage_offsetq(info, &$6); }

    | SENS COLON VOLT COLON DC COLON OFFSQ channel
    { scpi_dev_sense_voltage_offsetq(info, &$6); }

    | SENS COLON VOLT COLON PTPQ channel
    { scpi_dev_sense_voltage_ptpq(info, &$6); }

    | SENS COLON VOLT COLON DC COLON PTPQ channel
    { scpi_dev_sense_voltage_ptpq(info, &$6); }

    | SENS COLON VOLT COLON RANGQ channel
    { scpi_dev_sense_voltage_upq(info, &$6); }

    | SENS COLON VOLT COLON DC COLON RANGQ channel
    { scpi_dev_sense_voltage_upq(info, &$6); }

    | SENS COLON VOLT COLON UPPQ channel
    { scpi_dev_sense_voltage_upq(info, &$6); }

    | SENS COLON VOLT COLON DC COLON UPPQ channel
    { scpi_dev_sense_voltage_upq(info, &$6); }

    | SOUR COLON DIG COLON DAT nr1
    { scpi_dev_source_digital_data(info, &$6); }

    | SOUR COLON DIG COLON DATQ
    { scpi_dev_source_digital_dataq(info); }

    | SOUR COLON FREQ numeric_value
    { scpi_dev_source_waveform_frequency(info, &$4); }

    | SOUR COLON FREQQ
    { scpi_dev_source_waveform_frequencyq(info); }

    | SOUR COLON FUNC source_function
    { scpi_dev_source_waveform_function(info, &$4); }

    | SOUR COLON FUNCQ
    { scpi_dev_source_waveform_functionq(info); }

    | SOUR COLON FUNC COLON USER block
    { scpi_dev_source_waveform_user(info, &$6); }

    | SOUR COLON FUNC COLON USERQ
    { scpi_dev_source_waveform_userq(info); }

    | SOUR COLON PULS COLON DCYC numeric_value
    { scpi_dev_source_pwm_duty_cycle(info, &$6); }

    | SOUR COLON PULS COLON DCYCQ
    { scpi_dev_source_pwm_duty_cycleq(info); }

    | SOUR COLON PULS COLON FREQ numeric_value
    { scpi_dev_source_pwm_frequency(info, &$6); }

    | SOUR COLON PULS COLON FREQQ
    { scpi_dev_source_pwm_frequencyq(info); }


    | SYST COLON COMM COLON TCP COLON CONTQ
    { scpi_system_communicate_tcp_controlq(info); }

    | SYST COLON internal COLON QUIT
    { scpi_system_internal_quit(info); }

    | SYST COLON internal COLON CAL
    { scpi_system_internal_calibrate(info); }

    | SYST COLON internal COLON CONF
    { scpi_system_internal_configure(info); }

    | SYST COLON internal COLON SHOWQ
    { scpi_system_internal_showq(info); }

    | SYST COLON internal COLON SETUQ
    { scpi_system_internal_setupq(info); }
    ;

%%

/*EPILOGUE*/

