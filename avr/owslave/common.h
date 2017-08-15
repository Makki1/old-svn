/* common defines
 *
 */

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
