/* common defines
 *
 */

#ifndef _common_h_
#define _common_h_

const uint16_t EE_MAGIC_NUMBER = 0xE1E2;
const uint8_t EE_DEFTYPE = 1;

const uint8_t EE_MAGIC_OFFSET = 0; //2byte
const uint8_t EE_OWID_OFFSET = 2; //8byte
const uint8_t EE_RCNT_OFFSET = 10; //2byte
const uint8_t EE_COUNTER_OFFSET = 12; //keep 8x4=32bytes free!
const uint8_t EE_TYPE_OFFSET = 44; //1byte
const uint8_t EE_VERSION_OFFSET = 45; //2byte

const uint8_t EE_LABEL_OFFSET = 50; //32byte
const uint8_t EE_LABEL_MAXLEN = 31;
/*
 * OW-Timing adjustable in EEPROM ??!
 * const uint8_t EE_USER1_OFFSET = 82; //Userdata?
 * counter-config like debounce-time, ticks/impulse etc.?
 */

/* some nice macros */
#define BIT(x) (1 << (x))
#define SETBITS(x,y) ((x) |= (y))
#define CLEARBITS(x,y) ((x) &= (~(y)))
#define SETBIT(x,y) SETBITS((x), (BIT((y))))
#define CLEARBIT(x,y) CLEARBITS((x), (BIT((y))))
#define BITSET(x,y) ((x) & (BIT(y)))
#define BITCLEAR(x,y) !BITSET((x), (y))
#define BITSSET(x,y) (((x) & (y)) == (y))
#define BITSCLEAR(x,y) (((x) & (y)) == 0)
#define BITVAL(x,y) (((x)>>(y)) & 1)

#define CONCAT(a,b) a##b
#ifdef USE_PINx_TOGGLE
#  define TOGGLE(a,b) CONCAT(PIN,a) = (1 << (b))
#else
#  define TOGGLE(a,b) CONCAT(PORT,a) ^= (1 << (b))
#endif
/* use:
TOGGLE(D, 4);
TOGGLE(A, 0);
*/

/* function prototypes */

#endif // _common_h_