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
 * VERSION 1.3pre2 for ATmega48
 *
 * Created: 15.05.2013 13:36:59
 *  Author: Michael Markstaller
 *
 * use included Makefile: just change target MCU and avrdude params
 *
 * Changelog:
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
#include <avr/pgmspace.h>
#include "uart.h" //FIXME: also exclude in Makefile for attiny!
#include "common.h"

#define DEBUG 1
#include "debug.h"

#ifdef __AVR_ATtiny25__
// OW_PORT Pin 7  - PB2

//OW Pin
#define OW_PORT PORTB //1 Wire Port
#define OW_PIN PINB //1 Wire Pin as number
#define OW_PORTN (1<<PINB2)  //Pin as bit in registers
#define OW_PINN (1<<PINB2)
#define OW_DDR DDRB  //pin direction register
#define SET_LOW OW_DDR|=OW_PINN;OW_PORT&=~OW_PORTN;  //set 1-Wire line to low
#define RESET_LOW {OW_DDR&=~OW_PINN;}  //set 1-Wire pin as input
//Pin interrupt
#define EN_OWINT {GIMSK|=(1<<INT0);GIFR|=(1<<INTF0);}  //enable interrupt
#define DIS_OWINT  GIMSK&=~(1<<INT0);  //disable interrupt
#define SET_RISING MCUCR=(1<<ISC01)|(1<<ISC00);  //set interrupt at rising edge
#define SET_FALLING MCUCR=(1<<ISC01); //set interrupt at falling edge
#define CHK_INT_EN (GIMSK&(1<<INT0))==(1<<INT0) //test if interrupt enabled
#define PIN_INT ISR(INT0_vect)  // the interrupt service routine
//Timer Interrupt
#define EN_TIMER {TIMSK |= (1<<TOIE0); TIFR|=(1<<TOV0);} //enable timer interrupt
#define DIS_TIMER TIMSK  &= ~(1<<TOIE0); // disable timer interrupt
#define TCNT_REG TCNT0  //register of timer-counter
#define TIMER_INT ISR(TIM0_OVF_vect) //the timer interrupt service routine


#define OWT_MIN_RESET 51
#define OWT_RESET_PRESENCE 4
#define OWT_PRESENCE 20
#define OWT_READLINE 3 //3 for fast master, 4 for slow master and long lines
#define OWT_LOWTIME 3 //3 for fast master, 4 for slow master and long lines

//Initializations of AVR
#define INIT_AVR CLKPR=(1<<CLKPCE); \
                   CLKPR=0; /*8Mhz*/  \
                   TIMSK=0; \
                   GIMSK=(1<<INT0);  /*set direct GIMSK register*/ \
                   TCCR0B=(1<<CS00)|(1<<CS01); /*8mhz /64 couse 8 bit Timer interrupt every 8us*/ \
                /* FIXME: disable ADC, Pullups to save power here? */


#define PC_INT_ISR ISR(PCINT0_vect) { /*ATT25 with 0 by PCINT*/ \
                    if (((PINB&(1<<PINB0))==0)&&((istat&(1<<PINB0))==(1<<PINB0))) {    Counter1++;    }        \
                    if (((PINB&(1<<PINB1))==0)&&((istat&(1<<PINB1))==(1<<PINB1))) {    Counter2++;    }        \
                    if (((PINB&(1<<PINB3))==0)&&((istat&(1<<PINB3))==(1<<PINB3))) {    Counter3++;    }        \
                    if (((PINB&(1<<PINB4))==0)&&((istat&(1<<PINB4))==(1<<PINB4))) {    Counter4++;    }        \
                    istat=PINB;}    \

#define INIT_COUNTER_PINS /* Counter Interrupt */\
                        GIMSK|=(1<<PCIE);\
                        PCMSK=(1<<PCINT0)|(1<<PCINT1)|(1<<PCINT3)|(1<<PCINT4);    \
                        DDRB &=~((1<<PINB0)|(1<<PINB1)|(1<<PINB3)|(1<<PINB4)); \
                        istat=PINB;\

#endif // __AVR_ATtiny25__

#if defined(__AVR_ATtiny2313A__) || defined(__AVR_ATtiny2313__)
// OW_PORT Pin 6  - PD2

//OW Pin
#define OW_PORT PORTD //1 Wire Port
#define OW_PIN PIND //1 Wire Pin as number
#define OW_PORTN (1<<PIND2)  //Pin as bit in registers
#define OW_PINN (1<<PIND2)
#define OW_DDR DDRD  //pin direction register
#define SET_LOW OW_DDR|=OW_PINN;OW_PORT&=~OW_PORTN;  //set 1-Wire line to low
#define RESET_LOW {OW_DDR&=~OW_PINN;}  //set 1-Wire pin as input
//Pin interrupt
#define EN_OWINT {GIMSK|=(1<<INT0);EIFR|=(1<<INTF0);}  //enable interrupt
#define DIS_OWINT  GIMSK&=~(1<<INT0);  //disable interrupt
#define SET_RISING MCUCR|=(1<<ISC01)|(1<<ISC00);  //set interrupt at rising edge
#define SET_FALLING {MCUCR|=(1<<ISC01);MCUCR&=~(1<<ISC00);} //set interrupt at falling edge
#define CHK_INT_EN (GIMSK&(1<<INT0))==(1<<INT0) //test if interrupt enabled
#define PIN_INT ISR(INT0_vect)  // the interrupt service routine
//Timer Interrupt
#define EN_TIMER {TIMSK |= (1<<TOIE0); TIFR|=(1<<TOV0);} //enable timer interrupt
#define DIS_TIMER TIMSK  &= ~(1<<TOIE0); // disable timer interrupt
#define TCNT_REG TCNT0  //register of timer-counter
#define TIMER_INT ISR(TIMER0_OVF_vect) //the timer interrupt service routine


#define OWT_MIN_RESET 51
#define OWT_RESET_PRESENCE 4
#define OWT_PRESENCE 20
#define OWT_READLINE 3 //for fast master, 4 for slow master and long lines
#define OWT_LOWTIME 3 //for fast master, 4 for slow master and long lines

//Initializations of AVR
#define INIT_AVR CLKPR=(1<<CLKPCE); \
                   CLKPR=0; /*8Mhz*/  \
                   TIMSK=0; \
                   GIMSK=(1<<INT0);  /*set direct GIMSK register*/ \
                   TCCR0B=(1<<CS00)|(1<<CS01); /*8mhz /64 couse 8 bit Timer interrupt every 8us*/
                /* FIXME: disable ADC to save power here? */

/* disable stuff not needed on this chip here */
#define PWRSAVE_AVR ;

#if defined(__AVR_ATtiny2313__)
#define PC_INT_ISR ISR(PCINT_vect) { /*ATT2313 without 0 by PCINT */ \
                    if (((PINB&(1<<PINB1))==0)&&((istat&(1<<PINB1))==(1<<PINB1))) {    Counter1++;    }        \
                    if (((PINB&(1<<PINB2))==0)&&((istat&(1<<PINB2))==(1<<PINB2))) {    Counter2++;    }        \
                    if (((PINB&(1<<PINB3))==0)&&((istat&(1<<PINB3))==(1<<PINB3))) {    Counter3++;    }        \
                    if (((PINB&(1<<PINB4))==0)&&((istat&(1<<PINB4))==(1<<PINB4))) {    Counter4++;    }        \
                    istat=PINB;}    \

#endif

#if defined(__AVR_ATtiny2313A__)
#define PC_INT_ISR ISR(PCINT_B_vect) { /*attiny2313a is PCINT_B_vect*/ \
                    if (((PINB&(1<<PINB1))==0)&&((istat&(1<<PINB1))==(1<<PINB1))) {    Counter1++;    }        \
                    if (((PINB&(1<<PINB2))==0)&&((istat&(1<<PINB2))==(1<<PINB2))) {    Counter2++;    }        \
                    if (((PINB&(1<<PINB3))==0)&&((istat&(1<<PINB3))==(1<<PINB3))) {    Counter3++;    }        \
                    if (((PINB&(1<<PINB4))==0)&&((istat&(1<<PINB4))==(1<<PINB4))) {    Counter4++;    }        \
                    istat=PINB;}    \

#endif

#define INIT_COUNTER_PINS /* Counter Interrupt */\
                        GIMSK|=(1<<PCIE);\
                        PORTB |= ( (1<<PB1) | (1<<PB2) | (1<<PB3) | (1<<PB4) ); /* activate internal Pull-Up  */ \
                        PCMSK=(1<<PCINT1)|(1<<PCINT2)|(1<<PCINT3)|(1<<PCINT4);    \
                        DDRB &=~((1<<PINB1)|(1<<PINB2)|(1<<PINB3)|(1<<PINB4)); \
                        istat=PINB; \

/* FIXME: !! nonsense ! just to emulate DS18B20 !! */
#define CONV_TEMP   { scratchpad[0]=0x00ff;\
                      scratchpad[1]=0x00ff&(0xEE>>8);\
                        }



#endif // __AVR_ATtiny2313__


#if defined(__AVR_ATmega48__) || (__AVR_ATmega644__)
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
#define EN_OWINT {EIMSK|=(1<<INT0);EIFR|=(1<<INTF0);}  //enable interrupt 0
#define DIS_OWINT  EIMSK&=~(1<<INT0);  //disable interrupt 0
#define SET_RISING EICRA|=(1<<ISC01)|(1<<ISC00);  //set interrupt at rising edge
#define SET_FALLING {EICRA|=(1<<ISC01);EICRA&=~(1<<ISC00);} //set interrupt at falling edge
#define CHK_INT_EN (EIMSK&(1<<INT0))==(1<<INT0) //test if interrupt enabled
#define PIN_INT ISR(INT0_vect)  // the interrupt service routine
//Timer Interrupt
#define EN_TIMER {TIMSK0 |= (1<<TOIE0); TIFR0|=(1<<TOV0);} //enable timer0 interrupt
#define DIS_TIMER TIMSK0  &= ~(1<<TOIE0); // disable timer interrupt
#define TCNT_REG TCNT0  //register of timer-counter
#define TIMER_INT ISR(TIMER0_OVF_vect) //the timer interrupt service routine

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
                  /* TCCR0B=(1<<CS00)|(1<<CS01);  FIXME/CHECK: 4mhz /8 cause 8 bit Timer interrupt every 2us? so we have only 8 ticks! */ \
                  /* MAYBE use 16bit TC1 for this @4MHz?? */ \
                  /* FIXME: init TC1 for PWM/internal time-clock */ \
                  /* FIXED: test wether TC0 can be still used for PWM->YES 490Hz- though it "flickers" for 2x20ms when 1-Wire is active: */ \
                  DDRD |= ((1<<PIND5)|(1<<PIND6)); /* PD5& as output */ \
                  TCCR0A |= ((2<<COM0A0) | (2<<COM0B0) | (3<<WGM00)); /* enable output-compare-match */ \
                  OCR0A = 128; OCR0B = 192; /* set dummy-values 50/75% */ \
                  /* TCNT1 - Test */ \
                  DDRB |= ((1<<PINB1)|(1<<PINB2)); /* PB1&2 as output */ \
                  TIMSK1 |= (1<<TOIE1); TIFR1 |= (1<<TOV1); /* enable overflow int for timer1 */ \
                  TCCR1A |= (1<<COM1A1)|(1<<COM1B1)|(1<<WGM10); /* 8bit Fast-PWM 0<<COM1A0 0<<COM1B0 0<<WGM11 */ \
                  TCCR1B |= (1<<WGM12)|(1<<CS11)|(1<<CS10); /* 0<<WGM13 FastPWM - Clock for TIM1 /1024 ; /256 ohne |(1<<CS10) */ \
                  OCR1A = 128; OCR1B = 192; /* set dummy-values 50/75% */ \

                  /* /124 gives PWM cycle of 32.8ms = 30Hz ; 4885-5190 = 305 * ++ in OVF per 10s */
                  /* /256 gives 125Hz? 8ms = 1222 * ++ in OVF per 10s
                   * every 32us ?

                  /* !! FIXME: must set DDRB PB0/PB1 to output and remove counter below! for PWM to work FIXME !! */
                  // :1024 = every 7812,5 uS?
                  /* Atmega48: PD5/PD6; Attiny2313: PB2/PD5 */ \
                  /* FIXME: disable ADC etc to save power here? */

#define PWRSAVE_AVR PRR = (1<<PRTWI)|(1<<PRTIM2)|(1<<PRSPI)|(1<<PRADC); /* power down TWI, TIMCNT2, leave USART |(1<<PRUSART0) on for debug! */ \
                    /* |(1<<PRTIM1) enabled for now */ \
                    DIDR0 = (1<<ADC5D)|(1<<ADC4D)|(1<<ADC3D)|(1<<ADC2D)|(1<<ADC1D)|(1<<ADC0D); /* diable Digital-in on ADC5..0 = PORTC5..0 */


#define PC_INT_ISR ISR(PCINT0_vect) { /* ATmega48 is PCINT0_vect*/ \
/*                    if (((PINB&(1<<PINB1))==0)&&((istat&(1<<PINB1))==(1<<PINB1))) {    Counter1++;    }      */ \
/*                    if (((PINB&(1<<PINB2))==0)&&((istat&(1<<PINB2))==(1<<PINB2))) {    Counter2++;    }        */ \
                    if (((PINB&(1<<PINB3))==0)&&((istat&(1<<PINB3))==(1<<PINB3))) {    Counter3++;    }        \
                    if (((PINB&(1<<PINB4))==0)&&((istat&(1<<PINB4))==(1<<PINB4))) {    Counter4++;    }        \
                    istat=PINB;}    \
                    /* count on pintoggle */

                /* FIXME: !!! debounce !!! */

#define INIT_COUNTER_PINS /* Init counter pins */ \
                        PCICR=(1<<PCIE0); /* enable PCINT0..7 global */ \
/*                        PORTB |= ( (1<<PB1) | (1<<PB2) ); */ \
                        PORTB |= ( (1<<PB3) | (1<<PB4) ); /* activate internal Pull-Up PB1-4 */ \
/*                        PCMSK0|= ((1<<PCINT1)|(1<<PCINT2)); */ \
                        PCMSK0|= ((1<<PCINT3)|(1<<PCINT4)); /* enable PCINT1-4 PB1-4 */ \
/*                        DDRB &=~((1<<PINB1)|(1<<PINB2)); */ \
                        DDRB &=~((1<<PINB3)|(1<<PINB4)); /* PB1-4 as input */ \
                        istat=PINB; \

/* FIXME: !! nonsense ! just to emulate DS18B20 !! */
#define CONV_TEMP   { scratchpad[0]=0x00ff;\
                        scratchpad[1]=0x00ff&(0x80>>8);\
                        }

#endif // __AVR_ATmega48__

LV_SETUP  //quirk to define itoa-outbuf..

volatile uint32_t uptime = 0; /* holds uptime in 1/4 seconds - overflows after 3.4 years */
volatile uint8_t ovf_flag = 0;


ISR(TIMER1_OVF_vect) {
  if (ovf_flag == 249) { //once every 125*4 calls we have a second
    uptime++; ovf_flag=0;
  }
  ovf_flag++; //TODO: can be made better with bitshifting!

}

//#define _ONE_DEVICE_CMDS_  //Commands for only one device on bus (Not tested)

/* FIXME: !! nonsense ! just to emulate DS2423 !! */
typedef union {
    volatile uint8_t bytes[13];//={1,1,2,0,0,0,0,0,0,0,0,5,5};
    struct {
        uint16_t addr;
        uint8_t read;
        uint32_t counter;
        uint32_t zero;
        uint16_t crc;
    };
} counterpack_t;
counterpack_t counterpack;
volatile uint8_t scratchpad[9]={0x50,0x05,0x0,0x0,0x7f,0xff,0x00,0x10,0x0}; //Initial scratchpad
volatile uint16_t scrc; //CRC calculation

volatile uint8_t lastcps;
volatile uint32_t Counter1;
volatile uint32_t Counter2;
volatile uint32_t Counter3;
volatile uint32_t Counter4;
volatile uint8_t istat;


volatile uint8_t cbuf; //Input buffer for a command
//const uint8_t owid[8]= {0x1D, 0xA2, 0xD9, 0x84, 0x00, 0x00, 0x13, 0xF4};
//uint8_t owid[8] = {0x1D, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01, 0x49 };
uint8_t owid[8] = {0xE1, 0xE1, 0x00, 0x00, 0x00, 0x00, 0x02, 0x20 };
//example-slave uint8_t owid[8] = {0xDD, 0x01, 0x01, 0x02, 0x03, 0x04, 0x06, 0x6C };
//DS2413 - unimplemented! uint8_t owid[8] = {0x3A, 0x01, 0x01, 0x02, 0x03, 0x04, 0x06, 0x60 };
//uint8_t owid[8] = {0xEE, 0x01, 0x01, 0x02, 0x03, 0x04, 0x06, 0x3F };
//uint8_t owid[8] = {0x28, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x9E };

// uint8_t owid[8] = ....? EEMEM
volatile uint8_t bitp;  //pointer to current Byte
volatile uint8_t bytep; //pointer to current Bit

volatile uint8_t mode; //state
volatile uint8_t wmode; //if 0 next bit that send the device is  0
volatile uint8_t actbit; //current
volatile uint8_t srcount; //counter for search rom

//States / Modes
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

#define OWC_GET_VERSION 0x11
#define OWC_GET_TYPE 0x12

/* DS2423:
WRITE_SCRATCHPAD = 0x0F
    + 2 x Target address TA1 TA2 -> CRC16
The memory address range of the DS2423 is 0000H to 01FFH.
READ_SCRATCHPAD = 0xAA
Copy Scratchpad [5AH]
Read Memory [F0H]
Read Memory + Counter [A5H]
*/

/* Missing for DS18B20 - relevant for pullup:
 * DS18B20-DS: In some situations the bus master may not know whether the DS18B20s on the bus are parasite powered
or powered by external supplies. The master needs this information to determine if the strong bus pullup
should be used during temperature conversions. To get this information, the master can issue a Skip ROM
[CCh] command followed by a Read Power Supply [B4h] command followed by a “read time slot”.
During the read time slot, parasite powered DS18B20s will pull the bus low, and externally powered
DS18B20s will let the bus remain high. If the bus is pulled low, the master knows that it must supply the
strong pullup on the 1-Wire bus during temperature conversions.
*/


#ifdef _ONE_DEVICE_CMDS_
#define OWM_READ_ROM 50
#endif

//Write a bit after next falling edge from master
//its for sending a zero as soon as possible
#define OWW_NO_WRITE 2
#define OWW_WRITE_1 1
#define OWW_WRITE_0 0



PIN_INT {
    uint8_t lwmode=wmode;  //let this variables in registers
    uint8_t lmode=mode;
    if ((lwmode==OWW_WRITE_0)) {SET_LOW;lwmode=OWW_NO_WRITE;}    //if necessary set 0-Bit
    DIS_OWINT; //disable interrupt, only in OWM_SLEEP mode it is active
    switch (lmode) {
        case OWM_SLEEP:
            TCNT_REG=~(OWT_MIN_RESET);
            EN_OWINT; //other edges ?
            break;
        //start of reading with falling edge from master, reading closed in timer isr
        case OWM_MATCH_ROM:  //falling edge wait for receive
        case OWM_WRITE_SCRATCHPAD:
        case OWM_GET_ADRESS:
        case OWM_READ_COMMAND:
            TCNT_REG=~(OWT_READLINE); //wait a time for reading
            break;
        case OWM_SEARCH_ROM:   //Search algorithm waiting for receive or send
            if (srcount<2) { //this means bit or complement is writing,
                TCNT_REG=~(OWT_LOWTIME);
            } else
                TCNT_REG=~(OWT_READLINE);  //init for read answer of master
            break;
        case OWC_GET_VERSION:
        case OWC_GET_TYPE:
        case OWM_READ_SCRATCHPAD:  //a bit is sending
            TCNT_REG=~(OWT_LOWTIME);
            break;
#ifdef _ONE_DEVICE_CMDS_
        case OWM_READ_ROM:
#endif
        case OWM_READ_MEMORY_COUNTER: //a bit is sending
            TCNT_REG=~(OWT_LOWTIME);
            break;
        case OWM_CHK_RESET:  //rising edge of reset pulse
            SET_FALLING;
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
    //Interrupt still active ?
    if (CHK_INT_EN) {
        //maybe reset pulse
        if (p==0) {
            lmode=OWM_CHK_RESET;  //wait for rising edge
            SET_RISING;
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
                        /* FIXME: owid from eeprom - takes 4 cyc = 500ns @ 8 */
                        lactbit=(owid[lbytep]&lbitp)==lbitp; //set actual bit
                        lwmode=lactbit;  //prepare for writing when next falling edge
                        break;
                    //FIXME: case 0xEC:  //alarm search rom - TODO
                    case 0x4E: //DS1820
                        lmode=OWM_WRITE_SCRATCHPAD;
                        lbytep=2;scratchpad[2]=0;  //initialize writing position in scratch pad
                        break;
                    case 0x44:  //DS1820 Start Convert
                    case 0x64:  // some tool uses this command
                        CONV_TEMP;
                        lmode=OWM_SLEEP;
                        break;
                    case 0xBE: //DS1820
                        lmode=OWM_READ_SCRATCHPAD; //read scratch pad
                        lbytep=0;lscrc=0; //from first position
                        lactbit=(lbitp&scratchpad[0])==lbitp;
                        lwmode=lactbit; //prepare for send firs bit
                        break;
                    case 0xA5: // DS2423 Read Memory + Counter [A5H]
                        lmode=OWM_GET_ADRESS; //first the master send an address
                        lbytep=0;lscrc=0x7bc0; //CRC16 of 0xA5
                        counterpack.bytes[0]=0;
                        break;
#ifdef _ONE_DEVICE_CMDS_
                    case 0xCC: //skip ROM
                        lbytep=0;cbuf=0;lmode=OWM_READ_COMMAND;break;
                    case 0x33: //read ROM
                        lmode=OWM_READ_ROM;
                        lbytep=0;
                        break;
#endif
                    case 0x11: //E1 CUSTOM get version
                        lmode=OWC_GET_VERSION;
                        lbytep=0;
                        break;
                    case 0x12: //E1 CUSTOM get type
                        lmode=OWC_GET_TYPE;
                        lbytep=0;
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
                        /* FIXME: owid from eeprom - takes 4 cyc = 500ns @ 8 */
                        lactbit=(owid[lbytep]&lbitp)==lbitp;
                        lwmode=lactbit;
                    }
                    break;
            }
            break;
        case OWM_MATCH_ROM:
            /* FIXME: owid from eeprom - takes 4 cyc = 500ns @ 8 */
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
        case OWM_WRITE_SCRATCHPAD:
            if (p) {
                scratchpad[lbytep]|=lbitp;
            }
            lbitp=(lbitp<<1);
            if (!lbitp) {
                lbytep++;
                lbitp=1;
                if (lbytep==5) {
                    lmode=OWM_SLEEP;
                    break;
                } else scratchpad[lbytep]=0;
            }
            break;
        case OWM_READ_SCRATCHPAD:
            RESET_LOW;
            if ((lscrc&1)!=lactbit) lscrc=(lscrc>>1)^0x8c; else lscrc >>=1;
            lbitp=(lbitp<<1);
            if (!lbitp) {
                lbytep++;
                lbitp=1;
                if (lbytep>=9) {
                    lmode=OWM_SLEEP;
                    break;
                } else if (lbytep==8) scratchpad[8]=lscrc;
            }
            lactbit=(lbitp&scratchpad[lbytep])==lbitp;
            lwmode=lactbit;
            break;
        case OWM_GET_ADRESS:
            //FIXME: copy value to local varible/"scratchpad"?? it might change during read!
            if (p) { //Get the Address for reading
                counterpack.bytes[lbytep]|=lbitp;
            }
            //address is part of crc
            if ((lscrc&1)!=p) lscrc=(lscrc>>1)^0xA001; else lscrc >>=1;
            lbitp=(lbitp<<1);
            if (!lbitp) {
                lbytep++;
                lbitp=1;
                if (lbytep==2) {
                    lmode=OWM_READ_MEMORY_COUNTER;
                    lactbit=(lbitp&counterpack.bytes[lbytep])==lbitp;
                    lwmode=lactbit;
                    lsrcount=(counterpack.addr&0xfe0)+0x20-counterpack.addr;
                    //bytes between start and Counter Values, Iam never understanding why so much???
                    break;
                } else counterpack.bytes[lbytep]=0;
            }
            break;
        case OWM_READ_MEMORY_COUNTER:
            RESET_LOW;
            //CRC16 Calculation
            if ((lscrc&1)!=lactbit)
              lscrc=(lscrc>>1)^0xA001;
            else
              lscrc >>=1;
            p=lactbit;
            lbitp=(lbitp<<1);
            if (!lbitp) {
                lbytep++;
                lbitp=1;
                if (lbytep==3) {
                    lsrcount--;
                    if (lsrcount) lbytep--;
                    else  {//now copy counter in send buffer
                        switch (counterpack.addr&0xFe0) {
                        case 0x180:
                            counterpack.counter=Counter1;
                        break;
                        case 0x1A0:
                            counterpack.counter=Counter2;
                            break;
                        case 0x1C0:
                            counterpack.counter=Counter3;
                            break;
                        case 0x1E0:
                            counterpack.counter=Counter4;
                            break;
                        default: counterpack.counter=0;
                        }
                    }
                }
                if (lbytep>=13) { //done sending
                    lmode=OWM_SLEEP;
                    break;
                }
                if ((lbytep==11)&&(lbitp==1)) { //Send CRC
                    counterpack.crc=~lscrc;
                }

            }
            lactbit=(lbitp&counterpack.bytes[lbytep])==lbitp;
            lwmode=lactbit;
            break;
        case OWC_GET_VERSION:
            RESET_LOW;
            lbitp=(lbitp<<1);
            if (!lbitp) {
                lbytep++;
                lbitp=1;
                if (lbytep>=2) {
                    lmode=OWM_SLEEP;
                    break;
                }
            }
            //lactbit=(lbitp&owid[lbytep])==lbitp;
            lactbit=(lbitp& 0x89)==lbitp;
            lwmode=lactbit;
            break;
        case OWC_GET_TYPE:
            RESET_LOW;
            lbitp=(lbitp<<1);
            if (!lbitp) {
                lbytep++;
                lbitp=1;
                if (lbytep>=1) {
                    lmode=OWM_SLEEP;
                    break;
                }
            }
            lactbit=(lbitp& 0x88)==lbitp;
            lwmode=lactbit;
            break;

#ifdef _ONE_DEVICE_CMDS_
        case OWM_READ_ROM:
            RESET_LOW;
            lbitp=(lbitp<<1);
            if (!lbitp) {
                lbytep++;
                lbitp=1;
                if (lbytep>=8) {
                    lmode=OWM_SLEEP;
                    break;
                }
            }
            lactbit=(lbitp&owid[lbytep])==lbitp;
            lwmode=lactbit;
            break;
#endif
        }
        if (lmode==OWM_SLEEP) {DIS_TIMER;}
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



PC_INT_ISR  //for counting  defined for specific device

void init_eeprom(void) {
    /* check magic, read slave address and counter values, resetcount, init-name, */
    if (eeprom_read_word(EE_MAGIC_OFFSET) == EE_MAGIC_NUMBER) {
      //EEPROM valid -> read counters & settings
      for (uint16_t i=EE_OWID_OFFSET;i<EE_OWID_OFFSET+8;i++)
        owid[i-EE_OWID_OFFSET] = eeprom_read_byte((uint8_t *) i);
      eeprom_update_word(EE_RCNT_OFFSET, eeprom_read_word(EE_RCNT_OFFSET) + 1);
      Counter1 = eeprom_read_dword(EE_COUNTER_OFFSET);
      Counter2 = eeprom_read_dword(EE_COUNTER_OFFSET+4);
      Counter3 = eeprom_read_dword(EE_COUNTER_OFFSET+8);
      Counter4 = eeprom_read_dword(EE_COUNTER_OFFSET+16);
    } else {
      //Init values
      /* should cli(); here no sei(); yet enabled in main.. */
      eeprom_write_word(EE_MAGIC_OFFSET, EE_MAGIC_NUMBER);
      for (uint16_t i=EE_OWID_OFFSET;i<EE_OWID_OFFSET+8;i++)
        eeprom_write_byte((uint8_t *) i,owid[i-EE_OWID_OFFSET]);
      eeprom_write_word(EE_RCNT_OFFSET, 1);
      eeprom_write_dword(EE_COUNTER_OFFSET,0);
      eeprom_write_dword(EE_COUNTER_OFFSET+4,0);
      eeprom_write_dword(EE_COUNTER_OFFSET+8,0);
      eeprom_write_dword(EE_COUNTER_OFFSET+12,0);
      eeprom_write_byte((uint8_t *) EE_TYPE_OFFSET, EE_DEFTYPE);
      eeprom_write_word(EE_VERSION_OFFSET, 0x0102);
    }
}

int main(void) {
    mode=OWM_SLEEP;
    wmode=OWW_NO_WRITE;
    OW_DDR&=~OW_PINN;

    for(uint8_t i=0;i<sizeof(counterpack);i++) counterpack.bytes[i]=0;

    SET_FALLING;

    INIT_AVR
    PWRSAVE_AVR
    DEBUG_INIT
    LSL("Startup")

    init_eeprom();

    DIS_TIMER;
    /* FIXME: read slave-id from eeprom here? */

    INIT_COUNTER_PINS

    sleep_enable();
    sei();
    set_sleep_mode(SLEEP_MODE_IDLE);
    //FIXME: somehow int0 doesn't wake up! maybe SET_FAILLING is missing or so.. PWR_SAVE should work
    //or set/change sleep_mode in OWINT?
    uint32_t tlast;
    while(1){
        /* FIXME: Idle / sleep here? */
        sleep_cpu();

        /* some int debug
        LL
        LSL("Mode: ") LV(mode) LL
        LSL("INT0: ") LV(EIMSK&(1<<INT0)) LL
        LSL("INT0: ") LV(EICRA&(1<<ISC00)) LL
        LSL("INT0: ") LV(EICRA&(1<<ISC01)) LL
        LSL("Timer: ") LV((TIMSK0&(1<<TOIE0))) LL
        */

        DLY(1000)
        LV(uptime)
        LSL("\r\n");
    }
}

