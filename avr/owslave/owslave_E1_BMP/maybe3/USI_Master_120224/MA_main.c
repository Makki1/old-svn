// --- [ MA_main.c ] ----------------------------------------------------------
//
//       tab = 3 - 24.02.2012
//
//       Testprogramm für den USI-Master auf einem ATtiny25, sendet an PCF8574

/*****************************************************************************
*
* Atmel Corporation
*
* File              : main.c
* Compiler          : WINAVR
* Revision          : $Revision: 1.11 $
* Date              : $Date: Tuesday, September 13, 2005 09:09:36 UTC $
* Updated by        : $Author: jtyssoe $
*
* Support mail      : avr@atmel.com
*
* Supported devices : All device with USI module can be used.
*                     The example is written for the ATmega169, ATtiny26 & ATtiny2313
*
* AppNote           : AVR310 - Using the USI module as a TWI Master
*
* Description       : Schreibt eine aufsteigende Zahlenfolge in Zeitintervallen
*                     and einen PCF8574 und liest die Daten zurück
*
****************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <util/delay.h>
#include "USI_Master.h"

#define SLAVE_ADR  		   0x40				// Adresse des TWI-Slave (PCF8574)
#define TWI_BUFFER_SIZE    16					// Größe des Array für Daten (3 + 1Byte Adresse)
                                          // das wird per Referenz an usi_twi_master übergeben

// ----------------------------------------------------------------------------
int main( void )
{
	uint8_t twi_buffer[TWI_BUFFER_SIZE];
	uint8_t i = 0, j, k, e;

                                          // den Takt vom intern 8MHz auf 4 MHz reduzieren
	CLKPR = (1<<CLKPCE);						   // vorbereiten der Aktion durch setzen von CLKPCE
	CLKPR = (1<<CLKPS0);						   // innerhalb von 4 Takten neuen Wert schreiben (Teiler = 2)

	DDRB  = (1<<PB1 | 1<<PB4);					// PB1 / PB4 für LED vorbereiten

	USI_MA_Init();                         // den Master initialisieren

	while(1)									      // Endlosschleife
		{
		twi_buffer[0] = SLAVE_ADR;
		twi_buffer[1] = i;
		twi_buffer[2] = i;

      e  = USI_MA_Write(twi_buffer, 3);   // Daten schreiben, ein Fehler liefert 0, Erfolg 1
      e += USI_MA_Read(twi_buffer, 3);    // Daten lesen, ein Fehler liefert 0, Erfolg 1

		j = twi_buffer[1];                  // gelesener Status

		if (j == i)                         // wenn beide übereinstimmen
         {
         i++;
         PINB = (1<<PB1);                 // an PB1 blinken
         }

      if (!(e == 2)) PINB = (1<<PB4);     // wenn nicht beide Übertragungen erfolgreich waren, dann blinken

		k = 10;
		while(--k)	_delay_ms(100);         // 1 Sekunde warten

	}
	return 0;
}

// --- [ eof ] ----------------------------------------------------------------
