/*
 * OWSlave
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 *  any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http: * www.gnu.org/licenses/>.
 *
 * based on owdevice - A small 1-Wire emulator for AVR Microcontroller
 *
 * Copyright (C) 2012  Tobias Mueller mail (at) tobynet.de
 *
 * VERSION 1.4 for ATmega48/328P
 *
 * Created: 15.05.2013 13:36:59
 *  Author: Michael Markstaller
 *
 * use included Makefile: just change target MCU and avrdude params
 *
 * Changelog:
 * v1.4
 *  - moved code to 1.4 of counter with user-eeprom, massive power-savings and WDT-isr instead of delay
 * v1.3pre2
 *  - combined prototype for Counter / RM with full owfs-support as family E1
 * v1.3pre1:
 *  - Major cleanup code: define serial & debug-macros
 *  - adjust ISR to exact current defines in avr-libc, add uartlib (tiny24|44|84/25|45|85 missing yet! fits if UART is removed )
 *  - add EEPROM-functions
 *  - add basic ow function-commands instead of page/memory access used in DS2423
 *  - Family E1 introduced
 *  - much testcode
 * v1.2: basic code
 *
 * avrdude fuses attiny84
 * without EESAVE:    -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
 * with EESAVE:       -U lfuse:w:0xe2:m -U hfuse:w:0xd7:m -U efuse:w:0xff:m
 */


#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <string.h>
#include <avr/wdt.h>
#include "common.h"
// #include "uart.h"
// #include "uartsw.h" //avrlib
#include "mydefs_peda.h"
#include "suart.h"

#define DEBUG 0
#include "debug.h"
#ifndef F_CPU
# warning "F_CPU was not defined!! defining it now in debug.h but you should take care before!"
#define F_CPU 8000000UL //very important! define before delay.h as delay.h fucks up the serial-timing otherwise somehow
#endif
#include <util/delay.h>


#if defined (__AVR_ATtiny24__) || (__AVR_ATtiny44__) || (__AVR_ATtiny84__)
/* Port/Pin overview:
 * OW_PORT = Pin 5 - PB2 - INT0
 * UART_RX = Pin 6 - PA7 - ICP
 * UART_TX = Pin 7 - PA6 - variable but OC1 isn't usable anyway with sw-uart
 * ADC? - Pin 13-11 PA0-2
 */

//OW Pin
#define OW_PORT PORTB //1 Wire Port
#define OW_PIN PINB //1 Wire Pin as number
#define OW_PORTN (1<<PINB2)  //Pin as bit in registers
#define OW_PINN (1<<PINB2)
#define OW_DDR DDRB  //pin direction register
#define SET_LOW OW_DDR|=OW_PINN;OW_PORT&=~OW_PORTN;  //set 1-Wire line to low
#define RESET_LOW {OW_DDR&=~OW_PINN;}  //set 1-Wire pin as input
//Pin interrupt
#define EN_OWINT { GIMSK|=(1<<INT0); GIFR=(1<<INTF0); }  //enable interrupt and clear it! GIFR=(1<<INTF0);
#define DIS_OWINT  GIMSK&=~(1<<INT0); sleepmode=SLEEP_MODE_IDLE; //disable interrupt
#define SET_OWINT_RISING MCUCR=(1<<ISC01)|(1<<ISC00);  //set interrupt at rising edge
#define SET_OWINT_FALLING MCUCR=(1<<ISC01);MCUCR&=~(1<<ISC00); //set interrupt at falling edge FIXME/TODO: Check - MM added clearing ISC00 here!
#define SET_OWINT_BOTH MCUCR=(1<<ISC00);MCUCR&=~(1<<ISC01); //set interrupt at both edges
#define SET_OWINT_LOWLEVEL MCUCR&=~((1<<ISC01)|(1<<ISC00)); //set interrupt at low level

#define CHK_INT_EN (GIMSK&(1<<INT0))==(1<<INT0) //test if interrupt enabled
#define PIN_INT ISR(INT0_vect)  // the interrupt service routine
//Timer Interrupt
#define EN_TIMER {TIMSK0 |= (1<<TOIE0); TIFR0|=(1<<TOV0); } //enable timer interrupt
#define DIS_TIMER TIMSK0  &= ~(1<<TOIE0); // disable timer interrupt + IDLE-Sleep?
#define TCNT_REG TCNT0  //register of timer-counter
#define TIMER_INT ISR(TIM0_OVF_vect) //the timer interrupt service routine

#define RMRXLED_PORT PORTA
#define RMRXLED_DDR DDRA
#define RMRXLED_PIN PINA3

#define OWRXLED_PORT PORTA
#define OWRXLED_DDR DDRA
#define OWRXLED_PIN PINA4

/* PWRDOWN: use PORTB / same PORT as counters ! */
#define PWRDOWN_PORT PORTB
#define PWRDOWN_DDR DDRB
#define PWRDOWN_PIN PINB0

#define INIT_LED_PINS RMRXLED_DDR |= (1<<RMRXLED_PIN); \
                      OWRXLED_DDR |= (1<<OWRXLED_PIN); /* pins as output */

#define OWT_MIN_RESET 51
#define OWT_RESET_PRESENCE 4
#define OWT_PRESENCE 20
#define OWT_READLINE 2 //2 if clock 7.8MHz!
#define OWT_LOWTIME 3 //3 for fast master

//Initializations of AVR
#define INIT_AVR CLKPR=(1<<CLKPCE); \
                   CLKPR=0; /*8Mhz*/  \
                   TIMSK0=0; \
                   GIMSK=(1<<INT0);  /*set direct GIMSK register*/ \
                   TCCR0B=(1<<CS00)|(1<<CS01); /*8mhz /64 couse 8 bit Timer interrupt every 8us*/ \
                   WDTCSR |= ((1<<WDP2)|(1<<WDP1)|(1<<WDP0)); /* ((1<<WDP2)|(1<<WDP1)|(1<<WDP0)) WDT_ISR every 2s - (1<<WDP3) every 4s */ \
                   WDTCSR |= (1<<WDIE); /* only enable int, no real watchdog */

//FIXME: maybe activate PCINT7 on PA7/RX to wakeup from power-down if serial data gets in?

#define PWRSAVE_AVR ADCSRA &= ~(1<<ADEN); PRR |= (1<<PRUSI)|(1<<PRADC); //PRR |= (1<<PRUSI)|(1<<PRADC); //PRR |= (1<<PRTIM1)|(1<<PRUSI)|(1<<PRADC);

#define PC_INT_ISRA ISR(PCINT0_vect) { /*Attiny84 - PAx is PCINT0 */ \
                    if (((PINA&(1<<PINA3))==0)&&((pinstatA&(1<<PINA3))==(1<<PINA3))) { Counter2++; }       \
                    if (((PINA&(1<<PINA4))==0)&&((pinstatA&(1<<PINA4))==(1<<PINA4))) { Counter3++; }       \
                    if (((PINA&(1<<PINA5))==0)&&((pinstatA&(1<<PINA5))==(1<<PINA5))) { Counter4++; }       \
                    pinstatA=PINA;}    \

#define PC_INT_ISRB ISR(PCINT1_vect) { /*Attiny84 - PBx is PCINT1 */ \
                    if (((PINB&(1<<PINB1))==0)&&((pinstatB&(1<<PINB1))==(1<<PINB1))) { Counter1++; }       \
                    if (((PINB&(1<<PINB0))==0)&&((pinstatB&(1<<PINB0))==(1<<PINB0))) {               \
                          /* FIXME: ints off for eeprom-write ?? eeprom own routines to save much space? */ \
                          eeprom_update_dword((uint32_t *) (EE_COUNTER_OFFSET+0),Counter1);\
                          eeprom_update_dword((uint32_t *) (EE_COUNTER_OFFSET+4),Counter2);\
                          eeprom_update_dword((uint32_t *) (EE_COUNTER_OFFSET+8),Counter3);\
                          eeprom_update_dword((uint32_t *) (EE_COUNTER_OFFSET+12),Counter4); } \
                    pinstatB=PINB;}    \

#define INIT_COUNTER_PINS /* Counter Interrupt */\
                        GIMSK|=(1<<PCIE0);\
                        GIMSK|=(1<<PCIE1);\
                        PCMSK1|=(1<<PCINT9); /* PB1 PCINT */ \
                        PCMSK1|= (1<<PCINT8); /* enable PCINT8 PB0 */ \
                        DDRB &=~((1<<PINB1));  /* Counter-pins as input */ \
                        PORTB |= ( (1<<PB1) ); /* activate internal Pull-Up PB1 */ \
                        DDRB &=~((1<<PINB0)); /* PB0 as input */ \
                        pinstatB=PINB; \
                        PCMSK0|=((1<<PCINT3)|(1<<PCINT4)|(1<<PCINT5)); /* PA3-5 PCINT */ \
                        DDRA &=~((1<<PINA3)|(1<<PINA4)|(1<<PINA5));  /* Counter-pins PA3-5 as input */ \
                        PORTA |= ( (1<<PA3)|(1<<PA4)|(1<<PA5) ); /* activate internal Pull-Up PA3-5 */ \
                        pinstatA=PINA; \

#endif // __AVR_ATtiny84__


//States / Modes (defines from original owslave.c - new are all called OWC_ !)
#define OWM_SLEEP 0  //Waiting for next reset pulse
#define OWM_RESET 1  //Reset pulse received
#define OWM_PRESENCE 2  //sending presence pulse
#define OWM_READ_COMMAND 3 //read 8 bit of command
#define OWM_SEARCH_ROM 4  //SEARCH_ROM algorithms
#define OWM_MATCH_ROM 5  //test number
#define OWM_CHK_RESET 8  //waiting of rising edge from reset pulse
#define OWM_GET_ADRESS 6
#define OWM_READ_MEMORY_COUNTER 7
#define OWM_WRITE_SCRATCHPAD 9
#define OWM_READ_SCRATCHPAD 10

#define OWM_WRITE_PAGE_TO_MASTER 11
#define OWM_WRITE_FUNC 12

#define OWC_READ_SCRATCHPAD 0xBE
#define OWC_WRITE_SCRATCHPAD 0x4E
#define OWC_WRITE_FUNC 0x4F
/* READ_SCRATCHPAD 0xBE + 0xYY is adaptec from DS2438! BE + Address
 *
 */

//Write a bit after next falling edge from master
//its for sending a zero as soon as possible
#define OWW_NO_WRITE 2
#define OWW_WRITE_1 1
#define OWW_WRITE_0 0

LV_SETUP  //quirk to define itoa-outbuf..

volatile uint32_t uptime = 0; /* holds uptime in 1/2 seconds - overflows after 6.8 years */

typedef union {
    volatile uint8_t bytes[11];
    struct {
      uint8_t   page1;
      uint8_t   u8_11;
      uint8_t   u8_12;
      uint16_t  u16_13;
      uint32_t  u32_14;
      uint8_t   crc1;
    };
    struct {
      uint8_t   page2;
      uint16_t  u16_21;
      uint16_t  u16_22;
      uint16_t  u16_23;
      uint8_t   u16_24;
      uint8_t   crc2;
    };
    struct {
      uint8_t   page3;
      uint32_t  u32_31;
      uint32_t  u32_32;
      uint8_t   crc3;
    };
} scratchpad_t;
scratchpad_t scratchpad;

typedef union {
  uint8_t bytes[8];
  struct {
    uint8_t b1;
    uint8_t b2;
    uint8_t b3;
    uint8_t b4;
    uint8_t b5;
    uint8_t b6;
    uint8_t b7;
    uint8_t b8;
  };
  struct {
    uint32_t u32_1;
    uint32_t u32_2;
  };
  struct {
    uint16_t u16_1;
    uint16_t u16_2;
    uint16_t u16_3;
    uint16_t u16_4;
  };
} rmdata_t;
rmdata_t rmdata[5];

/* recv-states */
enum {
      S_NULL = 0,
      S_STX = 2,
      S_ETX = 3,
      S_ACK = 6,
      S_NACK = 15,
      S_DATA = 20,
};
uint8_t recv_state = S_NULL;
uint8_t nackmsg=0;

volatile uint16_t scrc; //CRC calculation
volatile uint8_t page; /* address of memory-page to read/write */

volatile uint8_t lastcps;
volatile uint8_t istat;
volatile uint8_t sleepmode;

volatile uint8_t cbuf; //Input buffer for a command
uint8_t owid[8] = {0xE1, 0x00, 0x00, 0x00, 0x00, 0x05, 0x84, 0x90 };

volatile uint8_t bitp;  //pointer to current Byte
volatile uint8_t bytep; //pointer to current Bit

volatile uint8_t mode; //state
volatile uint8_t wmode; //if 0 next bit that send the device is  0
volatile uint8_t actbit; //current
volatile uint8_t srcount; //counter for search rom

/* temp vars to avoid eeprom-reading - in case of low-mem: FIXME */
uint16_t version = 0x0105;
uint8_t stype = 2;
uint16_t rcnt = 1;
uint8_t eflag; //internal error/status-flag
uint8_t serflag;
volatile uint8_t crcerrcnt=0;

volatile uint8_t ewrite_flag;//ewrite_flag is already volatile and used as semaphore..
typedef union {
    uint8_t bytes[8];
    struct {
      uint32_t u32_1;
      uint32_t u32_2;
    };
} eewrite_t;
eewrite_t eewrite_buf;


PIN_INT {
    uint8_t lwmode=wmode;  //let this variables in registers
    uint8_t lmode=mode;
    if (lwmode==OWW_WRITE_0) { //if necessary set 0-Bit
        SET_LOW;
        lwmode=OWW_NO_WRITE;
    }
    DIS_OWINT; //disable interrupt, only in OWM_SLEEP mode it is active
    sleepmode=SLEEP_MODE_IDLE; //powerdown is set in TIMER_INT on OWM_SLEEP only!
    switch (lmode) {
        case OWM_SLEEP:
            TCNT_REG=~(OWT_MIN_RESET);
            //RESET_LOW;  //??? Set pin as input again ???
            EN_OWINT; SET_OWINT_RISING; //other edges ?
            OWRXLED_PORT &= ~(1<<OWRXLED_PIN); /* led off */
            break;
        //start of reading a byte with falling edge from master, reading closed in timer isr
        case OWM_MATCH_ROM:  //falling edge wait for receive
        case OWM_WRITE_SCRATCHPAD:
        case OWM_GET_ADRESS:
        case OWM_READ_COMMAND:
        case OWM_WRITE_FUNC:
            TCNT_REG=~(OWT_READLINE); //wait a time for reading
            break;
        case OWM_SEARCH_ROM:   //Search algorithm waiting for receive or send
            if (srcount<2) { //this means bit or complement is writing,
                TCNT_REG=~(OWT_LOWTIME);
            } else
                TCNT_REG=~(OWT_READLINE);  //init for read answer of master
            break;
        case OWM_READ_SCRATCHPAD:
        case OWM_WRITE_PAGE_TO_MASTER:
            TCNT_REG=~(OWT_LOWTIME);
            break;
        case OWM_CHK_RESET:  //rising edge of reset pulse
            SET_OWINT_FALLING;
            TCNT_REG=~(OWT_RESET_PRESENCE);  //waiting for sending presence pulse
            lmode=OWM_RESET;
            break;
    }
    EN_TIMER;
    mode=lmode;
    wmode=lwmode;
}


TIMER_INT {
    uint8_t lwmode=wmode; //let this variables in registers
    uint8_t lmode=mode;
    uint8_t lbytep=bytep;
    uint8_t lbitp=bitp;
    uint8_t lsrcount=srcount;
    uint8_t lactbit=actbit;
    uint16_t lscrc=scrc;
    //Ask input line sate
    uint8_t p=((OW_PIN&OW_PINN)==OW_PINN);

    OWRXLED_PORT ^= (1<<OWRXLED_PIN); /* toolge RX-led */

    //Interrupt still active ?
    if (CHK_INT_EN) {
        //maybe reset pulse
        if (p==0) {
            lmode=OWM_CHK_RESET;  //wait for rising edge
            SET_OWINT_RISING;
        }
        DIS_TIMER;
    } else
    switch (lmode) {
        case OWM_RESET:  //Reset pulse and time after is finished, now go in presence state
            lmode=OWM_PRESENCE;
            SET_LOW;
            TCNT_REG=~(OWT_PRESENCE);
            DIS_OWINT;  //No Pin interrupt necessary only wait for presence is done
            break;
        case OWM_PRESENCE:
            RESET_LOW;  //Presence is done now wait for a command
            lmode=OWM_READ_COMMAND;
            cbuf=0;lbitp=1;  //Command buffer have to set zero, only set bits will write in
            break;
        case OWM_READ_COMMAND:
            if (p) {  //Set bit if line high
                cbuf|=lbitp;
            }
            lbitp=(lbitp<<1);
            if (!lbitp) { //8-Bits read - weird syntax?
                lbitp=1;
                switch (cbuf) {
                    case 0x55://Match ROM
                        lbytep=0;
                        lmode=OWM_MATCH_ROM;
                        break;
                    case 0xF0:  //initialize search rom
                        lmode=OWM_SEARCH_ROM;
                        lsrcount=0;
                        lbytep=0;
                        lactbit=(owid[lbytep]&lbitp)==lbitp; //set actual bit
                        lwmode=lactbit;  //prepare for writing when next falling edge
                        break;
                    //FIXME: case 0xEC:  //alarm search rom - TODO
                    case 0xBE: //read scratchpad
                        lmode=OWM_GET_ADRESS; //first the master sends an address
                        lbytep=0;
                        page=0;
                        //EN_OWINT; SET_OWINT_LOWLEVEL; sleepmode=SLEEP_MODE_PWR_DOWN; //Testing if we can charge-up?
                        break;
                    case OWC_WRITE_FUNC:
                        lmode=OWM_WRITE_FUNC; //first the master sends an address(page/func), then 8bytes + crc
                        lbytep=0;
                        lscrc=0;
                        scratchpad.page1=0;
                        break;
                    default:
                        LSL("\r\nDC:")
                        //LVH(cbuf)
                        lmode=OWM_SLEEP;  //all other commands do nothing
                }
            }
            break;
        case OWM_SEARCH_ROM:
            RESET_LOW;  //Set low also if nothing send (branch takes time and memory)
            lsrcount++;  //next search rom mode
            switch (lsrcount) {
                case 1:lwmode=!lactbit;  //preparation sending complement
                    break;
                case 3:
                    if (p!=(lactbit==1)) {  //check master bit
                        lmode=OWM_SLEEP;  //not the same go sleep
                    } else {
                        lbitp=(lbitp<<1);  //prepare next bit
                        if (lbitp==0) {
                            lbitp=1;
                            lbytep++;
                            if (lbytep>=8) {
                                lmode=OWM_SLEEP;  //all bits processed
                                break;
                            }
                        }
                        lsrcount=0;
                        lactbit=(owid[lbytep]&lbitp)==lbitp;
                        lwmode=lactbit;
                    }
                    break;
            }
            break;
        case OWM_MATCH_ROM:
            if (p==((owid[lbytep]&lbitp)==lbitp)) {  //Compare with ID Buffer
                lbitp=(lbitp<<1);
                if (!lbitp) {
                    lbytep++;
                    lbitp=1;
                    if (lbytep>=8) {
                        lmode=OWM_READ_COMMAND;  //same? get next command
                        cbuf=0;
                        break;
                    }
                }
            } else {
                lmode=OWM_SLEEP;
            }
            break;
        case OWM_GET_ADRESS:
            if (p) { //Get the Address for reading
                page|=lbitp;
            }
            lbitp=(lbitp<<1);
            if (!lbitp) {
                lbytep++;
                lbitp=1;
                if (lbytep==1) {
                    lmode=OWM_WRITE_PAGE_TO_MASTER;
                    lbytep=0;lscrc=0; //from first position
                    memset( &scratchpad.bytes, 0, 10 );
                    uint8_t i=0;
                    switch (page) {
                      case 0:
                        scratchpad.page1 = page;
                        scratchpad.u8_11 = stype;
                        scratchpad.u8_12 = eflag;
                        scratchpad.u16_13 = version;
                        scratchpad.u32_14 = uptime;
                        break;
                      case 1:
                        scratchpad.page1 = page;
                        scratchpad.u16_21 = rcnt;
                        //scratchpad.u16_22 = ADC1
                        //scratchpad.u16_23 = ADC2
                        scratchpad.u16_24 = crcerrcnt; //temp/debug
                        //scratchpad.u16_24 = freeRam;
                        break;
                      case 2:
                        scratchpad.page1 = page;
                        //scratchpad.u32_31 = Counter1;
                        //scratchpad.u32_32 = Counter2;
                        break;
                      case 3:
                        scratchpad.page1 = page;
                        //scratchpad.u32_31 = Counter3;
                        //scratchpad.u32_32 = Counter4;
                        break;
                      case 4:
                        scratchpad.page1 = page;
                        for (i=0;i<8;i++)
                          scratchpad.bytes[1+i] =rmdata[0].bytes[i];
                        break;
                      case 5:
                        scratchpad.page1 = page;
                        for (i=0;i<8;i++)
                          scratchpad.bytes[1+i] =rmdata[1].bytes[i];
                        break;
                      case 6:
                        scratchpad.page1 = page;
                        for (i=0;i<8;i++)
                          scratchpad.bytes[1+i] =rmdata[2].bytes[i];
                        break;
                      case 7:
                        scratchpad.page1 = page;
                        for (i=0;i<8;i++)
                          scratchpad.bytes[1+i] =rmdata[3].bytes[i];
                        break;
                      case 8:
                        scratchpad.page1 = page;
                        for (i=0;i<8;i++)
                          scratchpad.bytes[1+i] =rmdata[4].bytes[i];
                        //memcpy(*scratchpad+1,&rmdata[4],8);
                        break;
                      case 9:
                        scratchpad.page1 = page;
                        scratchpad.u32_31 = eeprom_read_dword((const uint32_t *) (EE_LABEL_OFFSET+0));
                        scratchpad.u32_32 = eeprom_read_dword((const uint32_t *) (EE_LABEL_OFFSET+4));
                        break;
                      case 10:
                        scratchpad.page1 = page;
                        scratchpad.u32_31 = eeprom_read_dword((const uint32_t *) (EE_LABEL_OFFSET+8));
                        scratchpad.u32_32 = eeprom_read_dword((const uint32_t *) (EE_LABEL_OFFSET+12));
                        break;
                      default:
                        scratchpad.page1 = page;
                        scratchpad.u8_11 = 0xff;
                        scratchpad.u8_12 = page; //this is actually an error! page unknown
                        break;
                    }
                    lactbit=(lbitp&scratchpad.bytes[0])==lbitp;
                    lwmode=lactbit; //prepare for send firs bit
                    break;
                } else page=0; // never happens, should be page[lbytepos]
            }
            break;
        case OWM_WRITE_FUNC:
            if (p) {
                scratchpad.bytes[lbytep]|=lbitp;
            }
            /* Page(function) is part of CRC! */
            if ((lscrc&1)!=p) lscrc=(lscrc>>1)^0x8c; else lscrc >>=1;
            lbitp=(lbitp<<1);
            if (!lbitp) {
                lbytep++;
                lbitp=1;
                if (lbytep==10) {
                    /* now process received Write-function(s) if crc matches */
                    if (scratchpad.bytes[9] != scratchpad.bytes[10])
                      crcerrcnt++;
                    else {
                      switch (scratchpad.bytes[0]) { /* "page" or function-id */
                        case 10: /* set / reset smokealarm */
                          {
                            /*ALALRM:
                            uint8_t req2[] = {0x02, 0x30, 0x33, 0x30, 0x32, 0x31, 0x30, 0x32, 0x36, 0x03 };
                            TESTALARM
                            uint8_t req2[] = {0x02, 0x30, 0x33, 0x30, 0x32, 0x38, 0x30, 0x32, 0x44, 0x03 };
                            */
                            uint8_t req2[] = {0x02, 0x30, 0x33, 0x30, 0x32, 0x30, 0x30, 0x32, 0x35, 0x03 };
                            if (scratchpad.bytes[1] == 0) {
                              /* default is silence alarm */
                            } else if (scratchpad.bytes[1] == 1) {
                              req2[5]=0x31;req2[8]=0x36;
                            } else if (scratchpad.bytes[1] == 2) {
                              req2[5]=0x38;req2[8]=0x44;
                            }
                            for (int i=0;i<sizeof(req2);i++)
                                uputchar(req2[i]);
                          }
                          break;
                        case 72:
                        case 76:
                        case 80:
                        case 84:
                          if (ewrite_flag > 0) //last write still pending!
                            break;
                          eewrite_buf.u32_1 = scratchpad.u32_31;
                          ewrite_flag = (scratchpad.page1) - 72 + EE_LABEL_OFFSET;
                          break;
                        default:
                          if (scratchpad.page1 > 71 && scratchpad.page1 < 72+EE_LABEL_MAXLEN)
                            ; //alternatively: just write it?
                          break;
                      }
                    }
                    lmode=OWM_SLEEP;
                    break;
                } else scratchpad.bytes[lbytep]=0;
                if (lbytep==9) {
                    //copy calculated CRC to last scratchpad-byte as we receive it with next byte!
                    scratchpad.bytes[10] = lscrc;
                }
            }
            break;
        case OWM_WRITE_PAGE_TO_MASTER:
            RESET_LOW;
            if ((lscrc&1)!=lactbit) lscrc=(lscrc>>1)^0x8c; else lscrc >>=1;
            lbitp=(lbitp<<1);
            if (!lbitp) {
                lbytep++;
                lbitp=1;
                if (lbytep>=10) {
                    lmode=OWM_SLEEP;
                    break;
                } else if (lbytep==9) scratchpad.bytes[9]=lscrc;
            }
            lactbit=(lbitp&scratchpad.bytes[lbytep])==lbitp;
            lwmode=lactbit;
            break;
        }
        if (lmode==OWM_SLEEP) {
          //RESET_LOW;  //??? Set pin as input again ???
          DIS_TIMER;
          EN_OWINT; SET_OWINT_LOWLEVEL;
          sleepmode=SLEEP_MODE_PWR_DOWN; //sleep deep NOTE: for other sleep-mode also change OWINT != LOWLEVEL!
          OWRXLED_PORT &= ~(1<<OWRXLED_PIN); /* led off */
        } else {
          //sleepmode=SLEEP_MODE_IDLE; //no sleep
        }

        if (lmode!=OWM_PRESENCE)  {
            TCNT_REG=~(OWT_MIN_RESET-OWT_READLINE);  //OWT_READLINE around OWT_LOWTIME
            EN_OWINT;
        }
        mode=lmode;
        wmode=lwmode;
        bytep=lbytep;
        bitp=lbitp;
        srcount=lsrcount;
        actbit=lactbit;
        scrc=lscrc;
}

//PC_INT_ISR  //for counting  defined for specific device
ISR(PCINT0_vect) { }
ISR(PCINT1_vect) { }

void init_eeprom(void) {
    /* check magic, read slave address and counter values, resetcount, init-name, */
    if (eeprom_read_word((const uint16_t *) (EE_MAGIC_OFFSET+0)) == EE_MAGIC_NUMBER) {
      //EEPROM valid -> read counters & settings
      for (uint16_t i=EE_OWID_OFFSET;i<EE_OWID_OFFSET+8;i++)
        owid[i-EE_OWID_OFFSET] = eeprom_read_byte((uint8_t *) i);
      rcnt = eeprom_read_word((const uint16_t *) (EE_RCNT_OFFSET+0)) + 1;
      eeprom_update_word((uint16_t *) (EE_RCNT_OFFSET+0), rcnt);
/*
      Counter1 = eeprom_read_dword((const uint32_t *) (EE_COUNTER_OFFSET+0));
      Counter2 = eeprom_read_dword((const uint32_t *) (EE_COUNTER_OFFSET+4));
      Counter3 = eeprom_read_dword((const uint32_t *) (EE_COUNTER_OFFSET+8));
      Counter4 = eeprom_read_dword((const uint32_t *) (EE_COUNTER_OFFSET+12));
*/
      version = eeprom_read_word((const uint16_t *) (EE_VERSION_OFFSET+0));
      stype = eeprom_read_byte((uint8_t *) (EE_TYPE_OFFSET+0));
    } else {
      //Init values
      /* should cli(); here no sei(); yet enabled in main.. */
      eeprom_write_word((uint16_t *) (EE_MAGIC_OFFSET+0), EE_MAGIC_NUMBER);
      for (uint16_t i=EE_OWID_OFFSET;i<EE_OWID_OFFSET+8;i++)
        eeprom_write_byte((uint8_t *) i,owid[i-EE_OWID_OFFSET]);
      eeprom_write_word((uint16_t *) (EE_RCNT_OFFSET+0), rcnt);
/*
      eeprom_write_dword((uint32_t *) (EE_COUNTER_OFFSET+0),0);
      eeprom_write_dword((uint32_t *) (EE_COUNTER_OFFSET+4),0);
      eeprom_write_dword((uint32_t *) (EE_COUNTER_OFFSET+8),0);
      eeprom_write_dword((uint32_t *) (EE_COUNTER_OFFSET+12),0);
*/
      eeprom_write_byte((uint8_t *) (EE_TYPE_OFFSET+0), stype);
      eeprom_write_word((uint16_t *) (EE_VERSION_OFFSET+0), version);
    }
}

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


void processSerial(void) {
    uint8_t sbuf;
    char recv_buf[16];
    uint8_t bpos = 0;
    uint16_t chksum = 0;

    while( kbhit() ) {
      sbuf = ugetchar();
      RMRXLED_PORT ^= (1<<RMRXLED_PIN);
      switch (sbuf) {
        case 0x00:
            recv_state = S_NULL;
            bpos=0; chksum=0;
            break;
        case 0x02:
            recv_state = S_STX;
            bpos=0; chksum=0;
            break;
        case 0x03:
            recv_state = S_ETX;
            //FIXME: sendack only if chksum is ok!
            uputchar(0x06); // send ACK
            break;
        case 0x06:
            recv_state = S_ACK;
            bpos=0; chksum=0;
            break;
        case 0x15:
            recv_state = S_NULL;
            nackmsg++;
            bpos=0; chksum=0;
            break;
        default: //ASCII
            if (recv_state == S_STX) {
              recv_state = S_DATA;
              bpos=0; chksum=0;
            }
            break;
      }
      switch (recv_state) {
        case S_DATA:
            recv_buf[bpos] = sbuf;
            chksum += sbuf;
            if (bpos < sizeof (recv_buf))
              bpos++;
            break;
      }
      _delay_ms(1); // FIXME: sleep a little..
    }

    //quirk, substract last two bytes
    chksum -= recv_buf[bpos-2] + recv_buf[bpos-1];
    chksum &= 0xFF;

    uint8_t chksum_recv = hex2dec(recv_buf[bpos-2])*16 + hex2dec(recv_buf[bpos-1]);
    if (chksum != chksum_recv && bpos > 0)
      eflag |= (1<<5);
    else if (chksum == chksum_recv && bpos > 0)
      eflag &= ~(1<<5);

    if ((recv_buf[0] == 'C' || recv_buf[0] == '8') && chksum == chksum_recv) {
      switch (recv_buf[1]) {
        case '2':
          LS("Status: ")
          /* just push the bytes into buffer - no decode .. */
          rmdata[0].b1 = hex2dec(recv_buf[2])*16 + hex2dec(recv_buf[3]);
          /*
          tmpb = hex2dec(recv_buf[2])*16 + hex2dec(recv_buf[3]);
          rmstate.error = (tmpb & 0x02);
          rmstate.button = (tmpb & 0x08);
          rmstate.alarm1 = (tmpb & 0x10);
          rmstate.onbatt = (tmpb & 0x20);
          LVB(tmpb)
          */
          rmdata[0].b2 = hex2dec(recv_buf[4])*16 + hex2dec(recv_buf[5]);
          /*
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
          */
          rmdata[0].b3 = hex2dec(recv_buf[6])*16 + hex2dec(recv_buf[7]);
          /*
          rmstate.sbyte3 = tmpb;
          LVB(tmpb)
          tmpb = hex2dec(recv_buf[8])*16 + hex2dec(recv_buf[9]);
          */
          rmdata[0].b4 = hex2dec(recv_buf[8])*16 + hex2dec(recv_buf[9]);
          /*
          rmstate.temp1err = (tmpb & 0x04);
          rmstate.temp2err = (tmpb & 0x10);
          rmstate.sbyte4 = tmpb;
          LVB(tmpb)
          LL
          */
          break;
        case '4':
          rmdata[0].u32_2 = (uint32_t) (hex2dec(recv_buf[2])*16) << 24;
          rmdata[0].u32_2 += (uint32_t) (hex2dec(recv_buf[3])) << 24;
          rmdata[0].u32_2 += (uint32_t) (hex2dec(recv_buf[4])*16) << 16;
          rmdata[0].u32_2 += (uint32_t) (hex2dec(recv_buf[5])) << 16;
          rmdata[0].u32_2 += (uint32_t) (hex2dec(recv_buf[6])*16) << 8;
          rmdata[0].u32_2 += (uint32_t) (hex2dec(recv_buf[7])) << 8;
          rmdata[0].u32_2 += (uint32_t) (hex2dec(recv_buf[8])*16);
          rmdata[0].u32_2 += (uint32_t) (hex2dec(recv_buf[9]));
          /*
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
          */
          break;
        case '8':
          rmdata[1].b1 = (hex2dec(recv_buf[2])*16);
          rmdata[1].b1 += (hex2dec(recv_buf[3]));
          rmdata[1].b2 = (hex2dec(recv_buf[4])*16);
          rmdata[1].b2 += (hex2dec(recv_buf[5]));
          rmdata[1].b3 = (hex2dec(recv_buf[6])*16);
          rmdata[1].b3 += (hex2dec(recv_buf[7]));
          rmdata[1].b4 = (hex2dec(recv_buf[8])*16);
          rmdata[1].b4 += (hex2dec(recv_buf[9]));
          /*
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
          */
          break;
        case '9':
          rmdata[1].u32_2 = (uint32_t) (hex2dec(recv_buf[2])*16) << 24;
          rmdata[1].u32_2 += (uint32_t) (hex2dec(recv_buf[3])) << 24;
          rmdata[1].u32_2 += (uint32_t) (hex2dec(recv_buf[4])*16) << 16;
          rmdata[1].u32_2 += (uint32_t) (hex2dec(recv_buf[5])) << 16;
          rmdata[1].u32_2 += (uint32_t) (hex2dec(recv_buf[6])*16) << 8;
          rmdata[1].u32_2 += (uint32_t) (hex2dec(recv_buf[7])) << 8;
          rmdata[1].u32_2 += (uint32_t) (hex2dec(recv_buf[8])*16);
          rmdata[1].u32_2 += (uint32_t) (hex2dec(recv_buf[9]));
          /*
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
          */
          break;
        case 'B':
          rmdata[2].u16_1 = (uint16_t) (hex2dec(recv_buf[2])*16) << 8;
          rmdata[2].u16_1 += (uint16_t) (hex2dec(recv_buf[3])) << 8;
          rmdata[2].u16_1 += (uint16_t) (hex2dec(recv_buf[4])*16);
          rmdata[2].u16_1 += (uint16_t) (hex2dec(recv_buf[5]));
          /*
          LS("Smoke: ")
          tmpi = (uint16_t) (hex2dec(recv_buf[2])*16) << 8;
          tmpi += (uint16_t) (hex2dec(recv_buf[3])) << 8;
          tmpi += (uint16_t) (hex2dec(recv_buf[4])*16);
          tmpi += (uint16_t) (hex2dec(recv_buf[5]));
          rmstate.smokeval = tmpi;
          */
          rmdata[2].b3 = (hex2dec(recv_buf[6])*16);
          rmdata[2].b3 += (hex2dec(recv_buf[7]));
          /*
          LV(tmpi) LL //tmpf = tmpi *0.003223; // floats aren't good for uC
          tmpb = (hex2dec(recv_buf[6])*16);
          tmpb += (hex2dec(recv_buf[7]));
          rmstate.smokealarms = tmpb;
          */
          rmdata[2].b4 = (hex2dec(recv_buf[8])*16);
          rmdata[2].b4 += (hex2dec(recv_buf[9]));
          /*
          LS("Smoke-Alarms: ") LV(tmpb) LL
          tmpb = (hex2dec(recv_buf[8])*16);
          tmpb += (hex2dec(recv_buf[9]));
          rmstate.smokedirt = tmpb;
          LS("Dirt: ") LV(tmpb) LL
          */
          break;
        case 'C':
          rmdata[2].u16_3 = (uint16_t) (hex2dec(recv_buf[2])*16) << 8;
          rmdata[2].u16_3 += (uint16_t) (hex2dec(recv_buf[3])) << 8;
          rmdata[2].u16_3 += (uint16_t) (hex2dec(recv_buf[4])*16);
          rmdata[2].u16_3 += (uint16_t) (hex2dec(recv_buf[5]));
          /*
          tmpi = (uint16_t) (hex2dec(recv_buf[2])*16) << 8;
          tmpi += (uint16_t) (hex2dec(recv_buf[3])) << 8;
          tmpi += (uint16_t) (hex2dec(recv_buf[4])*16);
          tmpi += (uint16_t) (hex2dec(recv_buf[5]));
          tmpf = tmpi * 0.018369; // * 9184 / 5000
          rmstate.battvolt = tmpf;
          LS("Batt: ") LV(tmpf) LL
          */
          rmdata[2].b7 = (hex2dec(recv_buf[6])*16);
          rmdata[2].b7 += (hex2dec(recv_buf[7]));
          /*
          tmpb = (hex2dec(recv_buf[6])*16);
          tmpb += (hex2dec(recv_buf[7]));
          tmpf = tmpb/2-20;
          rmstate.temp1 = tmpb/2-20;
          LS("Temp:") LV(tmpf) LL
          */
          rmdata[2].b8 = (hex2dec(recv_buf[8])*16);
          rmdata[2].b8 += (hex2dec(recv_buf[9]));
          /*
          tmpb = (hex2dec(recv_buf[8])*16);
          tmpb += (hex2dec(recv_buf[9]));
          tmpf = tmpb/2-20;
          rmstate.temp2 = tmpb/2-20;
          LS("Temp:") LV(tmpf) LL
          */
          break;
        case 'D':
          rmdata[3].b1 = (hex2dec(recv_buf[2])*16);
          rmdata[3].b1 += (hex2dec(recv_buf[3]));
          rmdata[3].b2 = (hex2dec(recv_buf[4])*16);
          rmdata[3].b2 += (hex2dec(recv_buf[5]));
          rmdata[3].b3 = (hex2dec(recv_buf[6])*16);
          rmdata[3].b3 += (hex2dec(recv_buf[7]));
          rmdata[3].b4 = (hex2dec(recv_buf[8])*16);
          rmdata[3].b4 += (hex2dec(recv_buf[9]));
          /*
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
          */
          break;
        case 'E':
          rmdata[3].b5 = (hex2dec(recv_buf[2])*16);
          rmdata[3].b5 += (hex2dec(recv_buf[3]));
          rmdata[3].b6 = (hex2dec(recv_buf[4])*16);
          rmdata[3].b6 += (hex2dec(recv_buf[5]));
          /*
          tmpb = (hex2dec(recv_buf[2])*16);
          tmpb += (hex2dec(recv_buf[3]));
          rmstate.wiretestalarms = tmpb;
          LS("Test-Alarms Wire: ") LV(tmpb) LL
          tmpb = (hex2dec(recv_buf[4])*16);
          tmpb += (hex2dec(recv_buf[5]));
          rmstate.rftestalarms = tmpb;
          LS("Test-Alarms RF: ") LV(tmpb) LL
          */
          break;
        case 'F': //unknwon!
          rmdata[4].b1 = (hex2dec(recv_buf[2])*16);
          rmdata[4].b1 += (hex2dec(recv_buf[3]));
          rmdata[4].b2 = (hex2dec(recv_buf[4])*16);
          rmdata[4].b2 += (hex2dec(recv_buf[5]));
          rmdata[4].b3 = (hex2dec(recv_buf[6])*16);
          rmdata[4].b3 += (hex2dec(recv_buf[7]));
          rmdata[4].b4 = (hex2dec(recv_buf[8])*16);
          rmdata[4].b4 += (hex2dec(recv_buf[9]));
      }
      serflag=0; /* we got something, so clear errorflag */
      RMRXLED_PORT ^= (1<<RMRXLED_PIN); /* toggle led */
    }
}


//FIXME: enable real watchdog?
ISR(WDT_vect) {
  uptime += 2;
}

void sleepIdle(uint8_t seconds) {
  /* function to sleep in current powersave-mode for at least SECONDS
   * this is LOW-RES ! with WDT2S worst case 2secs off!
   */
  uint32_t last = uptime;
  while ((last + seconds) > uptime)
    sleep_cpu();
}

int main(void) {
    //FIXME: this is dumb waste of space.. put into progmem..
    uint8_t reqA[9][3] = {
            { 0x32, 0x36, 0x32 },
            { 0x34, 0x36, 0x34 },
            { 0x38, 0x36, 0x38 },
            { 0x39, 0x36, 0x39 },
            { 0x42, 0x37, 0x32 },
            { 0x43, 0x37, 0x33 },
            { 0x44, 0x37, 0x34 },
            { 0x45, 0x37, 0x35 },
            { 0x46, 0x37, 0x36 },
    };
    mode=OWM_SLEEP;
    wmode=OWW_NO_WRITE;
    OW_DDR&=~OW_PINN;

    INIT_AVR
    PWRSAVE_AVR
    init_eeprom();

    /* RM-restore
    #define UART_BAUD_RATE 9600
    uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );
    */

    INIT_LED_PINS

    SET_OWINT_FALLING;
    DIS_TIMER;
    EN_OWINT;

    sei();
    //force sleep first
    DIS_TIMER;
    EN_OWINT; SET_OWINT_LOWLEVEL;
    sleepmode=SLEEP_MODE_PWR_DOWN;

    //SUART
    suart_init();

    while(1){
        processSerial();
        RMRXLED_PORT &= ~(1<<RMRXLED_PIN); // LED off
        OWRXLED_PORT &= ~(1<<OWRXLED_PIN); // LED off
        sleep_enable();
        set_sleep_mode(sleepmode);
        //sleep_cpu();
        serflag = 1; // should be cleared in processSerial if we receive smthg.
        for (uint8_t j=0;j<7;j++) {
          processSerial();
          uputchar(0x02);uputchar(0x30);
          for (uint8_t k=0;k<3;k++) {
            uputchar(reqA[j][k]);
          }
          uputchar(0x03);
          processSerial();
          RMRXLED_PORT &= ~(1<<RMRXLED_PIN); // LED off
          //avoid delay, get waked by OWINT or WDT anyway, better sleep (idle 2 vs 9 mA on attiny )
          _delay_ms(500); //it takes up to 200ms for the RM to respond!
        }
        if (serflag)
          eflag |= (1<<4);
        else
          eflag &= ~(1<<4);

        processSerial();
        RMRXLED_PORT &= ~(1<<RMRXLED_PIN); // LED off
        OWRXLED_PORT &= ~(1<<OWRXLED_PIN); // LED off
        set_sleep_mode(sleepmode);
        //sleep_cpu();
        //sleepIdle(2);

        // we sleep the cpu anyway and get waked at least every 2s
        //_delay_ms(2000);
        if (ewrite_flag) {
          eeprom_update_dword((uint32_t *) (ewrite_flag+0), eewrite_buf.u32_1);
          ewrite_flag = 0;
        }
        OWRXLED_PORT &= ~(1<<OWRXLED_PIN); // LED off
    }
}
