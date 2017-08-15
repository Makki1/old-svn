/*
 *      __________  ________________  __  _______
 *     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
 *    / /_  / /_/ / __/ / __/ / __  / / / /\__ \ 
 *   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ / 
 *  /_/   /_/ |_/_____/_____/_____/\____//____/  
 *                                      
 *  Copyright (c) 2013 Andreas Krieger
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */
#ifndef FB_APP_LC
#define FB_APP_LC

// Parameter-Adressen im EEPROM

// Adressen zum speichern von Applikations Daten
#define PORTSAVE	0x99	// Portzust�nde
#define TIMERANZ	0x02	// timeranzahl





#define REFRESH \
		//P0= oldportbuffer;	// refresh des Portzustandes in der hal
							// f�r astabile Relaise 
// SPI Konfiguration
#define CLK			P0_1
#define DATAOUT		P0_0
#define WRITE		P0_2
#define beep_port	P0_6//P1_2
#define QUIT		P0_7//P1_3
extern 	__bit portchanged;// globale variable, sie ist 1 wenn sich portbuffer ge�ndert hat
extern unsigned char portbuffer;

extern unsigned char led_obj[3];
extern unsigned char led_hell_obj;
extern unsigned char quitted_obj[3];
extern __bit zentral_alarm_obj,reset_obj;
extern unsigned char blink;
extern unsigned char t0_div;

extern const unsigned int timerflagmask[];
extern const unsigned char bitmask_1[];
extern const unsigned char bitmask_0[];
extern const unsigned char bitmask_11[];

void timer0_int(void) __interrupt (1);
//void write_value_req(void);		// Hauptroutine f�r Ausg�nge schalten gem�� EIS 1 Protokoll (an/aus)
//void read_value_req(void);
void delay_timer(void);		// z�hlt alle 130ms die Variable Timer hoch und pr�ft Queue
void LED_schalten(void);	// Ausg�nge schalten
void spi_2_out(unsigned char daten);
//unsigned int sort_output(unsigned char portbuffer);
void bus_return(void);		// Aktionen bei Busspannungswiederkehr
void restart_app(void);		// Alle Applikations-Parameter zur�cksetzen
unsigned long read_obj_value(unsigned char objno);	// gibt den Wert eines Objektes zurueck
void write_obj_value(unsigned char objno,unsigned int objvalue);	// schreibt den aktuellen Wert eines Objektes ins 'USERRAM'
void hell_stellen (void);// stellt die Helligkeit der LEDs ein
void erease_alarm(__bit value);
#endif
