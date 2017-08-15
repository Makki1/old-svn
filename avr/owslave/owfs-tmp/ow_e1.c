/*
$Id: ow_e1.c,v 1.1 2013-12-06 14:54:34 markstaller Exp $
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: palfille@earthlink.net
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor

	Code for custom (uC based) slaves from Elaborated Networks GmbH
	(ElabNET/WireGate)

	(C) 2013 Michael Markstaller / Elaborated Networks GmbH <devel@wiregate.de>
*/

/* The slaves with family ID 1 uses 8 byte pages+CRC to transfer data,
 * somewhat similar to the DS2438.
 *
 * Content of the pages is slave-specific, see below.
 *
 */

/* Generic page-structure: (x=unused)
 * page0: TEVVUUUU Type, Errorflag, Version, Uptime
 * page1: RRA1A2FF Resets(powercycles of slave), ADC1, ADC2, Free sram
 *
 * Errorflag is bitwise: 0: general error, 1, 2, 3
 * 4 = Serial-comm, 5 = Serial-CRC, , 6=Serial-other
 *
 * Slave Quad-counter:
 * page2: CCCCcccc Counter 1,2 uint32
 * page3: CCCCcccc Counter 3,4 uint32
 *
 * Slave Smoke-detector specific:
 * page4-7
 * The smoke-detector interface is "raw", i.e. it only packs and transfers
 * the serial data:
 * Command 82/C2+C4 ->page.4, C8+C9 ->page.5, CB+CC ->page.6, CD+CE->page.7, CF->page.8
 * Credits for describing the protocol goes to many other people, see:
 * http://knx-user-forum.de/diy-do-yourself/15259-gira-dual-rauchmelder-vernetzung-einzelauswertung-fernausloesung.html
 * and https://github.com/selfbus/software-incubation/tree/master/Rauchmelder/Doc
 *
 */

#include <config.h>
#include <math.h>
#include "owfs_config.h"
#include "ow_e1.h"

/* ------- Prototypes ----------- */

/* ElabNET custom slaves */
/* WiP */
READ_FUNCTION(FS_localtype);
READ_FUNCTION(FS_r_page);
READ_FUNCTION(FS_r_int8);
READ_FUNCTION(FS_r_int16);
READ_FUNCTION(FS_r_int32);
READ_FUNCTION(FS_r_int32_d4);
READ_FUNCTION(FS_r_rm_volt);
READ_FUNCTION(FS_r_rm_temp);
READ_FUNCTION(FS_r_version);
WRITE_FUNCTION(FS_w_func_u32);

static enum e_visibility VISIBLE_E1_COUNTER( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_E1_ADC( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_E1_SMOKE( const struct parsedname * pn ) ;
static enum e_visibility VISIBLE_E1_PRES( const struct parsedname * pn ) ;
static enum e_E1_type VISIBLE_E1( const struct parsedname * pn ) ;

enum e_E1_type {
    e1t_NONE = 0,
    e1t_COUNTER = 1,
	e1t_SMOKE = 2,
	e1t_PWM4 = 3,
	e1t_PWM6 = 4,
	e1t_SRF = 5,
	e1t_PRES = 6,
} ;

/* ------- Structures ----------- */
static struct aggregate APAGE = { 11, ag_numbers, ag_separate, };
static struct aggregate ACNT = { 4, ag_letters, ag_separate, };

// struct bitfield { "alias_link", number_of_bits, shift_left, } - zero-based
static struct bitfield e1_rm_error = { "smoke/byte2_1", 1, 1, } ; //0x02
static struct bitfield e1_rm_button = { "smoke/byte2_1", 1, 3, } ;
static struct bitfield e1_rm_alarm1 = { "smoke/byte2_1", 1, 4, } ;
static struct bitfield e1_rm_onbatt = { "smoke/byte2_1", 1, 5, } ;
static struct bitfield e1_rm_battlow = { "smoke/byte2_2", 1, 0, } ; //0x01
static struct bitfield e1_rm_smokealarm = { "smoke/byte2_2", 1, 2, } ; //0x04
static struct bitfield e1_rm_wirealarm = { "smoke/byte2_2", 1, 3, } ; //0x08
static struct bitfield e1_rm_rfalarm = { "smoke/byte2_2", 1, 4, } ; //0x10
static struct bitfield e1_rm_localtestalarm = { "smoke/byte2_2", 1, 5, } ; //0x20
static struct bitfield e1_rm_wiretestalarm = { "smoke/byte2_2", 1, 6, } ; //0x40
static struct bitfield e1_rm_rftestalarm = { "smoke/byte2_2", 1, 7, } ; //0x80

static struct bitfield e1_error_generic = { "errors/byte", 1, 0, } ;
static struct bitfield e1_error_onewire = { "errors/byte", 1, 1, } ;
static struct bitfield e1_error_power = { "errors/byte", 1, 2, } ;
static struct bitfield e1_error_serial = { "errors/byte", 1, 4, } ; //0x10
static struct bitfield e1_error_serialcrc = { "errors/byte", 1, 5, } ; //0x20

static struct bitfield e1_inB_PB0 = { "input_byteB", 1, 0, } ;
static struct bitfield e1_inB_PB1 = { "input_byteB", 1, 1, } ;

static struct bitfield e1_inA_PA3 = { "input_byteA", 1, 3, } ;
static struct bitfield e1_inA_PA4 = { "input_byteA", 1, 4, } ;
static struct bitfield e1_inA_PA5 = { "input_byteA", 1, 5, } ;

static struct bitfield e1_outA_PA0 = { "output_byteA", 1, 0, } ;
static struct bitfield e1_outA_PA1 = { "output_byteA", 1, 1, } ;

static struct filetype ElabNET_E1[] = {
    F_STANDARD_NO_TYPE,
    {"pages", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
    {"pages/page", 8, &APAGE, ft_binary, fc_volatile, FS_r_page, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },

    {"typeid", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_stable, FS_r_int8, NO_WRITE_FUNCTION, VISIBLE, {i:0}, },
    {"type", PROPERTY_LENGTH_TYPE, NON_AGGREGATE, ft_ascii, fc_link, FS_localtype, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
    {"errors", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
    {"errors/byte", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, VISIBLE, {i:1}, },
    {"errors/onewire", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_error_onewire, }, },
    {"errors/power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_error_power, }, },
    {"errors/serial", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_error_serial, }, },
    {"errors/serial_crc", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_error_serialcrc, }, },

    {"version", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_ascii, fc_link, FS_r_version, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
    {"versionid", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_stable, FS_r_int16, NO_WRITE_FUNCTION, INVISIBLE, {i:2}, },
    {"uptime", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_int32, NO_WRITE_FUNCTION, VISIBLE, {i:4}, },
    {"resets", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_stable, FS_r_int16, NO_WRITE_FUNCTION, VISIBLE, {i:8}, },
    {"adc1", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int16, NO_WRITE_FUNCTION, VISIBLE_E1_ADC, {i:10}, },
    {"adc2", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int16, NO_WRITE_FUNCTION, VISIBLE_E1_ADC, {i:12}, },
    {"sram", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_stable, FS_r_int16, NO_WRITE_FUNCTION, INVISIBLE, {i:14}, },

    {"input_byteA", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i: 32}, },
    {"input_byteB", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i: 33}, },
    //FIXME: complement of inputs?
    {"inputA", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_E1_COUNTER, {v: &e1_inB_PB1}, },
    {"inputB", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_E1_COUNTER, {v: &e1_inA_PA3}, },
    {"inputC", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_E1_COUNTER, {v: &e1_inA_PA4}, },
    {"inputD", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE_E1_COUNTER, {v: &e1_inA_PA5}, },
    {"in_power", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, INVISIBLE, {v: &e1_inB_PB0}, },

    {"internal_pullup", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, FS_r_int8, FS_w_func_u32, VISIBLE_E1_COUNTER, {i:34}, },

    /* FIXME: better aggregated? */
    {"output_byteA", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, FS_w_func_u32, INVISIBLE, {i:35}, },
    {"outputA", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, FS_w_bitfield, VISIBLE_E1_COUNTER, {v: &e1_outA_PA0}, },
    {"outputB", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, FS_w_bitfield, VISIBLE_E1_COUNTER, {v: &e1_outA_PA1}, },

    {"counter", PROPERTY_LENGTH_UNSIGNED, &ACNT, ft_unsigned, fc_volatile, FS_r_int32, FS_w_func_u32, VISIBLE_E1_COUNTER, {i:16}, },
/*    {"counter", PROPERTY_LENGTH_UNSIGNED, &ACNT, ft_unsigned, fc_volatile, FS_r_int32, NO_WRITE_FUNCTION, VISIBLE_E1_COUNTER, {i:16}, },
    {"set", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_E1_COUNTER, NO_FILETYPE_DATA, },
    {"set/counterA", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, NO_READ_FUNCTION, FS_w_func_u32, VISIBLE_E1_COUNTER, {i:16}, },
    {"set/counterB", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, NO_READ_FUNCTION, FS_w_func_u32, VISIBLE_E1_COUNTER, {i:20}, },
    {"set/counterC", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, NO_READ_FUNCTION, FS_w_func_u32, VISIBLE_E1_COUNTER, {i:24}, },
    {"set/counterD", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, NO_READ_FUNCTION, FS_w_func_u32, VISIBLE_E1_COUNTER, {i:28}, },
*/
    {"resetALL", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_w_func_u32, VISIBLE_E1_COUNTER, {i:1}, },
/* this is unfinished but leads to user-settable (eeprom) values 4*8=32 bytes starting at page.9
    {"user1", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_int32, FS_w_func_u32, VISIBLE, {i:72}, },
    {"user2", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_int32, FS_w_func_u32, VISIBLE, {i:76}, },
    {"user3", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_int32, FS_w_func_u32, VISIBLE, {i:80}, },
    {"user4", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_int32, FS_w_func_u32, VISIBLE, {i:84}, },
*/
    {"user", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_E1_COUNTER, NO_FILETYPE_DATA, },
    {"user/eeprom", PROPERTY_LENGTH_UNSIGNED, &ACNT, ft_unsigned, fc_volatile, FS_r_int32, FS_w_func_u32, VISIBLE_E1_COUNTER, {i:72}, },

    {"pressure", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_int32, NO_WRITE_FUNCTION, VISIBLE_E1_PRES, {i:16}, },
    {"temperature", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_int32, NO_WRITE_FUNCTION, VISIBLE_E1_PRES, {i:20}, },
/* FIXME: read out calibration-data from BMP05/BMP180 */
/* FIXME: user-eeprom for E1_PRES */

    {"smoke", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE_E1_SMOKE, NO_FILETYPE_DATA, },
    {"smoke/ALARM", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_stable, NO_READ_FUNCTION, FS_w_func_u32, VISIBLE, {i:10}, },
    {"smoke/byte2_1", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i:32, }, },
    {"smoke/byte2_2", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i:33, }, },
    {"smoke/byte2_3", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i:34, }, },
    {"smoke/byte2_4", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i:35, }, },
    {"smoke/serial", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_int32, NO_WRITE_FUNCTION, VISIBLE, {i:36, }, },
    {"smoke/byte8_1", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i:40, }, },
    {"smoke/byte8_2", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i:41, }, },
    {"smoke/byte8_3", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i:42, }, },
    {"smoke/byte8_4", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i:43, }, },
    {"smoke/runtime", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_int32_d4, NO_WRITE_FUNCTION, VISIBLE, {i:44, }, },
    {"smoke/smokeval", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_volatile, FS_r_int16, NO_WRITE_FUNCTION, VISIBLE, {i:48, }, },
    {"smoke/counter", PROPERTY_LENGTH_SUBDIR, NON_AGGREGATE, ft_subdir, fc_subdir, NO_READ_FUNCTION, NO_WRITE_FUNCTION, VISIBLE, NO_FILETYPE_DATA, },
    {"smoke/counter/localsmokealarms", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_int8, NO_WRITE_FUNCTION, VISIBLE, {i:50, }, },
    {"smoke/smokedirt", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_int8, NO_WRITE_FUNCTION, VISIBLE, {i:51, }, },
    {"smoke/battvolt", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_rm_volt, NO_WRITE_FUNCTION, VISIBLE, {i:52, }, },
    {"smoke/temp1", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_rm_temp, NO_WRITE_FUNCTION, VISIBLE, {i:54, }, },
    {"smoke/temp2", PROPERTY_LENGTH_FLOAT, NON_AGGREGATE, ft_float, fc_volatile, FS_r_rm_temp, NO_WRITE_FUNCTION, VISIBLE, {i:55, }, },
    {"smoke/counter/localtempalarms", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_int8, NO_WRITE_FUNCTION, VISIBLE, {i:56, }, },
    {"smoke/counter/localtestalarms", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_int8, NO_WRITE_FUNCTION, VISIBLE, {i:57, }, },
    {"smoke/counter/rfalarms", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_int8, NO_WRITE_FUNCTION, VISIBLE, {i:58, }, },
    {"smoke/counter/wirealarms", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_int8, NO_WRITE_FUNCTION, VISIBLE, {i:59, }, },
    {"smoke/counter/wiretestalarms", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_int8, NO_WRITE_FUNCTION, VISIBLE, {i:60, }, },
    {"smoke/counter/rftestalarms", PROPERTY_LENGTH_UNSIGNED, NON_AGGREGATE, ft_unsigned, fc_stable, FS_r_int8, NO_WRITE_FUNCTION, VISIBLE, {i:61, }, },
    {"smoke/byteF_1", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i:64, }, },
    {"smoke/byteF_2", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i:65, }, },
    {"smoke/byteF_3", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i:66, }, },
    {"smoke/byteF_4", PROPERTY_LENGTH_INTEGER, NON_AGGREGATE, ft_integer, fc_volatile, FS_r_int8, NO_WRITE_FUNCTION, INVISIBLE, {i:67, }, },
    {"smoke/error", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_rm_error, }, },
    {"smoke/button", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_rm_button, }, },
    {"smoke/alarm", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_rm_alarm1, }, },
    {"smoke/onbatt", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_rm_onbatt, }, },
    {"smoke/battlow", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_rm_battlow, }, },
    {"smoke/smokealarm", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_rm_smokealarm, }, },
    {"smoke/wirealarm", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_rm_wirealarm, }, },
    {"smoke/rfalarm", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_rm_rfalarm, }, },
    {"smoke/localtestalarm", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_rm_localtestalarm, }, },
    {"smoke/wiretestalarm", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_rm_wiretestalarm, }, },
    {"smoke/rftestalarm", PROPERTY_LENGTH_YESNO, NON_AGGREGATE, ft_yesno, fc_volatile, FS_r_bitfield, NO_WRITE_FUNCTION, VISIBLE, {v: &e1_rm_rftestalarm, }, },
};

DeviceEntry(E1, ElabNET_E1, NO_GENERIC_READ, NO_GENERIC_WRITE);

#define _1W_WRITE_SCRATCHPAD 0x4E
#define _E1_WRITE_FUNC 0x4F
#define _1W_READ_SCRATCHPAD 0xBE

/* ------- Functions ------------ */

/* prototypes */
static GOOD_OR_BAD OW_r_page(BYTE * p, const int page, const struct parsedname *pn);
static GOOD_OR_BAD OW_w_page(const BYTE * p, const struct parsedname *pn);

/* 8 Byte pages */
#define E1_ADDRESS_TO_PAGE(a)   ((a)>>3)
#define E1_ADDRESS_TO_OFFSET(a) ((a)&0x07)

static enum e_E1_type VISIBLE_E1( const struct parsedname * pn )
{
    int e1t = -1 ;
    LEVEL_DEBUG("Checking visibility of %s",SAFESTRING(pn->path)) ;
    if ( BAD( GetVisibilityCache( &e1t, pn ) ) ) {
        struct one_wire_query * owq = OWQ_create_from_path(pn->path) ; // for read
        if ( owq != NULL) {
            UINT U_e1t ;
            if ( FS_r_sibling_U( &U_e1t, "typeid", owq ) == 0 ) {
                e1t = U_e1t ;
                SetVisibilityCache( e1t, pn ) ;
            }
            OWQ_destroy(owq) ;
        }
    }
    return (enum e_E1_type) e1t ;
}
static enum e_visibility VISIBLE_E1_COUNTER( const struct parsedname * pn )
{
    switch ( VISIBLE_E1(pn) ) {
        case e1t_COUNTER:
            return visible_now ;
        default:
            return visible_not_now ;
    }
}
static enum e_visibility VISIBLE_E1_ADC( const struct parsedname * pn )
{
    switch ( VISIBLE_E1(pn) ) { //FIXME: depends on MCU/version
        case e1t_NONE:
            return visible_now ;
        default:
            return visible_not_now ;
    }
}
static enum e_visibility VISIBLE_E1_SMOKE( const struct parsedname * pn )
{
    switch ( VISIBLE_E1(pn) ) {
        case e1t_SMOKE:
            return visible_now ;
        default:
            return visible_not_now ;
    }
}
static enum e_visibility VISIBLE_E1_PRES( const struct parsedname * pn )
{
    switch ( VISIBLE_E1(pn) ) {
        case e1t_PRES:
            return visible_now ;
        default:
            return visible_not_now ;
    }
}



static ZERO_OR_ERROR FS_r_page(struct one_wire_query *owq)
{
    struct parsedname *pn = PN(owq);
    BYTE data[8];
    RETURN_ERROR_IF_BAD( OW_r_page(data, pn->extension, pn) ) ;
    memcpy((BYTE *) OWQ_buffer(owq), &data[OWQ_offset(owq)], OWQ_size(owq));
    return 0;
}

static ZERO_OR_ERROR FS_r_int8(struct one_wire_query *owq)
{
    struct parsedname *pn = PN(owq);
    int page = E1_ADDRESS_TO_PAGE(pn->selected_filetype->data.s);
    int offset = E1_ADDRESS_TO_OFFSET(pn->selected_filetype->data.s);
    BYTE data[8];

    RETURN_ERROR_IF_BAD(OW_r_page(data, page, pn)) ;
    OWQ_U(owq) = UT_int8(&data[offset]);
    return 0;
}
static ZERO_OR_ERROR FS_r_int16(struct one_wire_query *owq)
{
    struct parsedname *pn = PN(owq);
    int page = E1_ADDRESS_TO_PAGE(pn->selected_filetype->data.s);
    int offset = E1_ADDRESS_TO_OFFSET(pn->selected_filetype->data.s);
    BYTE data[8];

    RETURN_ERROR_IF_BAD(OW_r_page(data, page, pn)) ;
    OWQ_U(owq) = UT_int16(&data[offset]);
    return 0;
}

static ZERO_OR_ERROR FS_r_rm_volt(struct one_wire_query *owq)
{
    struct parsedname *pn = PN(owq);
    int page = E1_ADDRESS_TO_PAGE(pn->selected_filetype->data.s);
    int offset = E1_ADDRESS_TO_OFFSET(pn->selected_filetype->data.s);
    BYTE data[8];

    RETURN_ERROR_IF_BAD(OW_r_page(data, page, pn)) ;
    OWQ_F(owq) = (UT_int16(&data[offset])) * 0.018369; //* 9184 / 5000
    return 0;
}
static ZERO_OR_ERROR FS_r_rm_temp(struct one_wire_query *owq)
{
    struct parsedname *pn = PN(owq);
    int page = E1_ADDRESS_TO_PAGE(pn->selected_filetype->data.s);
    int offset = E1_ADDRESS_TO_OFFSET(pn->selected_filetype->data.s);
    BYTE data[8];

    RETURN_ERROR_IF_BAD(OW_r_page(data, page, pn)) ;
    OWQ_F(owq) = (UT_int8(&data[offset]))/2-20;
    return 0;
}

static ZERO_OR_ERROR FS_r_int32(struct one_wire_query *owq)
{
    struct parsedname *pn = PN(owq);
    int page = E1_ADDRESS_TO_PAGE(pn->selected_filetype->data.s + (OWQ_pn(owq).extension*4));
    int offset = E1_ADDRESS_TO_OFFSET(pn->selected_filetype->data.s + (OWQ_pn(owq).extension*4));
    BYTE data[8];
    //LEVEL_DEBUG("######################################################## %s Extension %d, page %d, offset %d, data %d)",PN(owq)->path,OWQ_pn(owq).extension, page, offset, pn->selected_filetype->data.s );
    RETURN_ERROR_IF_BAD(OW_r_page(data, page, pn)) ;
    OWQ_U(owq) = UT_int32(&data[offset]);
    return 0;
}
static ZERO_OR_ERROR FS_r_int32_d4(struct one_wire_query *owq)
{
    struct parsedname *pn = PN(owq);
    int page = E1_ADDRESS_TO_PAGE(pn->selected_filetype->data.s + (OWQ_pn(owq).extension*4));
    int offset = E1_ADDRESS_TO_OFFSET(pn->selected_filetype->data.s + (OWQ_pn(owq).extension*4));
    BYTE data[8];
    RETURN_ERROR_IF_BAD(OW_r_page(data, page, pn)) ;
    OWQ_U(owq) = UT_int32(&data[offset])/4;
    return 0;
}

static ZERO_OR_ERROR FS_localtype(struct one_wire_query *owq)
{
    UINT typeid;
    if ( FS_r_sibling_U( &typeid, "typeid", owq ) != 0 ) {
        return -EINVAL ;
    }

    switch ((enum e_E1_type) typeid) {
        case e1t_COUNTER:
            return OWQ_format_output_offset_and_size_z("ElabNET Quadcounter", owq) ;
        case e1t_SMOKE:
            return OWQ_format_output_offset_and_size_z("RM-interface", owq) ;
        case e1t_PWM4:
            return OWQ_format_output_offset_and_size_z("ElabNET Quad PWM", owq) ;
        case e1t_PRES:
            return OWQ_format_output_offset_and_size_z("ElabNET Pressure sensor", owq) ;
        default:
            return FS_type(owq);
    }
}

/* this function is messed up, it just takes some bytes and calculates CRC8..
 * page-setup is 1 byte function + 8byte data + 1byte CRC
 */
static ZERO_OR_ERROR FS_w_func_u32(struct one_wire_query *owq)
{
    struct parsedname *pn = PN(owq);
    //int page = E1_ADDRESS_TO_PAGE(pn->selected_filetype->data.s + (OWQ_pn(owq).extension*4));
    //int offset = E1_ADDRESS_TO_OFFSET(pn->selected_filetype->data.s + (OWQ_pn(owq).extension*4));
    //BYTE func = pn->selected_filetype->data.i;
    BYTE data[10];
    memset(data, 0, 9);
    //data[0] = pn->selected_filetype->data.s + (page*8) + offset;
    data[0] = pn->selected_filetype->data.i + (OWQ_pn(owq).extension*4);
    UT_uint32_to_bytes( OWQ_U(owq), &data[1] );

    data[9] = CRC8(data,9);

    return GB_to_Z_OR_E( OW_w_page(data, pn) ) ;
}

static ZERO_OR_ERROR FS_r_version(struct one_wire_query *owq)
{
    char v[6];
    UINT version_number;

    if ( FS_r_sibling_U( &version_number, "versionid", owq ) != 0 ) {
        return -EINVAL ;
    }

    UCLIBCLOCK;
    snprintf(v,6,"%X.%.2X",version_number>>8,version_number&0xFF);
    UCLIBCUNLOCK;

    return OWQ_format_output_offset_and_size(v, 5, owq);
}

/*
static ZERO_OR_ERROR FS_w_set_alarm(struct one_wire_query *owq)
{
    BYTE c;
    switch (OWQ_U(owq)) {
    case 0:
        c = 0 << 3;
        break;
    case 1:
        c = 1 << 3;
        break;
    case 10:
        c = 2 << 3;
        break;
    case 11:
        c = 3 << 3;
        break;
    case 100:
        c = 4 << 3;
        break;
    case 101:
        c = 5 << 3;
        break;
    case 110:
        c = 6 << 3;
        break;
    case 111:
        c = 7 << 3;
        break;
    default:
        return -ERANGE;
    }
    if (OW_w_mem(&c, 1, 0x200, PN(owq))) {
        return -EINVAL;
    }
    return 0;
}
*/


/* OW physical functions */
/* read page (1byte page + 8 bytes data + 1 CRC);
 * in slave values are atomic, i.e. cannot change within a started TRXN */
static GOOD_OR_BAD OW_r_page(BYTE * p, const int page, const struct parsedname *pn)
{
    BYTE data[10];
    BYTE r[] = { _1W_READ_SCRATCHPAD, page, };
    struct transaction_log t[] = {
        TRXN_START,
        TRXN_WRITE2(r),
        TRXN_READ(data, 10),
        TRXN_CRC8(data, 10),
        TRXN_END,
    };

    // read to scratch, then in
    RETURN_BAD_IF_BAD(BUS_transaction(t, pn)) ;
    if (data[0] != page) { //something went wrong, we got another page back than requested
      LEVEL_DEBUG("ERROR, requested page %d but got back %d",page,data[0]);
      return gbBAD;
    }

    // copy to buffer (without page&crc)
    memcpy(p, data+1, 8);
    return gbGOOD;
}

/* write page - 8 bytes; basically a fake, not the "page" but a specific
 * value/function defined by byte/page is written/executed
 */
static GOOD_OR_BAD OW_w_page(const BYTE * p, const struct parsedname *pn)
{
    BYTE w[] = { _E1_WRITE_FUNC, };
    struct transaction_log t[] = {
        TRXN_START,             // 0
        TRXN_WRITE1(w),
        TRXN_WRITE(p, 10),
        //TRXN_DELAY(34), // 8*3.4 msec
        TRXN_END,
    };

    return BUS_transaction(t, pn) ;
}
