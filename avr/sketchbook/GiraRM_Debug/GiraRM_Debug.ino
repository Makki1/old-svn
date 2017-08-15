/* Gira RM Dual/VdS 2330 Testcode 2013-05-13
  Just for testing!
  Using a Arduino Mega2560, does nothing useful on others:
  Serial(0): unused
  Serial 1 RX -> in from Pin5/6
  Serial 2 TX -> out to Pin7/8 (use a level-shifter for 3V3!! or 2x 1k voltage-divider)
  Serial 3 Debug-output only, - again, use level-shifter to PC

   (C) 2013 Michael Markstaller / Elaborated Networks GmbH
*/

/* Zusammenfassung Debug mit Atmega/Arduino siehe README:
Pins (wie bereits angegeben)
"unten links" bei Sicht auf den RM mit lesbarem Etikett innen = 1!
1-2: Vcc 9V+
3-4: GND
5-6: TX vom RM (P3.4 TXD)
7-8: RX des RM (P1.6)
9-10: ? (P1.5)
11-12: 3V3+
13/14: RESET/TEST?

- 9600 8N1
- Senden (unmotiviert) auf RX wird mit 0x15 (NACK) beantwortet.


connections Mega2560 @ 5V:
(not implemented here, just tested!):
Aref: INTERNAL1V1 (as 5VDC could be less than 5.0!)
- we surely don't want to measure/burn some mA using Vdif if running on batts!
- A2 5VDC Voltage-divider 10k/2k2 Ohm: 5V->10k->A2->2k2->GND
  Vcc = 5,9737*analogRead/1000 at 10k/2k2
- A1 12Vext: Vdif 10k/1k
  Vext = 11,8389*analogRead/1000 at 10k/1k
-> Diode between 230/12V input-pin on green connector, on Pins 1-2 we have it AFTER/battery-voltage!

*/

#define DEBUG 1
#include "debug.h"

/* Temp vars / debug */
unsigned long lastcheck = millis(); //only for debug
unsigned long data_last = millis(); //data-watchdog
int democounter = 0;
int msgfails = 0;
byte led_state = 1;
const byte ID_PIN = 8;

/* recv-states */
enum {
      S_NULL = 0,
      S_STX = 2,
      S_ETX = 3,
      S_ACK = 6,
      S_NACK = 15,
      S_DATA = 20,
};

// currently consumes 26bytes, maybe save a little on attiny!
// RFM12B payload is 64byte, safe.. push this out via RF, RS485 or 1Wire as whole struct
typedef struct rmstate_s {
  uint32_t serial; //save 2, cutoff or CRC?
  uint16_t uptime; //uint16 for hours lasting 7yrs
  uint16_t smokeval;
  uint8_t smokealarms;
  uint8_t smokedirt;
  float battvolt; //save 3 by (val-4)*30
  uint8_t temp1; //save 1 using middle?
  uint8_t temp2;
  uint8_t localtempalarms;
  uint8_t localtestalarms;
  uint8_t wirealarms;
  uint8_t rfalarms;
  uint8_t wiretestalarms;
  uint8_t rftestalarms;
  union {
    uint16_t states;
    bool button:1;
    bool onbatt:1;
    bool alarm1:1;
    bool error:1;
    bool battlow:1;
    bool smokealarm:1;
    bool wirealarm:1;
    bool rfalarm:1;
    bool localtestalarm:1;
    bool wiretestalarm:1;
    bool rftestalarm:1;
    bool temp1err:1;
    bool temp2err:1;
    // 3 bits remain..
  };
  uint8_t sbyte3; //save 1, unknwon
  uint8_t sbyte4; //save 1, unknwon only, only Bit2/4 = tempsensor
};

rmstate_s rmstate;

uint8_t recv_state = S_NULL;
uint8_t nackmsg=0;

void setup()
{
  DEBUG_INIT LL LL
  DEBUG_PRINT("Debug-Port enabled, setup")
  LS("Code: ") LS(__DATE__ " " __TIME__) LS(" Free: ") LV(freeRam()) LL
  LSL("Sizeof struct:") LVD(sizeof(rmstate))

  // initialize the serial communication:
  Serial1.begin(9600);
  Serial2.begin(9600);
  pinMode(ID_PIN,OUTPUT);
  digitalWrite(ID_PIN,led_state);
} //setup


void loop() {

byte manual_mode = 1;
#ifdef DEBUG
if (millis() - lastcheck > 10000) {
  //LS("L:") LV(millis()) LS(":State:") LL
  //LSL("loop")
  if (led_state)
    led_state = 0;
  else
    led_state = 1;
  digitalWrite(ID_PIN, led_state);

  lastcheck = millis();
}
#endif //debug

#if defined(__AVR_ATmega2560__)
//accept some debug-commands on Serial
if (Serial.available()) {
  byte tmp = Serial.read();
  uint8_t req[] = {0x02, 0x30, 0x00, 0x00, 0x00, 0x03 };
  switch (tmp) {
    case 'V': //print Debug, dump values etc..
      LS("Code: ") LS(__DATE__ " " __TIME__) LS(" Free: ") LV(freeRam()) LL
      break;
/*
    case 'S': //silence alarm 07 00 08 2F? useless!
      {
        uint8_t req2[] = {0x02, 0x30, 0x37, 0x30, 0x30, 0x30, 0x38, 0x32, 0x46, 0x03 };
        Serial2.write(req2,sizeof(req2));
      }
      break;
*/
    case 'L': //07 00 20 29 - locate RM 2Hz beep, only with button off!
      {
        uint8_t req2[] = {0x02, 0x30, 0x37, 0x30, 0x30, 0x32, 0x30, 0x32, 0x39, 0x03 };
        Serial2.write(req2,sizeof(req2));
      }
      break;
/*
    03 02 10 26 - set ALARM active (via rF) -> A
    03 02 80 2D - set Testalarm (via rF) -> T
    03 02 00 25 - end (Test)-Alarm -> S(ilence)
*/
    case 'A':
      {
        uint8_t req2[] = {0x02, 0x30, 0x33, 0x30, 0x32, 0x31, 0x30, 0x32, 0x36, 0x03 };
        Serial2.write(req2,sizeof(req2));
      }
      break;
    case 'T':
      {
        uint8_t req2[] = {0x02, 0x30, 0x33, 0x30, 0x32, 0x38, 0x30, 0x32, 0x44, 0x03 };
        Serial2.write(req2,sizeof(req2));
      }
      break;
    case 'S':
      {
        uint8_t req2[] = {0x02, 0x30, 0x33, 0x30, 0x32, 0x30, 0x30, 0x32, 0x35, 0x03 };
        Serial2.write(req2,sizeof(req2));
      }
      break;
    case '1': 
      {
    //03 02 08 2D state?
        uint8_t req2[] = {0x02, 0x30, 0x33, 0x30, 0x32, 0x30, 0x38, 0x32, 0x44, 0x03 };
        Serial2.write(req2,sizeof(req2));
      }
      break;

    // sends querys
    case '2':
      req[2] = 0x32; req[3] = 0x36; req[4] = 0x32;
      break;
    case '4':
      req[2] = 0x34; req[3] = 0x36; req[4] = 0x34;
      break;
    case '8':
      req[2] = 0x38; req[3] = 0x36; req[4] = 0x38;
      break;
    case '9':
      req[2] = 0x39; req[3] = 0x36; req[4] = 0x39;
      break;
    case 'B':
      req[2] = 0x42; req[3] = 0x37; req[4] = 0x32;
      break;
    case 'C':
      req[2] = 0x43; req[3] = 0x37; req[4] = 0x33;
      break;
    case 'D':
      req[2] = 0x44; req[3] = 0x37; req[4] = 0x34;
      break;
    case 'E':
      req[2] = 0x45; req[3] = 0x37; req[4] = 0x35;
      break;
    case 'P':
      //pinAnalyzer();
      break;
    default:
      LSL("Dunno?") LVH(tmp) LL;
  }
  if (req[2]) {
    LS("- written ") LVH(req[2]) LL
    Serial2.write(req,sizeof(req));
  }
}
#endif //mega2560

if (Serial1.available() && manual_mode) {
  uint8_t bpos = 0;
  uint16_t chksum = 0;
  char recv_buf[16];

  LSL("S1-in:")
  while (Serial1.available()) {
    char tmp = Serial1.read();
    //LVH(tmp)
    switch (tmp) {
      case 0x00:
        LS("(NUL)");
        recv_state = S_NULL;
        bpos=0; chksum=0;
        break;
      case 0x02:
        LS("(STX)");
        recv_state = S_STX;
        bpos=0; chksum=0;
        break;
      case 0x03:
        LS("(ETX)");
        recv_state = S_ETX;
        //FIXME: sendack only if chksum is ok!
        Serial2.write(0x06); // send ACK
        break;
      case 0x06:
        //FIXME: only collectd data on first packet with ACK? all repeated 3x without ACK
        LS("(ACK)");
        recv_state = S_ACK;
        bpos=0; chksum=0;
        break;
      case 0x15:
        LS("(NACK)");
        recv_state = S_NULL;
        nackmsg++;
        bpos=0; chksum=0;
        break;
      default: //ASCII
        LV(tmp);
        if (recv_state == S_STX) {
          recv_state = S_DATA;
          bpos=0; chksum=0;
        }
        break;
    }
    switch (recv_state) {
      case S_DATA:
        recv_buf[bpos] = tmp;
        chksum += tmp;
        if (bpos < sizeof (recv_buf))
          bpos++;
        break;
    }
    delay(1); //wait a little in debug, so we get nice one-line-answers
  }
  LL
  LS("Data")
  for (uint8_t i = 0; i < bpos; i++) {
    LVH(recv_buf[i])
  }
  LL
  //quirk, substract last two bytes
  chksum -= recv_buf[bpos-2] + recv_buf[bpos-1];
  chksum &= 0xFF;

  uint8_t chksum_recv = hex2dec(recv_buf[bpos-2])*16 + hex2dec(recv_buf[bpos-1]);
  if (chksum != chksum_recv && bpos > 0) {
    LSL("Checksum failed!") LVH(chksum) LVH(recv_buf[bpos]) LVH(chksum_recv) LL
  }

  /* Now it's getting crazy but I don't want to use arduino/C++ stuff like Stream or String or phat sscanf!
  */

  uint32_t tmpl;
  uint16_t tmpi;
  uint8_t  tmpb;
  float tmpf;
  if ((recv_buf[0] == 'C' || recv_buf[0] == '8') && chksum == chksum_recv)
    switch (recv_buf[1]) {
      case '2':
        LS("Status: ")
        tmpb = hex2dec(recv_buf[2])*16 + hex2dec(recv_buf[3]);
        rmstate.error = (tmpb & 0x02);
        rmstate.button = (tmpb & 0x08);
        rmstate.alarm1 = (tmpb & 0x10);
        rmstate.onbatt = (tmpb & 0x20);
        LVB(tmpb)
        tmpb = hex2dec(recv_buf[4])*16 + hex2dec(recv_buf[5]);
        rmstate.battlow = (tmpb & 0x01);
        rmstate.smokealarm = (tmpb & 0x04);
        rmstate.wirealarm = (tmpb & 0x08);
        rmstate.rfalarm = (tmpb & 0x10);
        rmstate.localtestalarm = (tmpb & 0x20);
        rmstate.wiretestalarm = (tmpb & 0x40);
        rmstate.rftestalarm = (tmpb & 0x80);
        LVB(tmpb)
        tmpb = hex2dec(recv_buf[6])*16 + hex2dec(recv_buf[7]);
        rmstate.sbyte3 = tmpb;
        LVB(tmpb)
        tmpb = hex2dec(recv_buf[8])*16 + hex2dec(recv_buf[9]);
        rmstate.temp1err = (tmpb & 0x04);
        rmstate.temp2err = (tmpb & 0x10);
        rmstate.sbyte4 = tmpb;
        LVB(tmpb)
        LL
        break;
      case '4':
        LS("serial: ")
        tmpl = (uint32_t) (hex2dec(recv_buf[2])*16) << 24;
        tmpl += (uint32_t) (hex2dec(recv_buf[3])) << 24;
        tmpl += (uint32_t) (hex2dec(recv_buf[4])*16) << 16;
        tmpl += (uint32_t) (hex2dec(recv_buf[5])) << 16;
        tmpl += (uint32_t) (hex2dec(recv_buf[6])*16) << 8;
        tmpl += (uint32_t) (hex2dec(recv_buf[7])) << 8;
        tmpl += (uint32_t) (hex2dec(recv_buf[8])*16);
        tmpl += (uint32_t) (hex2dec(recv_buf[9]));
        rmstate.serial = tmpl;
        LV(tmpl) LL
        break;
      case '8':
        LS("?: ")
        tmpl = (uint32_t) (hex2dec(recv_buf[2])*16) << 24;
        tmpl += (uint32_t) (hex2dec(recv_buf[3])) << 24;
        tmpl += (uint32_t) (hex2dec(recv_buf[4])*16) << 16;
        tmpl += (uint32_t) (hex2dec(recv_buf[5])) << 16;
        tmpl += (uint32_t) (hex2dec(recv_buf[6])*16) << 8;
        tmpl += (uint32_t) (hex2dec(recv_buf[7])) << 8;
        tmpl += (uint32_t) (hex2dec(recv_buf[8])*16);
        tmpl += (uint32_t) (hex2dec(recv_buf[9]));
        LV(tmpl) LL
        break;
      case '9':
        LS("Runtime: ")
        tmpl = (uint32_t) (hex2dec(recv_buf[2])*16) << 24;
        tmpl += (uint32_t) (hex2dec(recv_buf[3])) << 24;
        tmpl += (uint32_t) (hex2dec(recv_buf[4])*16) << 16;
        tmpl += (uint32_t) (hex2dec(recv_buf[5])) << 16;
        tmpl += (uint32_t) (hex2dec(recv_buf[6])*16) << 8;
        tmpl += (uint32_t) (hex2dec(recv_buf[7])) << 8;
        tmpl += (uint32_t) (hex2dec(recv_buf[8])*16);
        tmpl += (uint32_t) (hex2dec(recv_buf[9]));
        rmstate.uptime = tmpl/4/60/60;
        LV(tmpl/4) LS(" sek = ") LV(tmpl/4/60/60) LS("h") LL
        break;
      case 'B':
        LS("Smoke: ")
        tmpi = (uint16_t) (hex2dec(recv_buf[2])*16) << 8;
        tmpi += (uint16_t) (hex2dec(recv_buf[3])) << 8;
        tmpi += (uint16_t) (hex2dec(recv_buf[4])*16);
        tmpi += (uint16_t) (hex2dec(recv_buf[5]));
        rmstate.smokeval = tmpi;
        LV(tmpi) LL //tmpf = tmpi *0.003223; // floats aren't good for uC
        tmpb = (hex2dec(recv_buf[6])*16);
        tmpb += (hex2dec(recv_buf[7]));
        rmstate.smokealarms = tmpb;
        LS("Smoke-Alarms: ") LV(tmpb) LL
        tmpb = (hex2dec(recv_buf[8])*16);
        tmpb += (hex2dec(recv_buf[9]));
        rmstate.smokedirt = tmpb;
        LS("Dirt: ") LV(tmpb) LL
        break;
      case 'C':
        tmpi = (uint16_t) (hex2dec(recv_buf[2])*16) << 8;
        tmpi += (uint16_t) (hex2dec(recv_buf[3])) << 8;
        tmpi += (uint16_t) (hex2dec(recv_buf[4])*16);
        tmpi += (uint16_t) (hex2dec(recv_buf[5]));
        tmpf = tmpi * 0.018369; //* 9184 / 5000
        rmstate.battvolt = tmpf;
        LS("Batt: ") LV(tmpf) LL
        tmpb = (hex2dec(recv_buf[6])*16);
        tmpb += (hex2dec(recv_buf[7]));
        tmpf = tmpb/2-20;
        rmstate.temp1 = tmpb/2-20;
        LS("Temp:") LV(tmpf) LL
        tmpb = (hex2dec(recv_buf[8])*16);
        tmpb += (hex2dec(recv_buf[9]));
        tmpf = tmpb/2-20;
        rmstate.temp2 = tmpb/2-20;
        LS("Temp:") LV(tmpf) LL
        break;
      case 'D':
        tmpb = (hex2dec(recv_buf[2])*16);
        tmpb += (hex2dec(recv_buf[3]));
        rmstate.localtempalarms = tmpb;
        LS("Therm-Alarms: ") LV(tmpb) LL
        tmpb = (hex2dec(recv_buf[4])*16);
        tmpb += (hex2dec(recv_buf[5]));
        rmstate.localtestalarms = tmpb;
        LS("Test-Alarms: ") LV(tmpb) LL
        tmpb = (hex2dec(recv_buf[6])*16);
        tmpb += (hex2dec(recv_buf[7]));
        rmstate.rfalarms = tmpb;
        LS("Remote-Alarms RF:") LV(tmpb) LL
        tmpb = (hex2dec(recv_buf[8])*16);
        tmpb += (hex2dec(recv_buf[9]));
        rmstate.wirealarms = tmpb;
        LS("Remote-Alarms Wire:") LV(tmpb) LL
        break;
      case 'E':
        tmpb = (hex2dec(recv_buf[2])*16);
        tmpb += (hex2dec(recv_buf[3]));
        rmstate.wiretestalarms = tmpb;
        LS("Test-Alarms Wire: ") LV(tmpb) LL
        tmpb = (hex2dec(recv_buf[4])*16);
        tmpb += (hex2dec(recv_buf[5]));
        rmstate.rftestalarms = tmpb;
        LS("Test-Alarms RF: ") LV(tmpb) LL
        break;
    }
    //TODO testing:
    /*
    07 00 08 2F - disable alarm by rf?
    07 00 20 29 - locate RM
    03 02 10 26 - set ALARM active (via rF)
    03 02 80 2D - set Testalarm (via rF)
    03 02 00 25 - end (Test)-Alarm
    */
}
/* useless echo
if (Serial2.available()) {
  LSL("S2-in:")
  while (Serial2.available()) {
    char tmp = Serial2.read();
    LVH(tmp)
    delay(1);
  }
}
*/

/*
    Here is the point where we want to do something useful with what we've got in rmstate like sending it..
*/

} //loop

char hex2dec(char buf) {
  if (buf > 47 && buf < 58)
    buf -= 48;
  else if (buf > 64 && buf < 71)
    buf -= 55;
  else if (buf > 96 && buf < 103)
    buf -= 87;
  else buf = 0;
  return buf;
}

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

