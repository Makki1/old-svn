/*
 *      __________  ________________  __  _______
 *     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
 *    / /_  / /_/ / __/ / __/ / __  / / / /\__ \ 
 *   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ / 
 *  /_/   /_/ |_/_____/_____/_____/\____//____/  
 *                                      
 *  Copyright (c) 2012 Andreas Krieger
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

#define TIMERANZ		0x04	// timeranzahl (17)

//#define IN8_2TE					// nur f�r shifter version des in8
//#define wertgeber				// mit Wertgeber
//#define zaehler					// mit Z�hler

//#define zykls					// mit zyklisches senden

#define VERSION 00
#define TYPE    00

extern unsigned char portbuffer,p0h;	// Zwischenspeicherung der Portzust�nde

extern unsigned char timerbase[TIMERANZ];// Speicherplatz f�r die Zeitbasis 
extern unsigned char timercnt[TIMERANZ];// speicherplatz f�r den timercounter und 1 status bit
extern unsigned char timerstate[TIMERANZ];//
extern unsigned long __at 0x08 counter_summe[4];
extern unsigned int  __at 0x18	counter_moment[4];
extern const unsigned char bitmask_1[8];
extern unsigned char timerflags;
extern volatile unsigned char precounter0;
extern volatile unsigned char precounter1;
extern volatile __bit counted0;
extern volatile __bit counted1;

void keypad_isr  (void) __interrupt (7);
void keypad_init(void);
void pin_changed(unsigned char pinno);
void schalten(__bit risefall, unsigned char pinno);	// Schaltbefehl senden
unsigned char pin_function(unsigned char pinno);	// Funktion des Eingangs ermitteln
unsigned char debounce(unsigned char pinno);		// Entprellzeit abwarten und pr�fen
void send_cyclic(unsigned char pinno);
unsigned char operation(unsigned char pinno);
unsigned char switch_dim(unsigned char pinno);
int eis5conversion(unsigned long zahl);
void delay_timer(void);
void write_value_req(void);	
//void sperren(unsigned char objno,unsigned char freigabe);
unsigned long read_obj_value(unsigned char objno);
void write_obj_value(unsigned char objno,unsigned long objvalue);
void read_value_req(void);
void write_send(unsigned char objno,unsigned int objval);
void sendbychange(unsigned char objno,unsigned char val);
void send_counter(unsigned char obj,unsigned char ctno);
void send_value(unsigned char type, unsigned char objno, int sval);
void restart_app(void);		// Alle Applikations-Parameter zur�cksetzen
void bus_return(void);// verhalten nach busspannungsr�ckkehr.
void checklevel(__bit fullcheck,unsigned char check);
#endif

/*Bedeutung der bits in timercnt[]:
   	bit 0-6 Z�hlwert
   	bit 7 timer-run bit

	
  Bedeutung der bits in timerbase[]
	bit 0-3 Zeitbasis (0=grundbasis von rtc(xx)*2; 1= wie 0 * 2, 2= wie 0*4 ...
	bit 4
	bit 5
	bit 6
	bit 7  L�schanforderung
*/





