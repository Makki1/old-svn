// --- [ USI_Master.c ] -------------------------------------------------------
//
//       tab = 3 - 24.02.2012
//

/*****************************************************************************
*
* Atmel Corporation
*
* File              : USI_TWI_Master.c
* Compiler          : WINAVR
* Revision          :
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
*                     the USI_TWI_Transceive() function. The transceive function
*                     returns a status byte, which can be used to evaluate the
*                     success of the transmission.

* Änderung          : Union für errorState, addressMode und masterWriteDataMode aufgelöst zu
*                     eigenständigen Variablen, Variablen 1Bit/8Bit Mode zu Konstanten gemacht
*                     32 Byte Programmspeicher gespart, 2 Byte mehr Daten.
*
*                     Im Gegensatz zu allen anderen TWI-Routinen verwendet usi_twi_master
*                     keinen eigenen Variablensatz für die Daten (Übergabe via Referenz)
*                     Die Variablen müssen im aufrufenden Programm angelegt werden
****************************************************************************/
#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>
#include "USI_Master.h"

#define USISR_8bit  (1<<USISIF)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)|(0x0<<USICNT0)
#define USIST_1bit  (1<<USISIF)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)|(0xE<<USICNT0)

uint8_t USI_TWI_Start_Transceiver_With_Data( uint8_t * , uint8_t);
uint8_t USI_TWI_Master_Transfer( uint8_t );
uint8_t USI_TWI_Master_Stop( void );

uint8_t errorState          = 0;				// Fehlermeldung
uint8_t addressMode         = 0;				// Flag für die Behandlung des 1.Byte (TWI-Adresse)
uint8_t masterWriteDataMode = 0;				// Flag für Lesen/Schreiben - abhängig von Bit.0 der Adresse
														// könnte man wegoptimieren, indem man die TWI-Adresse befragt

// ----------------------------------------------------------------------------
void USI_MA_Init( void )
{
	PORT_USI |= (1<<PIN_USI_SDA);           				// Enable pullup on SDA
	PORT_USI |= (1<<PIN_USI_SCL);           				// Enable pullup on SCL

	DDR_USI  |= (1<<PIN_USI_SCL);           				// Enable SCL as output
	DDR_USI  |= (1<<PIN_USI_SDA);           				// Enable SDA as output

	USIDR =  0xFF;                       					// Preload dataregister with "released level" data.
	USICR =  (0<<USISIE)|(0<<USIOIE)|                	// Disable Interrupts.
            (1<<USIWM1)|(0<<USIWM0)|                	// Set USI in Two-wire mode.
            (1<<USICS1)|(0<<USICS0)|(1<<USICLK)|    	// Software clock strobe as counter clock source
            (0<<USITC);
	USISR	=  (1<<USISIF)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)|// Clear flags,
            (0x0<<USICNT0);                         	// and reset counter.
}

// ----------------------------------------------------------------------------
// Löscht nur das Read-Bit und reicht die Daten an USI_TWI_Start_Transceiver_With_Data() durch
uint8_t USI_MA_Write(uint8_t * data, uint8_t n)
{
   data[0] &= 0xFE;
   return( USI_TWI_Start_Transceiver_With_Data( data, n));
}

// ----------------------------------------------------------------------------
// Setzt nur das Read-Bit und reicht die Daten an USI_TWI_Start_Transceiver_With_Data() durch
uint8_t USI_MA_Read(uint8_t * data, uint8_t n)
{
   data[0] |= 0x01;
   return( USI_TWI_Start_Transceiver_With_Data( data, n));
}

// ----------------------------------------------------------------------------
// Use this function to get hold of the error message from the last transmission
uint8_t USI_MA_Error_State( void )
{
	return ( errorState );                 				// Return error state.
}

/*---------------------------------------------------------------
 USI Transmit and receive function. LSB of first byte in data
 indicates if a read or write cycles is performed.
 If set a read operation is performed.

 Function generates (Repeated) Start Condition, sends address and
 R/W, Reads/Writes Data, and verifies/sends ACK.

 Success or error code is returned. Error codes are defined in USI_TWI_Master.h
---------------------------------------------------------------*/
uint8_t USI_TWI_Start_Transceiver_With_Data( uint8_t *msg, uint8_t msgSize)
{
	errorState = 0;												// Error zurücksetzen
	addressMode = TRUE;											// das erste Byte ist immer die TWI-Adresse
	masterWriteDataMode = TRUE;								// Master schreibt auf den TWI-Slave

#ifdef PARAM_VERIFICATION
	if(msg > (uint8_t*) RAMEND)        		         	// Test if address is outside SRAM space
		{
		errorState = USI_TWI_DATA_OUT_OF_BOUND;
		return (FALSE);
		}
	if(msgSize <= 1)                                 	// Test if the transmission buffer is empty
		{
		errorState = USI_TWI_NO_DATA;
		return (FALSE);
		}
#endif

#ifdef NOISE_TESTING                                	// Test if any unexpected conditions have arrived prior to this execution.
	if( USISR & (1<<USISIF) )									// wenn ein USI-Start erkannt wurde
		{
		errorState = USI_TWI_UE_START_CON;
		return (FALSE);
		}
	if( USISR & (1<<USIPF) )									// wenn ein USI-Stop erkannt wurde
		{
		errorState = USI_TWI_UE_STOP_CON;
		return (FALSE);
		}
	if( USISR & (1<<USIDC) )									// wenn ein Data Output Collision erkannt wurde
		{
		errorState = USI_TWI_UE_DATA_COL;
		return (FALSE);
		}
#endif

// The LSB in the address byte determines if is a masterRead or masterWrite operation.
	if ( *msg & 0x01 )                						// wenn Bit.0 gesetzt ist
		{
		masterWriteDataMode = FALSE;							// Flag: Master_liest vom Slave
		}

// Release SCL to ensure that (repeated) Start can be performed
	PORT_USI |= (1<<PIN_USI_SCL);                   	// Release SCL.
	while( !(PORT_USI & (1<<PIN_USI_SCL)) );        	// Verify that SCL becomes high.

	_delay_us( T4_TWI );                         		// Delay

// Generate Start Condition ---------------------------------------------------
	PORT_USI &= ~(1<<PIN_USI_SDA);                  	// Force SDA LOW.
	_delay_us( T4_TWI );											//
	PORT_USI &= ~(1<<PIN_USI_SCL);                  	// Pull SCL LOW.
	PORT_USI |= (1<<PIN_USI_SDA);                   	// Release SDA.

#ifdef SIGNAL_VERIFY
	if( !(USISR & (1<<USISIF)) )								// wenn kein USI-Start erkannt wurde, dann Fehler
		{
		errorState = USI_TWI_MISSING_START_CON;
		return (FALSE);
		}
#endif

// Write address and Read/Write data ------------------------------------------
	do
		{
		// If masterWrite cycle (or inital address transmission)
		if (addressMode || masterWriteDataMode)
			{
			addressMode = FALSE;           					// nur einmal ausführen
																		// Write a byte
			PORT_USI &= ~(1<<PIN_USI_SCL);               // Pull SCL LOW.
			USIDR     = *(msg++);                        // Setup data.
			USI_TWI_Master_Transfer( USISR_8bit );   // Send 8 bits on bus.

// Clock and verify (N)ACK from slave -----------------------------------------
			DDR_USI  &= ~(1<<PIN_USI_SDA);               // Enable SDA as input.
			if(( USI_TWI_Master_Transfer( USIST_1bit )) & 0x01) //(1<<TWI_NACK_BIT) )
				{
				if ( addressMode )
					{
					errorState = USI_TWI_NO_ACK_ON_ADDRESS;
					return (FALSE);   // ergänzt
					}
				else
					{
					errorState = USI_TWI_NO_ACK_ON_DATA;
					return (FALSE);
					}
				}
			}
		else															// else masterRead cycle
			{															// Read a data byte
			DDR_USI &= ~(1<<PIN_USI_SDA);              	// Enable SDA as input.
			*(msg++) = USI_TWI_Master_Transfer( USISR_8bit ); // Byte einlesen

// Prepare to generate ACK (or NACK in case of End Of Transmission) -----------
			if( msgSize == 1)                            // If transmission of last byte was performed.
				{
				USIDR = 0xFF;                             // Load NACK to confirm End Of Transmission.
				}
			else
				{
				USIDR = 0x00;                             // Load ACK. Set data register bit 7 (output for SDA) low.
				}
			USI_TWI_Master_Transfer( USIST_1bit );   // Generate ACK/NACK.
			}
		}
	while( --msgSize) ;                             	// Until all data sent/received.

	USI_TWI_Master_Stop();                           	// Send a STOP condition on the TWI bus.

	return (TRUE);													// Transmission successfully completed
}

/*---------------------------------------------------------------
 Core function for shifting data in and out from the USI.
 Data to be sent has to be placed into the USIDR prior to calling
 this function. Data read, will be returned from the function.
---------------------------------------------------------------*/
uint8_t USI_TWI_Master_Transfer( uint8_t temp )
{
	USISR = temp;                                     	// Set USISR according to temp.
																		// Prepare clocking.
	temp  =  (0<<USISIE)|(0<<USIOIE)|                 	// Interrupts disabled
            (1<<USIWM1)|(0<<USIWM0)|                 	// Set USI in Two-wire mode.
            (1<<USICS1)|(0<<USICS0)|(1<<USICLK)|     	// Software clock strobe as source.
            (1<<USITC);                              	// Toggle Clock Port.
	do
	{
		_delay_us( T2_TWI );
		USICR = temp;                          			// Generate positve SCL edge.
		while( !(PIN_USI & (1<<PIN_USI_SCL)) );			// Wait for SCL to go high.
		_delay_us( T4_TWI );
		USICR = temp;                          			// Generate negative SCL edge.
	}																	// solange, bis 8Bit raus sind
	while( !(USISR & (1<<USIOIF)) );        				// (kein Overflow Flag gesetzt)

	_delay_us( T2_TWI );
	temp  = USIDR;                          				// Read out data.
	USIDR = 0xFF;                            				// Release SDA.
	DDR_USI |= (1<<PIN_USI_SDA);             				// Enable SDA as output.

	return temp;                             				// Return the data from the USIDR
}

// ----------------------------------------------------------------------------
// Function for generating a TWI Stop Condition. Used to release the TWI bus.
uint8_t USI_TWI_Master_Stop( void )
{
	PORT_USI &= ~(1<<PIN_USI_SDA);           				// Pull SDA low.
	PORT_USI |= (1<<PIN_USI_SCL);            				// Release SCL.
	while( !(PIN_USI & (1<<PIN_USI_SCL)) );  				// Wait for SCL to go high.
	_delay_us( T4_TWI );
	PORT_USI |= (1<<PIN_USI_SDA);            				// Release SDA.
	_delay_us( T2_TWI );

#ifdef SIGNAL_VERIFY
	if( !(USISR & (1<<USIPF)) )								// wenn kein USI-Stop erkannt wurde
		{
		errorState = USI_TWI_MISSING_STOP_CON;
		return (FALSE);
		}
#endif

	return (TRUE);
}

// --- [ eof ] ----------------------------------------------------------------
