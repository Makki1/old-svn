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

#ifndef FB_APP_d2c
#define FB_APP_d2c

//#define einkanal
//#define applilpc
#define i2c_appli
//#define HAND				// Handsteuerung aktiv (auskommentieren wenn nicht gew�nscht)
#define MAX_PORTS_4			// Anzahl Ausg�nge (nur 4 oder 8 erlaubt)
//#define SPIBISTAB			// Serielle Ausgabe f�r bistabile relaise aktivieren
// Parameter-Adressen im EEPROM
#define FUNCASS		0xD8	// Startadresse der Zuordnung der Zusatzfunktionen (2 Byte)
#define OFFDISABLE	0xEB	// Aus-Telegramm ignorieren
#define FUNCTYP		0xED	// Typ der Zusatzfunktion
#define LOGICTYP	0xEE	// Verkn�pfungs Typ 0=keine 1=ODER 2=UND 3=UND mir R�ckf�hrung
#define BLOCKACT	0xEF	// Verhalten beim Sperren
#define BLOCKPOL	0xF1	// Polarit�t der Sperrobjekte
#define RELMODE		0xF2	// Relaisbetrieb (�ffner/Schlie�er)
#define RMINV		0xF3	// R�ckmeldung invertiert oder normal
#define	DELAYTAB	0xF9	// Start der Tabelle f�r Verz�gerungswerte (Basis)

// Adressen zum speichern von Applikations Daten
#define PORTSAVE	0x99	// Portzust�nde
#define TIMERANZ	0x06	// timeranzahl




#define DUTY	0x50	// 0xFF=immer low 0x00=immer high

#define REFRESH \
		//P0= oldportbuffer;	// refresh des Portzustandes in der hal
							// f�r astabile Relaise 
// SPI Konfiguration
#define CLK			P0_3
#define BOT_OUT		P0_0
#define MID_OUT		P0_1
#define WRITE		P0_2

extern 	__bit portchanged;// globale variable, sie ist 1 wenn sich portbuffer ge�ndert hat
//extern __bit sync_blocked;
__data __at (0x25) extern unsigned char portbuffer;
__bit __at(0x2C)A1;// bitadresse 0x2C ist byteadresse 0x25_4 (portbuffer_4)
__bit __at(0x2D)A2;
__bit __at(0x2E)A3;
__bit __at(0x2F)A4;
__bit __at(0x44)SM_1;
__bit __at(0x45)SM_2;
__bit __at(0x46)SM_3;
__bit __at(0x40)S_1;// Die Sperren bitadressen f�r byte 0x28
__bit __at(0x41)S_2;
__bit __at(0x42)S_3;
//extern __data __at (0x24) unsigned char syncval;
extern unsigned char dimmtimervorteiler;
extern __data __at (0x17) unsigned char dimmwert[2];
extern __data __at (0x0B) unsigned char dimmziel[2];
extern __data __at (0x1A) unsigned char dimmpwm[3];
extern __data __at (0x28) unsigned char sperren;
extern __data __at (0x24) unsigned char dimmcompare;
extern __data __at (0x11) unsigned char helligkeit[2];
extern  unsigned char helligkeit_RM[2];

extern unsigned char aushell[2];

extern const unsigned char grundhelligkeit_tabelle[];
extern const unsigned char prozentvalue[];

extern const unsigned int timerflagmask[];
extern const unsigned char bitmask_1[];
extern const unsigned char bitmask_0[];
extern const unsigned char bitmask_11[];

void ext0int (void) __interrupt (0);
void timer0_int(void) __interrupt (1);
//void write_delay_record(unsigned char objno, unsigned char delay_status, long delay_target);	// Schreibt die Schalt-Verzoegerungswerte ins Flash
//void clear_delay_record(unsigned char objno); // Loescht den Delay Eintrag
//void write_value_req(void);		// Hauptroutine f�r Ausg�nge schalten gem�� EIS 1 Protokoll (an/aus)
//void read_value_req(void);
void delay_timer(void);		// z�hlt alle 130ms die Variable Timer hoch und pr�ft Queue
void port_schalten(void);	// Ausg�nge schalten
void object_schalten(unsigned char objno, __bit objstate);	// Objekt schalten
void spi_2_out(unsigned int daten);
unsigned int sort_output(unsigned char portbuffer);
void bus_return(void);		// Aktionen bei Busspannungswiederkehr
void restart_app(void);		// Alle Applikations-Parameter zur�cksetzen
void read_dimmziel(unsigned char objno,unsigned char offset);
unsigned long read_obj_value(unsigned char objno);	// gibt den Wert eines Objektes zurueck
void write_obj_value(unsigned char objno,unsigned int objvalue);	// schreibt den aktuellen Wert eines Objektes ins 'USERRAM'
void hell_stellen (unsigned char obj,unsigned char value);// stellt die Helligkeit nach Helligkeit oder Lichszene tele ein
void dimmen_obj(unsigned char obj,unsigned char value); //Dimmt nach Telgramm oder hand.
void tastenauswertung(void);
unsigned char sperrvalue(unsigned char index,unsigned char obj);// holt Sperrvalue nach index

#endif
