// --- [ USI_Master.h ] -------------------------------------------------------
//
//       tab = 3 - 24.02.2012

/*****************************************************************************
*
* Atmel Corporation
*
* File              : USI_TWI_Master.h
* Compiler          : WINAVR
* Revision          : $Revision: 1.11 $
* Date              : $Date: Tuesday, September 13, 2005 09:09:36 UTC $
* Updated by        : $Author: jtyssoe $
*
* Support mail      : avr@atmel.com
*
* Supported devices : All device with USI module can be used.
*                     The example is written for the ATmega169, ATtiny26 and ATtiny2313
*
* AppNote           : AVR310 - Using the USI module as a TWI Master
*
* Description       : This is an implementation of an TWI master using
*                     the USI module as basis. The implementation assumes the AVR to
*                     be the only TWI master in the system and can therefore not be
*                     used in a multi-master system.
* Usage             : Initialize the USI module by calling the USI_TWI_Master_Initialise()
*                     function. Hence messages/data are transceived on the bus using
*                     the USI_TWI_Start_Transceiver_With_Data() function. If the transceiver
*                     returns with a fail, then use USI_TWI_Get_Status_Info to evaluate the
*                     course of the failure.
* geändert          : das Timing für den TWI-TAKT wird über _delay_us() geregelt
*
****************************************************************************/
//********** Prototypes **********//

void    USI_MA_Init( void );
uint8_t USI_MA_Write(uint8_t *, uint8_t);
uint8_t USI_MA_Read(uint8_t *, uint8_t);
uint8_t USI_MA_Error_State( void );

// --- General defines --------------------------------------------------------
#define TRUE  1
#define FALSE 0

// --- Defines controlling timing limits --------------------------------------
#define T2_TWI   5		// 5 + 5 us = 10us -> 100.000KHz
#define T4_TWI   5		//

// --- Defines controling code generating - alle zusammen + 60 Byte -----------
// #define PARAM_VERIFICATION       // völlig überflüssig
// #define NOISE_TESTING
// #define SIGNAL_VERIFY

/****************************************************************************
  Bit and byte definitions
****************************************************************************/

#define USI_TWI_NO_DATA             0x00  // Transmission buffer is empty
#define USI_TWI_DATA_OUT_OF_BOUND   0x01  // Transmission buffer is outside SRAM space
#define USI_TWI_UE_START_CON        0x02  // Unexpected Start Condition
#define USI_TWI_UE_STOP_CON         0x03  // Unexpected Stop Condition
#define USI_TWI_UE_DATA_COL         0x04  // Unexpected Data Collision (arbitration)
#define USI_TWI_NO_ACK_ON_DATA      0x05  // The slave did not acknowledge  all data
#define USI_TWI_NO_ACK_ON_ADDRESS   0x06  // The slave did not acknowledge  the address
#define USI_TWI_MISSING_START_CON   0x07  // Generated Start Condition not detected on bus
#define USI_TWI_MISSING_STOP_CON    0x08  // Generated Stop Condition not detected on bus

// --- attiny 25/45/85 --------------------------------------------------------
#define DDR_USI             DDRB
#define PORT_USI            PORTB
#define PIN_USI             PINB
#define PORT_USI_SDA        PORTB0
#define PORT_USI_SCL        PORTB2
#define PIN_USI_SDA         PINB0
#define PIN_USI_SCL         PINB2
#define USI_START_COND_INT  USISIF
#define USI_START_VECTOR    USI_START_vect
#define USI_OVERFLOW_VECTOR USI_OVF_vect

/*
// --- Device dependant defines -----------------------------------------------

#if defined(__AT90Mega169__) | defined(__ATmega169__) |  \
    defined(__AT90Mega165__) | defined(__ATmega165__) |  \
    defined(__ATmega325__)   | defined(__ATmega3250__) | \
    defined(__ATmega645__)   | defined(__ATmega6450__) | \
    defined(__ATmega329__)   | defined(__ATmega3290__) | \
    defined(__ATmega649__)   | defined(__ATmega6490__)
    #define DDR_USI             DDRE
    #define PORT_USI            PORTE
    #define PIN_USI             PINE
    #define PORT_USI_SDA        PORTE5
    #define PORT_USI_SCL        PORTE4
    #define PIN_USI_SDA         PINE5
    #define PIN_USI_SCL         PINE4
#endif

//if defined(__ATtiny25__)   | defined(__ATtiny45__) | defined(__ATtiny85__) |
//    defined(__AT90Tiny26__) | defined(__ATtiny26__)
    #define DDR_USI             DDRB
    #define PORT_USI            PORTB
    #define PIN_USI             PINB
    #define PORT_USI_SDA        PORTB0
    #define PORT_USI_SCL        PORTB2
    #define PIN_USI_SDA         PINB0
    #define PIN_USI_SCL         PINB2
//#endif

#if defined(__AT90Tiny2313__) | defined(__ATtiny2313__)
    #define DDR_USI             DDRB
    #define PORT_USI            PORTB
    #define PIN_USI             PINB
    #define PORT_USI_SDA        PORTB5
    #define PORT_USI_SCL        PORTB7
    #define PIN_USI_SDA         PINB5
    #define PIN_USI_SCL         PINB7
#endif

*/


// --- [ eof ] ----------------------------------------------------------------
