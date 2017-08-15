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
 */


#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <string.h>
#include <avr/wdt.h>
#include "common.h"
#include "uart.h"

#define DEBUG 0
#include "debug.h"
#ifndef F_CPU
//# warning "F_CPU was not defined!! defining it now in debug.h but you should take care before!"
#define F_CPU 8000000UL //very important! define before delay.h as delay.h fucks up the serial-timing otherwise somehow
#endif
#include <util/delay.h>


#if defined(__AVR_ATmega48__) || (__AVR_ATmega88__) || (__AVR_ATmega328P__)
// OW_PORT Pin 4  - PD2(INT0)

//OW Pin
#define OW_PORT PORTD //1 Wire Port
#define OW_PIN PIND //1 Wire Pin as number
#define OW_PORTN (1<<PIND2)  //Pin as bit in registers
#define OW_PINN (1<<PIND2)
#define OW_DDR DDRD  //pin direction register
#define SET_LOW OW_DDR|=OW_PINN;OW_PORT&=~OW_PORTN;  //set 1-Wire line to low
#define RESET_LOW {OW_DDR&=~OW_PINN;}  //set 1-Wire pin as input
//Pin interrupt
#define EN_OWINT {EIMSK|=(1<<INT0);EIFR=(1<<INTF0);}  //enable interrupt 0 and *clear* it - no OR!
#define DIS_OWINT  EIMSK&=~(1<<INT0); sleepmode=SLEEP_MODE_IDLE; //disable interrupt 0
#define SET_OWINT_RISING EICRA|=(1<<ISC01)|(1<<ISC00);  //set interrupt at rising edge
#define SET_OWINT_FALLING {EICRA|=(1<<ISC01);EICRA&=~(1<<ISC00);} //set interrupt at falling edge
#define SET_OWINT_BOTH EICRA=(1<<ISC00);EICRA&=~(1<<ISC01); //set interrupt at both edges
#define SET_OWINT_LOWLEVEL EICRA&=~((1<<ISC01)|(1<<ISC00)); //set interrupt at low level
#define CHK_INT_EN (EIMSK&(1<<INT0))==(1<<INT0) //test if interrupt enabled
#define PIN_INT ISR(INT0_vect)  // the interrupt service routine
//Timer Interrupt
#define EN_TIMER {TIMSK0 |= (1<<TOIE0); TIFR0|=(1<<TOV0);} //enable timer0 interrupt
#define DIS_TIMER TIMSK0  &= ~(1<<TOIE0); // disable timer interrupt
#define TCNT_REG TCNT0  //register of timer-counter
#define TIMER_INT ISR(TIMER0_OVF_vect) //the timer interrupt service routine

/* TODO/FIXME: define ports for status-leds, etc
 * AVOID:
 * PB2-5: SPI
 * PC4-5: TWI
 * PC0-3: ADC0-3
 * PD5-6: PWM0 (?TC in use!)
 * PB1-2: PWM1
 * PB3/PD3: PWM2
 * PD2-3: INT0-1
 * PD0-1: USART
 * Good:
 * PD4
 * PB6-7 (XTAL)
 * PD7
 * PB0
 * PB1-5 (used by counter, collides PWM0,SPI)
 */
#define RMRXLED_PORT PORTD
#define RMRXLED_DDR DDRD
#define RMRXLED_PIN PIND4

#define OWRXLED_PORT PORTD
#define OWRXLED_DDR DDRD
#define OWRXLED_PIN PIND7

#define INIT_LED_PINS RMRXLED_DDR |= (1<<RMRXLED_PIN); \
                     OWRXLED_DDR |= (1<<OWRXLED_PIN); /* pins as output */

//FIXME / TODO: double-check & compare timings!
#define OWT_MIN_RESET 51 // tRSTL 512uS in DS, tRSTH = 584; 51 are 408uS
#define OWT_RESET_PRESENCE 4 //tPDT 64uS in DS
#define OWT_PRESENCE 20 // unclear, mabe between 512 and 584 uS; 20 are 160uS? real bus 9/170uS -> default settings 31/200
#define OWT_READLINE 3 //for fast master, maybe 4 for long lines; flexible is 10 to 24 uS
#define OWT_LOWTIME 3 //3=24uS for DS9490,

//Initializations of AVR
#define INIT_AVR CLKPR=(1<<CLKPCE); /* FIXME! this is crap, next line disables it! */ \
                  CLKPR=0; /* 8Mhz */ \
                  /* CLKPR=(1<<CLKPCE)|(0<<CLKPS3)|(0<<CLKPS2)|(0<<CLKPS1)|(1<<CLKPS0);  FIXME/CHECK: this gives 4MHz */ \
                  TIMSK0=0; \
                  EIMSK=(1<<INT0);  /*set direct GICR INT0 enable*/ \
                  TCCR0B=(1<<CS00)|(1<<CS01); /*8mhz /64 cause 8 bit Timer interrupt every 8us */ \
                  WDTCSR |= ((1<<WDCE)|(1<<WDP2)|(1<<WDP1)|(1<<WDP0)); /* ((1<<WDP2)|(1<<WDP1)|(1<<WDP0)) WDT_ISR every 2s - (1<<WDP3) every 4s */ \
                  WDTCSR |= (1<<WDIE); /* only enable int, no real watchdog */

#define PWRSAVE_AVR

//PRR = (1<<PRTWI)|(1<<PRTIM2)|(1<<PRSPI)|(1<<PRTIM1)|(1<<PRADC); /* power down TWI, TIMCNT2, leave USART |(1<<PRUSART0) on for debug! */ \
//                    DIDR0 = (1<<ADC5D)|(1<<ADC4D)|(1<<ADC3D)|(1<<ADC2D)|(1<<ADC1D)|(1<<ADC0D); /* diable Digital-in on ADC5..0 = PORTC5..0 */

#endif // __AVR_ATmega48__


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
uint8_t owid[8] = {0xE1, 0xE1, 0x00, 0x00, 0x00, 0x03, 0x28, 0x28 };

volatile uint8_t bitp;  //pointer to current Byte
volatile uint8_t bytep; //pointer to current Bit

volatile uint8_t mode; //state
volatile uint8_t wmode; //if 0 next bit that send the device is  0
volatile uint8_t actbit; //current
volatile uint8_t srcount; //counter for search rom

/* temp vars to avoid eeprom-reading - in case of low-mem: FIXME */
uint16_t version = 0x0104;
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
                                uart_putc(req2[i]);
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
          sleepmode=SLEEP_MODE_PWR_DOWN; //sleep deep
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
    uint16_t cbuf;
    char recv_buf[16];
    uint8_t loop = 1;
    uint8_t bpos = 0;
    uint16_t chksum = 0;

    while (loop) {
      cbuf = uart_getc();
      if ( cbuf & UART_NO_DATA ) {
        loop = 0;
      } else { /*have data*/
          RMRXLED_PORT ^= (1<<RMRXLED_PIN); /* toggle led */
          switch (cbuf & 0xFF) {
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
              uart_putc(0x06); // send ACK
              break;
            case 0x06:
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
              recv_buf[bpos] = cbuf & 0xFF;
              chksum += cbuf & 0xFF;
              if (bpos < sizeof (recv_buf))
                bpos++;
              break;
          }
              _delay_ms(1); /* FIXME: sleep a little.. */
      }
    }

    //quirk, substract last two bytes
    chksum -= recv_buf[bpos-2] + recv_buf[bpos-1];
    chksum &= 0xFF;

    uint8_t chksum_recv = hex2dec(recv_buf[bpos-2])*16 + hex2dec(recv_buf[bpos-1]);
    if (chksum != chksum_recv && bpos > 0)
      eflag |= (1<<5);
    else if (chksum == chksum_recv && bpos > 0)
      eflag &= ~(1<<5);

    /*
    uint32_t tmpl;
    uint16_t tmpi;
    uint8_t  tmpb;
    float tmpf;
    */
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

int main(void) {
    cli();
    wdt_enable(WDTO_2S);
    wdt_reset();
    wdt_disable();
    WDTCSR = 0;
    WDTCSR |= ((1<<WDCE)|(1<<WDP3)); /* ((1<<WDP2)|(1<<WDP1)|(1<<WDP0)) WDT_ISR every 2s - (1<<WDP3) every 4s */ \
    WDTCSR |= (1<<WDIE); /* only enable int, no real watchdog */
    /* WDTIE doesnt really work on atmega, while well on attiny84! its much too fast.. */

    mode=OWM_SLEEP;
    wmode=OWW_NO_WRITE;
    OW_DDR&=~OW_PINN;

    INIT_AVR
    PWRSAVE_AVR
    init_eeprom();

#define UART_BAUD_RATE 9600
    uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );

    INIT_LED_PINS

    SET_OWINT_FALLING;
    DIS_TIMER;
    EN_OWINT;

    sei();
    //force sleep first
    DIS_TIMER;
    EN_OWINT; SET_OWINT_LOWLEVEL;
    sleepmode=SLEEP_MODE_PWR_DOWN;

    while(1){
        processSerial();
        RMRXLED_PORT &= ~(1<<RMRXLED_PIN); /* led off */
        sleep_enable();
        set_sleep_mode(sleepmode);
        sleep_cpu();
        //FIXME: this is dumb waste of space..
        uint8_t reqA[9][3] = {
//was:                              { 0x02, 0x30, 0x34, 0x36, 0x34, 0x03 },
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
        serflag = 1; // should be cleared in processSerial if we receive smthg.
        for (uint8_t j=0;j<7;j++) {
        //uint8_t req2[] = { 0x02, 0x30, 0x37, 0x30, 0x30, 0x30, 0x38, 0x32, 0x46, 0x03 };
        /* Send queries: 2,4,8,9,B,C,D,E,F */
          uart_putc(0x02);uart_putc(0x30);
          for (uint8_t k=0;k<3;k++) {
            uart_putc(reqA[j][k]);
          }
          uart_putc(0x03);
          processSerial();
          RMRXLED_PORT &= ~(1<<RMRXLED_PIN); /* led off */
          _delay_ms(500);
        }
        if (serflag)
          eflag |= (1<<4);
        else
          eflag &= ~(1<<4);

        processSerial();
        RMRXLED_PORT &= ~(1<<RMRXLED_PIN); /* led off */
        OWRXLED_PORT &= ~(1<<OWRXLED_PIN); /* led off */
        set_sleep_mode(sleepmode);
        sleep_cpu();

        // we sleep the cpu anyway and get waked at least every 2s
        _delay_ms(10000);

        if (ewrite_flag) {
          eeprom_update_dword((uint32_t *) (ewrite_flag+0), eewrite_buf.u32_1);
          ewrite_flag = 0;
        }
        OWRXLED_PORT &= ~(1<<OWRXLED_PIN); /* led off */
    }
}
