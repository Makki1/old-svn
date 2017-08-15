/*
 * based on owdevice - A small 1-Wire emulator for AVR Microcontroller
 * Copyright (C) 2012  Tobias Mueller mail (at) tobynet.de

 * OWSlave für I2C/TWI with BMP085/BMP180 pressure sensor
 * - based on owslave 1.52/1.06 4count (C) 2013 mm@elabnet.de
 *
 * I2C/TWI Master is rewritten from scratch as all available libs use a very dumb thing: delay.h..
 * For many reasons, mainly powersaving, using dumb delay.h burning CPU-cycles isn't allowed in my world..
 * Atmel AN: AVR310, AVR311, AVR312, AVR315 (which isn't avr-gcc compatible but for IAR..)
 * maybe3: http://www.mikrocontroller.net/topic/249639#2564952
 * (still with delay..) http://www.cs.cmu.edu/~dst/ARTSI/Create/PC%20Comm/USI_TWI_Master.c (http://www.cs.cmu.edu/~dst/ARTSI/Create/PC%20Comm/)
 * https://github.com/CalcProgrammer1/Stepper-Motor-Controller/tree/master/UnipolarStepperDriver
 * Credits still for many sources from mikrontroller.net, Peter Fleury, Jörg Wunsch and many more!
 *
 * Created: 2013-12-11, Author: Michael Markstaller
 *
 * use included Makefile: just change target MCU and avrdude params
 *
 * Notes:
 * - OWRXLED is connected with the ANODE to PAx
 *
 * Changelog:
 * v1.52(->1.06) 2013-12-11 first port from owslave_4count
 * - Added i2c-master
 * - purged ATMega for now
 *
 * Pending:
 * - min/max/history/pres-change/altitude/MSL-compensation?
 * - maybe extend things to supply a complete waether-station with Temp, Hum, Wind, ..?
 * - maybe some NMEA-output directly? GPS?
 *
 */


#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <string.h>
// #include <avr/pgmspace.h>
#include "common.h"


//#define DEBUG 0
#include "debug.h"
#ifndef F_CPU
//# warning "F_CPU was not defined!! defining it now in debug.h but you should take care before!"
#define F_CPU 8000000UL //very important! define before delay.h as delay.h fucks up the serial-timing otherwise somehow
#endif
//#include <util/delay.h>

#if defined (__AVR_ATtiny24__) || (__AVR_ATtiny44__) || (__AVR_ATtiny84__)
// OW_PORT Pin 5 - PB2

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

#define OWRXLED_PORT PORTA
#define OWRXLED_DDR DDRA
#define OWRXLED_PIN PINA2

#define INIT_LED_PINS OWRXLED_DDR |= (1<<OWRXLED_PIN); /* pins as output */

//FIXME / TODO: Double-check timings and move these to EEPROM
#define OWT_MIN_RESET 51
#define OWT_RESET_PRESENCE 4
#define OWT_PRESENCE 20
#define OWT_READLINE 2 //3 for fast master, 4 for slow master and long lines
#define OWT_LOWTIME 3 //3 for fast master, 4 for slow master and long lines

//Initializations of AVR
#define INIT_AVR CLKPR=(1<<CLKPCE); \
                   CLKPR=0; /*8Mhz*/  \
                   TIMSK0=0; \
                   GIMSK=(1<<INT0);  /*set direct GIMSK register*/ \
                   TCCR0B=(1<<CS00)|(1<<CS01); /*8mhz /64 couse 8 bit Timer interrupt every 8us*/ \
                   WDTCSR |= ((1<<WDP2)|(1<<WDP1)|(1<<WDP0)); /* ((1<<WDP2)|(1<<WDP1)|(1<<WDP0)) WDT_ISR every 2s - (1<<WDP3) every 4s */ \
                   WDTCSR |= (1<<WDIE); /* only enable int, no real watchdog */

#define PWRSAVE_AVR ADCSRA &= ~(1<<ADEN); PRR |= (1<<PRTIM1)|(1<<PRUSI)|(1<<PRADC); //FIXME: we need USI for TWI?

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
    struct {
      uint8_t   page4;
      uint8_t   u8_41;
      uint8_t   u8_42;
      uint8_t   u8_43;
      uint8_t   u8_44;
      uint32_t   u32_4XX;
      uint8_t   crc4;
    };
} scratchpad_t;
scratchpad_t scratchpad;

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
volatile uint32_t pressure;
volatile uint32_t temperature;
-> struct bmp085_calib_data _bmp085_coeffs

volatile uint8_t sleepmode;

volatile uint8_t cbuf; //Input buffer for a command
uint8_t owid[8] = {0xE1, 0xE2, 0x00, 0x00, 0x00, 0x06, 0x84, 0x82 };

volatile uint8_t bitp;  //pointer to current Byte
volatile uint8_t bytep; //pointer to current Bit

volatile uint8_t mode; //state
volatile uint8_t wmode; //if 0 next bit that send the device is  0
volatile uint8_t actbit; //current
volatile uint8_t srcount; //counter for search rom

/* temp vars to avoid eeprom-reading - in case of low-mem: FIXME */
uint16_t version = 0x0106;
uint8_t stype = 6;
uint16_t rcnt = 1;
uint8_t eflag; //internal error/status-flag
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
                        scratchpad.u32_31 = temperature;
                        scratchpad.u32_32 = pressure;
                        break;
                      case 3:
                        scratchpad.page1 = page;
                        //?
                        break;
                      case 4:
                        scratchpad.page1 = page;
                        /* Calibration data:
                          int16_t  ac1;
                          int16_t  ac2;
                          int16_t  ac3;
                          uint16_t ac4;
                          uint16_t ac5;
                          uint16_t ac6;
                          int16_t  b1;
                          int16_t  b2;
                          int16_t  mb;
                          int16_t  mc;
                          int16_t  md;
                        */
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

#ifdef PC_INT_ISR
PC_INT_ISR  //for counting  defined for specific device
#elif defined PC_INT_ISRA
PC_INT_ISRA
PC_INT_ISRB
#endif

void init_eeprom(void) {
    /* check magic, read slave address and counter values, resetcount, init-name, */
    if (eeprom_read_word((const uint16_t *) (EE_MAGIC_OFFSET+0)) == EE_MAGIC_NUMBER) {
      //EEPROM valid -> read counters & settings
      for (uint16_t i=EE_OWID_OFFSET;i<EE_OWID_OFFSET+8;i++)
        owid[i-EE_OWID_OFFSET] = eeprom_read_byte((uint8_t *) i);
      rcnt = eeprom_read_word((const uint16_t *) (EE_RCNT_OFFSET+0)) + 1;
      eeprom_update_word((uint16_t *) (EE_RCNT_OFFSET+0), rcnt);
      version = eeprom_read_word((const uint16_t *) (EE_VERSION_OFFSET+0));
      stype = eeprom_read_byte((uint8_t *) (EE_TYPE_OFFSET+0));
    } else {
      //Init values
      /* should cli(); here no sei(); yet enabled in main.. */
      eeprom_write_word((uint16_t *) (EE_MAGIC_OFFSET+0), EE_MAGIC_NUMBER);
      for (uint16_t i=EE_OWID_OFFSET;i<EE_OWID_OFFSET+8;i++)
        eeprom_write_byte((uint8_t *) i,owid[i-EE_OWID_OFFSET]);
      eeprom_write_word((uint16_t *) (EE_RCNT_OFFSET+0), rcnt);
      eeprom_write_byte((uint8_t *) (EE_TYPE_OFFSET+0), stype);
      eeprom_write_word((uint16_t *) (EE_VERSION_OFFSET+0), version);
    }
}

//FIXME: enable real watchdog?
ISR(WDT_vect) {
  uptime += 2;
}

int main(void) {
    mode=OWM_SLEEP;
    wmode=OWW_NO_WRITE;
    OW_DDR&=~OW_PINN;

    INIT_AVR
    PWRSAVE_AVR

    init_eeprom();

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
        if (ewrite_flag) {
          eeprom_update_dword((uint32_t *) (ewrite_flag+0), eewrite_buf.u32_1);
          ewrite_flag = 0;
        }
        OWRXLED_PORT &= ~(1<<OWRXLED_PIN); /* led off */
        sleep_enable();
        set_sleep_mode(sleepmode);
        sleep_cpu();
    }
}
