/*
 *      __________  ________________  __  _______
 *     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
 *    / /_  / /_/ / __/ / __/ / __  / / / /\__ \ 
 *   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ / 
 *  /_/   /_/ |_/_____/_____/_____/\____//____/  
 *                                      
 *  Copyright (c) 2008 Andreas Krebs <kubi@krebsworld.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#ifndef FB_APP_IN8
#define FB_APP_IN8


#define USERRAMADDRH  	0x1C	// UserRam start
#define DEBTIME			0xD2	// Entprellzeit in 0,5ms
//#define RELMODE			0xF2	// Relaisbetrieb
#define	DELAYTAB		0xF9	// Start der Tabelle f�r Verz�gerungswerte (Basis)
#define PDIR			0xFF	// Port-Richtung, wenn Bit gesetzt dann ist der entsprechende Pin ein Ausgang (f�r kombinierte Ein-/Ausg�nge)

#define TIMERANZ		0x09	// timeranzahl (17)
#define PROTTIMER		TIMERANZ-1// PROTTIMER ist stets der letzte timer(0-7user,8prot,Gesmatzahl=9)

//#define IN8_2TE					// nur f�r shifter version des in8
#define wertgeber				// mit Wertgeber
//#define zaehler					// mit Z�hler
#define dimmer				// mit Dimmfunktionen
#define zykls					// mit zyklisches senden

#define VERSION 07
#define TYPE    0x02

extern unsigned char portbuffer,p0h;	// Zwischenspeicherung der Portzust�nde
extern unsigned char blocked;	// Sperrobjekte
extern unsigned char timerbase[TIMERANZ];// Speicherplatz f�r die Zeitbasis 
extern unsigned char timercnt[TIMERANZ];// speicherplatz f�r den timercounter und 1 status bit
extern unsigned char timerstate[TIMERANZ];//
extern unsigned int  __at 0x19 zaehlervalue[2];
void pin_changed(unsigned char pinno);
void schalten(__bit risefall, unsigned char pinno);	// Schaltbefehl senden
unsigned char pin_function(unsigned char pinno);	// Funktion des Eingangs ermitteln
unsigned char debounce(unsigned char pinno);		// Entprellzeit abwarten und pr�fen
void send_cyclic(unsigned char pinno);
unsigned char operation(unsigned char pinno);
unsigned char switch_dim(unsigned char pinno);
int eis5conversion(unsigned char zahl,unsigned char typ);
void delay_timer(void);
void write_value_req(void);	
void sperren(unsigned char objno,unsigned char freigabe);
unsigned long read_obj_value(unsigned char objno);
void write_obj_value(unsigned char objno,long objvalue);
void read_value_req(void);
void write_send(unsigned char objno,unsigned int objval);

void send_value(unsigned char type, unsigned char objno, int sval);
void restart_app(void);		// Alle Applikations-Parameter zur�cksetzen
void bus_return(void);// verhalten nach busspannungsr�ckkehr.
extern const unsigned char bitmask_1[8];

#endif

/*Bedeutung der bits in timercnt[]:
   	bit 0-6 Z�hlwert
   	bit 7 timer-run bit

  Bedeutung der bits in timerstate[]:
	bit 0 Dimmen:schrittweite, Jalo:ab, Schalten:ein,
	bit 1 Dimmen:schrittweite,
	bit 2 Dimmen:schrittweite,
	bit 3 Dimmen:heller,
	bit 4 Jalo:T2,
	bit 5 Schalten:2.ebene,
	bit 6 Dimmen,
	bit 7 Jalo lang,
	
  Bedeutung der bits in timerbase[]
	bit 0-3 Zeitbasis (0=grundbasis von rtc(xx)*2; 1= wie 0 * 2, 2= wie 0*4 ...
	bit 4
	bit 5
	bit 6
	bit 7  && timerstate==0 :impulsz�hler aktiv
*/





