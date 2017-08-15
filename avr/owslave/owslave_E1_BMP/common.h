/* common defines
 *
 */

const uint16_t EE_MAGIC_NUMBER = 0xE1E2;
const uint8_t EE_DEFTYPE = 1;

const uint8_t EE_MAGIC_OFFSET = 0; //2byte
const uint8_t EE_OWID_OFFSET = 2; //8byte
const uint8_t EE_RCNT_OFFSET = 10; //2byte
const uint8_t EE_TYPE_OFFSET = 44; //1byte
const uint8_t EE_VERSION_OFFSET = 45; //2byte

const uint8_t EE_LABEL_OFFSET = 50; //32byte
const uint8_t EE_LABEL_MAXLEN = 32;
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

/*
Will man direkt auf bestimmte EEPROM Adressen zugreifen, dann sind folgende Funktionen hilfreich, um sich die Typecasts zu ersparen:

#include <avr/eeprom.h>

// Byte aus dem EEPROM lesen
uint8_t EEPReadByte(uint16_t addr)
{
  return eeprom_read_byte((uint8_t *)addr);
}

// Byte in das EEPROM schreiben
void EEPWriteByte(uint16_t addr, uint8_t val)
{
  eeprom_write_byte((uint8_t *)addr, val);
}

oder als Makro:

#define   EEPReadByte(addr)         eeprom_read_byte((uint8_t *)addr)
#define   EEPWriteByte(addr, val)   eeprom_write_byte((uint8_t *)addr, val)

Verwendung:

EEPWriteByte(0x20, 128);   // Byte an die Adresse 0x20 schreiben
Val=EEPReadByte(0x20);     // EEPROM-Wert von Adresse 0x20 lesen
*/

/* function prototypes */
