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
%token ALL
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
%token EVEN
%token EVENQ
%token EXT
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
%token IMM
%token INIT
%token INP
%token INT
%token INTEGER
%token INTERNAL
%token LEV
%token LEVQ
%token LOC
%token LOCQ
%token LOW
%token LOWQ
%token MAX
%token MEAS
%token MIN
%token NEG
%token NEXTQ
%token NONE
%token OCT
%token OFF
%token OFFS
%token OFFSQ
%token ON
%token OPC
%token OPCQ
%token OPER
%token OPERQ
%token OREF
%token PACK
%token POIN
%token POINQ
%token POS
%token PRES
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
%token RES
%token RST
%token SENS
%token SETUQ
%token SHAP
%token SHOWQ
%token SIN
%token SLOP
%token SLOPQ
%token SOUR
%token SOURQ
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
%token TIMEQ
%token TINT
%token TINTQ
%token TRI
%token TRIG
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
    { scpi_core_cmd_sep(info, 1); }
    ;

cmd-path
    : cmd
    | COLON cmd
    {  scpi_core_cmd_sep(info, 0); $$=$2; }
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
    { $$ = *scpi_core_nrf_list(info, &$1, &$3); }
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
    { scpi_core_add_prefix(info, $1.token); }
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


/*
 * Manage command prefixes
 */

syst: SYST
    { scpi_core_add_prefix(info, $1.token); }
    ;

syst_err
    : syst COLON ERR
    { scpi_core_add_prefix(info, $3.token); }
    ;

syst_int
    : syst COLON internal
    { scpi_core_add_prefix(info, $3.token); }
    ;

stat_oper
    : status COLON OPER
    { scpi_core_add_prefix(info, $3.token); }
    ;

stat_ques
    : status COLON QUES
    { scpi_core_add_prefix(info, $3.token); }
    ;

conf: CONF
    { scpi_core_add_prefix(info, $1.token); }
    ;

conf_dig
    : conf COLON DIG
    { scpi_core_add_prefix(info, $3.token); }
    ;

fetc_dig
    : FETC COLON DIG
    { scpi_core_add_prefix(info, $3.token); }
    ;

form: FORM
    { scpi_core_add_prefix(info, $1.token); }
    ;

init: INIT
    { scpi_core_add_prefix(info, $1.token); }
    ;

init_imm: init COLON IMM
    { scpi_core_add_prefix(info, $3.token); }
    ;

meas: MEAS
    { scpi_core_add_prefix(info, $1.token); }
    ;

meas_dig
    : meas COLON DIG
    { scpi_core_add_prefix(info, $3.token); }
    ;

read: READ
    { scpi_core_add_prefix(info, $1.token); }
    ;

read_dig
    : read COLON DIG
    { scpi_core_add_prefix(info, $3.token); }
    ;

sens: SENS
    { scpi_core_add_prefix(info, $1.token); }
    ;

sens_func
    : sens COLON FUNC
    { scpi_core_add_prefix(info, $3.token); }
    ;

sens_swe
    : sens COLON SWE
    { scpi_core_add_prefix(info, $3.token); }
    ;

sens_swe_offs
    : sens_swe COLON OFFS
    { scpi_core_add_prefix(info, $3.token); }
    ;

sens_swe_oref
    : sens_swe COLON OREF
    { scpi_core_add_prefix(info, $3.token); }
    ;

sens_volt
    : sens COLON VOLT
    { scpi_core_add_prefix(info, $3.token); }
    ;

sens_volt_dc
    : sens_volt COLON DC
    { scpi_core_add_prefix(info, $3.token); }
    ;

sour: SOUR
    { scpi_core_add_prefix(info, $1.token); }
    ;

sour_dig
    : sour COLON DIG
    { scpi_core_add_prefix(info, $3.token); }
    ;

sour_func
    : sour COLON FUNC
    { scpi_core_add_prefix(info, $3.token); }
    ;

sour_puls
    : sour COLON PULS
    { scpi_core_add_prefix(info, $3.token); }
    ;

trig: TRIG
    { scpi_core_add_prefix(info, $1.token); }
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
    { scpi_dev_identify(info); }

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

    | syst COLON ERRQ
    { scpi_system_error_nextq(info); }

    | syst_err COLON NEXTQ
    { scpi_system_error_nextq(info); }

    | syst_err COLON COUNQ
    { scpi_system_error_countq(info); }

    | syst COLON VERSQ
    { scpi_system_versionq(info); }

    | syst COLON CAPQ
    { scpi_system_capabilityq(info); }

    /* SCPI Command Reference Chapter 20 */
    | status COLON OPERQ
    { scpi_status_operation_eventq(info); }

    | stat_oper COLON EVENQ
    { scpi_status_operation_eventq(info); }

    | stat_oper COLON CONDQ
    { scpi_status_operation_conditionq(info); }

    | stat_oper COLON ENAB nr1
    { scpi_status_operation_enable(info, &$4); }

    | stat_oper COLON ENABQ
    { scpi_status_operation_enableq(info); }

    | status COLON PRES
    { scpi_status_operation_preset(info); }

    | status COLON QUESQ
    { scpi_status_questionableq(info); }

    | stat_ques COLON ENAB nr1
    { scpi_status_questionable_enable(info, &$4); }

    | stat_ques COLON ENABQ
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
    : NONE
    | RAND
    | SIN
    | SQU
    | TRI
    { $$ = $1; }
    ;

trigger_slope
    : POS
    | NEG
    ;

trigger_source
    : IMM
    | INT
    | EXT
    ;

int_sel
    : POS
    | NEG
    ;

block
    : nrf-list
    ;

dev-cmd
    : ABOR
    { scpi_dev_abort(info); }

    | conf_dig COLON DAT
    { scpi_dev_conf_digital_data(info); }

    | conf_dig COLON EVEN int_sel NUM channel
    { scpi_dev_conf_digital_event(info, &$4, &$5, &$6); }

    | conf_dig COLON EVEN NUM channel
    { scpi_dev_conf_digital_event(info, NULL, &$4, &$5); }

    | CONFQ
    { scpi_dev_confq(info); }

    | fetc_dig COLON DATQ
    { scpi_dev_fetch_digital_dataq(info); }

    | fetc_dig COLON EVENQ
    { scpi_dev_fetch_digital_eventq(info); }

    | form format_arg
    { scpi_core_format(info, &$2); }

    | form COLON DAT format_arg
    { scpi_core_format(info, &$4); }

    | FORMQ
    { scpi_core_formatq(info); }

    | form COLON DATQ
    { scpi_core_formatq(info); }


    | init
    { scpi_dev_initiate(info); }

    | init_imm
    { scpi_dev_initiate_immediate(info); }

    | init_imm COLON ALL
    { scpi_dev_initiate_immediate(info); }


    | INP COLON COUP coupling_arg
    { scpi_dev_input_coupling(info, &$4); }

    | meas_dig COLON DATQ
    { scpi_dev_measure_digital_dataq(info); }

    | meas_dig COLON EVENQ int_sel NUM channel
    { scpi_dev_measure_digital_eventq(info, &$4, &$5, &$6); }

    | meas_dig COLON EVENQ NUM channel
    { scpi_dev_measure_digital_eventq(info, NULL, &$4, &$5); }

    | read_dig COLON DATQ
    { scpi_dev_read_digital_dataq(info); }

    | read_dig COLON EVENQ
    { scpi_dev_read_digital_eventq(info); }

    | sens COLON DATQ channel
    { scpi_dev_sense_dataq(info, &$4); }

    | sens_func COLON CONC boolean
    { scpi_dev_sense_function_concurrent(info, &$4); }

    | sens_func COLON OFF channel
    { scpi_dev_sense_function_off(info, &$4); }

    | sens_func COLON stateq channel
    { scpi_dev_sense_function_stateq(info, &$4); }

    | sens_func COLON ON channel
    { scpi_dev_sense_function_on(info, &$4); }

    | sens COLON STATQ
    { scpi_dev_sense_statq(info); }

    | sens COLON RES
    { scpi_dev_sense_reset(info); }

    | sens_swe COLON POINQ
    { scpi_dev_sense_sweep_pointq(info); }

    | sens_swe_offs COLON POIN numeric_value
    { scpi_dev_sense_sweep_offset_point(info, &$4); }

    | sens_swe_offs COLON POINQ
    { scpi_dev_sense_sweep_offset_pointq(info); }

    | sens_swe_offs COLON TIME numeric_value
    { scpi_dev_sense_sweep_offset_time(info, &$4); }

    | sens_swe_offs COLON TIMEQ
    { scpi_dev_sense_sweep_offset_timeq(info); }

    | sens_swe_oref COLON LOC numeric_value
    { scpi_dev_sense_sweep_oref_loc(info, &$4); }

    | sens_swe_oref COLON LOCQ
    { scpi_dev_sense_sweep_oref_locq(info); }

    | sens_swe_oref COLON POIN numeric_value
    { scpi_dev_sense_sweep_oref_point(info, &$4); }

    | sens_swe_oref COLON POINQ
    { scpi_dev_sense_sweep_oref_pointq(info); }

    | sens_swe COLON TIME numeric_value
    { scpi_dev_sense_sweep_time(info, &$4); }

    | sens_swe COLON TIMEQ
    { scpi_dev_sense_sweep_timeq(info); }

    | sens_swe COLON TINT numeric_value
    { scpi_dev_sense_sweep_time_interval(info, &$4); }

    | sens_swe COLON TINTQ
    { scpi_dev_sense_sweep_time_intervalq(info); }

    | sens_volt COLON LOW numeric_value channel
    { scpi_dev_sense_voltage_low(info, &$4, &$5); }

    | sens_volt_dc COLON LOW numeric_value channel
    { scpi_dev_sense_voltage_low(info, &$4, &$5); }

    | sens_volt COLON OFFS numeric_value channel
    { scpi_dev_sense_voltage_offset(info, &$4, &$5); }

    | sens_volt_dc COLON OFFS numeric_value channel
    { scpi_dev_sense_voltage_offset(info, &$4, &$5); }

    | sens_volt COLON PTP numeric_value channel
    { scpi_dev_sense_voltage_ptp(info, &$4, &$5); }

    | sens_volt_dc COLON PTP numeric_value channel
    { scpi_dev_sense_voltage_ptp(info, &$4, &$5); }

    | sens_volt COLON RANG numeric_value channel
    { scpi_dev_sense_voltage_up(info, &$4, &$5); }

    | sens_volt_dc COLON RANG numeric_value channel
    { scpi_dev_sense_voltage_up(info, &$4, &$5); }

    | sens_volt COLON UPP numeric_value channel
    { scpi_dev_sense_voltage_up(info, &$4, &$5); }

    | sens_volt_dc COLON UPP numeric_value channel
    { scpi_dev_sense_voltage_up(info, &$4, &$5); }

    | sens_volt COLON LOWQ channel
    { scpi_dev_sense_voltage_lowq(info, &$4); }

    | sens_volt_dc COLON LOWQ channel
    { scpi_dev_sense_voltage_lowq(info, &$4); }

    | sens_volt COLON OFFSQ  channel
    { scpi_dev_sense_voltage_offsetq(info, &$4); }

    | sens_volt_dc COLON OFFSQ channel
    { scpi_dev_sense_voltage_offsetq(info, &$4); }

    | sens_volt COLON PTPQ channel
    { scpi_dev_sense_voltage_ptpq(info, &$4); }

    | sens_volt_dc COLON PTPQ channel
    { scpi_dev_sense_voltage_ptpq(info, &$4); }

    | sens_volt COLON RANGQ channel
    { scpi_dev_sense_voltage_upq(info, &$4); }

    | sens_volt_dc COLON RANGQ channel
    { scpi_dev_sense_voltage_upq(info, &$4); }

    | sens_volt COLON UPPQ channel
    { scpi_dev_sense_voltage_upq(info, &$4); }

    | sens_volt_dc COLON UPPQ channel
    { scpi_dev_sense_voltage_upq(info, &$4); }

    | sour_dig COLON DAT nr1
    { scpi_dev_source_digital_data(info, &$4); }

    | sour_dig COLON DATQ
    { scpi_dev_source_digital_dataq(info); }

    | sour COLON FREQ numeric_value
    { scpi_dev_source_waveform_frequency(info, &$4); }

    | sour COLON FREQQ
    { scpi_dev_source_waveform_frequencyq(info); }

    | sour_func source_function
    { scpi_dev_source_waveform_function(info, &$2); }

    | sour COLON FUNCQ
    { scpi_dev_source_waveform_functionq(info); }

    | sour_func COLON USER block
    { scpi_dev_source_waveform_user(info, &$4); }

    | sour_func COLON USERQ
    { scpi_dev_source_waveform_userq(info); }

    | sour_func COLON LEV numeric_value
    { scpi_dev_source_waveform_level(info, &$4); }

    | sour_func COLON LEVQ
    { scpi_dev_source_waveform_levelq(info); }

    | sour_puls COLON DCYC numeric_value
    { scpi_dev_source_pwm_duty_cycle(info, &$4); }

    | sour_puls COLON DCYCQ
    { scpi_dev_source_pwm_duty_cycleq(info); }

    | sour_puls COLON FREQ numeric_value
    { scpi_dev_source_pwm_frequency(info, &$4); }

    | sour_puls COLON FREQQ
    { scpi_dev_source_pwm_frequencyq(info); }

    | trig COLON COUP coupling_arg
    { scpi_dev_trigger_coupling(info, &$4); }

    | trig COLON LEV numeric_value
    { scpi_dev_trigger_level(info, &$4); }

    | trig COLON LEVQ
    { scpi_dev_trigger_levelq(info); }

    | trig COLON SLOP trigger_slope
    { scpi_dev_trigger_slope(info, &$4); }

    | trig COLON SLOPQ
    { scpi_dev_trigger_slopeq(info); }

    | trig COLON sour trigger_source
    { scpi_dev_trigger_source(info, &$4); }

    | trig COLON SOURQ
    { scpi_dev_trigger_sourceq(info); }

    | syst COLON COMM COLON TCP COLON CONTQ
    { scpi_system_communicate_tcp_controlq(info); }

    | syst_int COLON QUIT
    { scpi_system_internal_quit(info); }

    | syst_int COLON CAL
    { scpi_system_internal_calibrate(info); }

    | syst_int COLON CONF
    { scpi_system_internal_configure(info); }

    | syst_int COLON SHOWQ
    { scpi_system_internal_showq(info); }

    | syst_int COLON SETUQ
    { scpi_system_internal_setupq(info); }
    ;

%%

/*EPILOGUE*/

