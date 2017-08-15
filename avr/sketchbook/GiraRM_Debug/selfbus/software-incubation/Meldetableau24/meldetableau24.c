/*
 *      __________  ________________  __  _______
 *     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
 *    / /_  / /_/ / __/ / __/ / __  / / / /\__ \ 
 *   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ /
 *  /_/   /_/ |_/_____/_____/_____/\____//____/  
 *                                      
 *  Copyright (c) 20013 Andreas Krieger
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */
/**
* @file   meldetableau24.c
* @author Andreas Krieger 
* 
*/

#include <P89LPC922.h>
#include "../lib_lpc922/Releases/fb_lpc922_1.4x.h"
#include "fb_app_meldetableau24.h"
#include "../com/debug.h"
//#include "../com/debug.c"
#include "../com/fb_rs232.h"
#include"../com/watchdog.h"
#include"../com/watchdog.c"


#define TYPE 0
#define VERSION 0
/** 
* The start point of the program, init all libraries, start the bus interface, the application
* and check the status of the program button.
* 
*
*/

unsigned char __at 0x00 RAM[00]; //nur f�r die debug ausgabe


void main(void)
{ 
	unsigned char n,cmd,prog_button_level=0,quit_button_level=0;
	signed char cal;
	static __code signed char __at 0x1BFF trimsave;
	__bit prog_button_toggled=0,quit_button_toggled=0;

	restart_hw();							// Hardware zuruecksetzen
	TASTER=0;
	cal=trimsave;
	TRIM = TRIM+trimsave;
	//...rs_init...(6);im folgenden direkt:
	BRGCON&=0xFE;	// Baudrate Generator stoppen
	P1M1&=0xFC;		// RX und TX auf bidirectional setzen
	P1M2&=0xFC;
	SCON=0x50;		// Mode 1, receive enable
	SSTAT|=0xE0;	// TI wird am Ende des Stopbits gesetzt und Interrupt nur bei RX und double TX buffer an
	BRGCON|=0x02;	// Baudrate Generator verwenden aber noch gestoppt
	BRGR1=0x2F;	// Baudrate = cclk/((BRGR1,BRGR0)+16)
	BRGR0=0xF0;	// f�r 115200 0030 nehmen, autocal: 600bd= 0x2FF0
	BRGCON|=0x01;	// Baudrate Generator starten
	SBUF=0x55;

	TASTER=0;
	for (n=0;n<50;n++) {		// Warten bis Bus stabil
		TR0=0;					// Timer 0 anhalten
		TH0=eeprom[ADDRTAB+1];	// Timer 0 setzen mit phys. Adr. damit Ger�te unterschiedlich beginnen zu senden
		TL0=eeprom[ADDRTAB+2];
		TF0=0;					// �berlauf-Flag zur�cksetzen
		TR0=1;					// Timer 0 starten
		while(!TF0);
	}
	watchdog_init();
	watchdog_start();

	restart_app();							// Anwendungsspezifische Einstellungen zuruecksetzen
	bus_return();							// Aktionen bei Busspannungswiederkehr

	do  {
		//DEBUGPOINT
		
		watchdog_feed();	
		
		if(APPLICATION_RUN) {	// nur wenn run-mode gesetzt
			if (RTCCON &0x80){	
			delay_timer();
			LED_schalten();
		}

			
		cmd;	
		
		if (RI){
			RI=0;
			cmd=SBUF;
			if(cmd=='c'){
				while(!TI);
				TI=0;
				SBUF=0x55;
			}
			if(cmd=='+'){
				TRIM--;
				cal--;
			}
			if(cmd=='-'){
				TRIM++;
				cal++;
			}
			if(cmd=='w'){
				EA=0;
				START_WRITECYCLE;	//cal an 0x1bff schreiben

				FMADRH= 0x1B;		
				FMADRL= 0xFF; 

				FMDATA=	cal;
				STOP_WRITECYCLE;
				EA=1;				//int wieder freigeben
			}
			if(cmd=='p')status60^=0x81;	// Prog-Bit und Parity-Bit im system_state toggeln
			if(cmd=='v'){
				while(!TI);
				TI=0;
				SBUF=VERSION;
			}
			if(cmd=='t'){
				while(!TI);
				TI=0;
				SBUF=TYPE;
			}
		}//end if(RI...
		


		//   ###  QUIT_button ###
		if(!QUIT){ 		// Quitt-Taster gedr�ckt
			if(quit_button_level<255)	quit_button_level++;
			else{
				if(!quit_button_toggled && (eeprom[0xEE]&0x08)){
					reset_obj=1;
					erease_alarm(1);
					if(eeprom[0xEE]&0x01)send_obj_value(26);
				}
				quit_button_toggled=1;
			}
		}
		else {
			if(quit_button_level>0)prog_button_level--;
			else quit_button_toggled=0;
		}

				
		}// end if(runstate)
		
		
		n= tx_buffer[(tx_nextsend-1)&0x07];//n ist die letzte objno
		if (tel_arrived || (n==25 && tel_sent)) { // 
			tel_arrived=0;
			tel_sent=0;
			process_tel();
			
		}
		
	// ### PROG Button ###

		TASTER=1;				// Pin als Eingang schalten um Taster abzufragen
		if(!TASTER){ // Taster gedr�ckt
			if(prog_button_level<255)	prog_button_level++;
			else{
				if(!prog_button_toggled)status60^=0x81;	// Prog-Bit und Parity-Bit im system_state toggeln
				prog_button_toggled=1;
			}
		}
		else {
			if(prog_button_level>0)prog_button_level--;
			else prog_button_toggled=0;
		}
		TASTER=!(status60 & 0x01);	// LED entsprechend Prog-Bit schalten (low=LED an)
		
		

		for(n=0;n<100;n++) {}	// falls Hauptroutine keine Zeit verbraucht, der LED etwas Zeit geben, damit sie auch leuchten kann
	} while(1);
}


