/*
 *      __________  ________________  __  _______
 *     / ____/ __ \/ ____/ ____/ __ )/ / / / ___/
 *    / /_  / /_/ / __/ / __/ / __  / / / /\__ \ 
 *   / __/ / _, _/ /___/ /___/ /_/ / /_/ /___/ / 
 *  /_/   /_/ |_/_____/_____/_____/\____//____/  
 *                                      
 *  Copyright (c) 2009-2012 Andreas Krebs <kubi@krebsworld.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *
 *	V1.00	erste Version statemachine
 *	V1.10	Senden komplett �ber Interrupt und Ringspeicher realisiert
 *	V1.20	Senden um 3- und 4-Byte Objekte erweitert, read_obj_value ist jetzt unsigned long
 *	V1.21	rekursives Abarbeiten gesendeter Telegramme korrigiert: tel_arrived widr jetzt
 *			in state 13 gesetzt
 *	V1.22	statt tel_arrived wird tel_sent in state13 gesetzt (Senden eines Telegramms)
 *	V1.23	ack-Position leicht korrigiert (state 4)
 *	V1.24	EX1 vorm Schlafengehen der statemachine hart eingeschaltet, die lib war manchmal "taub" geworden
 *	V1.30	komplett umgebaut bei senden und wiederhol senden, state 80 raus, repeat_tx raus, kollision gefixt
 *	V1.31	wiederholtes (versp�tetes) ack senden raus
 * 
 */


#include <P89LPC922.h>
#include "fb_lpc922_mini.h"

unsigned char conh, conl;	// bei bestehender Verbindung phys. Adresse des Kommunikationspartners
unsigned char pcount;		// Paketzaehler, Gruppenadresszaehler
unsigned char mem_length, mem_adrh, mem_adrl, senders_pcount;	// l�nge bei memory_read_request

unsigned char telegramm[23];
unsigned char tx_buffer[8];		// Ringspeicher f�r zu sendende Telegramme
unsigned char telpos;			// Zeiger auf naechste Position im Array Telegramm
unsigned char cs;				// checksum
unsigned char fbrx_byte, fb_pattern;
volatile unsigned char fb_state, repeat_count;

__code unsigned char __at 0x1C00 userram[255];	/// Bereich im Flash fuer User-RAM
__code unsigned char __at 0x1D00 eeprom[255];	/// Bereich im Flash fuer EEPROM

__bit parity_ok;			// Parity Bit des letzten empfangenen Bytes OK
volatile __bit interrupted;	// Wird durch interrupt-routine gesetzt. So kann eine andere Routine pruefen, ob sie unterbrochen wurde
__bit fb_parity, ack, nack, its_me, tel_arrived, tel_sent, auto_ack, fbtx_bit, wait_for_ack;
__bit send_ack, send_nack, transparency;
volatile __bit connected;	/// Verbindung aufgebaut
unsigned char tx_nextwrite, tx_nextsend;
unsigned char status60;		// Statusbyte, das normalerweise im userram an Adresse 0x60 steht
__bit inc_pcount;






void X1_int(void) __interrupt (2)		// Flanke des Startbits l�st int1 aus
{
					//											5 (3..8) cycle Einsprung
	EX1=0;			// int1 stoppen								1 cycle = 0,27127us
	TR1=0;			// Timer 1 stoppen
	TMOD=(TMOD & 0x0F) + 0x20;	// Timer 1 als 8-Bit autoreload
	TL1=53;//108;//128;		// Timer laden so dass in state 2 bei 75�s abgefragt wird
	TH1=128;//128;			// autoreload auf 35�s setzen (gilt f�r sprung von 2 auf 3)
	TF1=0;			// Timer1-flag l�schen						1 cycle
	TR1=1;			// Timer1 starten							1 cycle
	ET1=1;			// Timer1-int aktiv							1 cycle
	fb_state=2;		// n�chster state
	fb_parity=0;	// Parity Bit l�schen
	parity_ok=1;	// wird bei Fehler gel�scht
	fb_pattern=0;
	fbrx_byte=0;
	interrupted=1;	// signalisiert app, dass unterbrochen wurde
}



void T1_int(void) __interrupt (3) 	// Timer 1 Interrupt
{
												//							5 (3..8) cycle Einsprung
	__bit fbrx_bit;								//							30 cycle push etc.
	unsigned char send_byte;//,n;

	switch (fb_state){

	// Empfang aktiv, Busfrei counter runterz�hlen und wenn Bus frei, dann Sendespeicher
	// pr�fen und ggf. Senden initiieren
	case 0:
		TR1=0;			// Timer 1 stoppen
		TMOD=(TMOD & 0x0F) + 0x20;	// Timer 1 als 8-Bit autoreload
		TL1=128;		// Timer laden
		TH1=128;
		TF1=0;			// Timer1-flag l�schen						1 cycle
		TR1=1;			// Timer1 starten							1 cycle
		if(tx_nextsend != tx_nextwrite) { 			// wenn zu sendendes Objekt vorhanden
			if(build_tel(tx_buffer[tx_nextsend])) {	// wenn Telegramm gebildet werden konnte
				EX1=0; 				//Um zu vermeiden dass man in die Abfrage hinein-empf�ngt
				if(!fb_state) { 	//nur bilden wenn vorher kein rx_intit durch ext int kam
					if((tx_buffer[tx_nextsend]&0x20)==0) {	// wenn erstmaliges Senden des Objektes
						tx_buffer[tx_nextsend]|=0x20;		// Bit f�r "wird gerade gesendet"
						repeat_count=0;						// Wiederholungsz�hler f�r nicht geackte Telegramme
					}
					if (repeat_count<4) init_tx();		// Senden starten
					else {		// wenn bereits 4 x wiederholt oder erfolgreich gesendet(geackt) -> n�chstes Objekt
						tx_nextsend++;
						tx_nextsend&=0x07;
						wait_for_ack=0;
						inc_pcount=0;
						init_rx();
						TR1=0;		// hier nicht noch einmal die ganze busfree Zeit warten
						TH1=0xF0;
						TL1=0x00;
						TR1=1;
					}
				}
				EX1=1;	// ext1 int einschalten falls Empfang...
			}
			else {
				tx_nextsend++; //hier Zeiger erh�hen wenn Telegramm nicht gebildet werden konnte
				tx_nextsend&=0x07;
			}
		}
		else {
			TR1=0;		// nichts mehr zu tun, also statemachine stoppen
			EX1=1;		// sicherstellen, dass ext. Int. 1 aktiv ist
		}
		break;


	case 2: // T=75us
		fbrx_bit=!IE1;				// Flipflop des ext. Int. 1 auslesen, ist 1 bei fallender Flanke am Pin
		IE1=0;						// Flipflop zur�cksetzen
		fb_state=3;// n�chster state: 3
		TH1=0;// reload auf 70�s (zeit swischen state 3 und 2)
		if(fb_pattern==0) {			// fb_pattern=0 bedeutet, dass bisher nur das Startbit empfangen wurde
			fb_pattern=1;			// 70us nach Startbit, als n�cstes kommt Datenbit 0
			ack=0;					// Empfang eines neuen Bytes, also ack/nack Flags zur�cksetzen
			nack=0;
		}
		else {						// Daten oder Parity Bit
			if(fb_pattern<129) {					// Daten-Bit LSB first
				if(fbrx_bit) {
					fb_parity=!fb_parity;			// Paritybit berechnen
					fbrx_byte+=fb_pattern;			// Datenbit speichern
				}
				fb_pattern=fb_pattern<<1;			// n�chstes Bit
				if(fb_pattern==0) fb_pattern=129;	// alle Daten-Bits gelesen, Parity-Bit folgt
			}
			else {									// Parity-Bit wurde empfangen
				TR1=0;
				TF1=0; //pr�fen ob erforderlich!!!
				if(fb_parity==fbrx_bit) {				// Parity-Bit OK
					if (telpos==0) {						// erstes empfangenes Byte
						if (fbrx_byte==0xCC) ack=1;				// ACK empfangen
						if (fbrx_byte==0x0C) nack=1;			// NACK empfangen
					}
					if (!ack && !nack) {					// Datenbyte empfangen
						telegramm[telpos]=fbrx_byte;			// Byte speichern
						cs^=fbrx_byte;							// Checksum berechnen
						telpos++;								// Telegrammzeiger erh�hen
					}
				}
				else parity_ok=0;						// Parity Error

				if (wait_for_ack) {						// es wird ein ACK erwartet
					wait_for_ack=0;							// Flag zur�cksetzen, da wir es ja gerade abarbeiten
					if (ack && parity_ok) {					// ACK empfangen und auch erwartet
						repeat_count=4;						// keine Wiederholtelegramme mehr senden
						if(inc_pcount) {
							inc_pcount=0;
							pcount+=4;
							pcount&=0x3C;
						}
					}
					init_rx();
				}
				else {									// kein ACK erwartet
					if (parity_ok && (!ack && !nack)) {		// ganz normales Datenbyte
						TMOD=(TMOD & 0x0F) +0x10;			// Timer 1 als 16-Bit Timer
						TH1=0xFA;//0xFA;							// Timer 1 auf Timeout-Position setzen (370us)
						TL1=0x70;//4f 0xAB; (bis hierher sinds 91�s)
						TR1=1;
						fb_state=4;							// n�chster state: timeout = Telegramm fertig
						EX1=1;								// int1 aktiv, falls noch ein Byte kommt
					}
					else init_rx();							// irgendwas empfangen
				}
			}
		}
		break;

	case 3: // T=104us=0us
		TH1=128;// reload auf 35�s (Zeit zwischen State 2 und 3)
		fb_state=2;//1
		break;

	case 4:	//	Timeout, d.h. Telegramm-Ende
		if (auto_ack && telpos>4) {		// wenn ACK gesendet werden soll und Telegramm zumindest 5 Bytes hat
			TR1=0;
			TMOD=(TMOD & 0x0F) +0x10;	// Timer 1 als 16-Bit Timer
			TH1=0xEF;					// Timer 1 auf ACK-Position setzen (15 Bit Pause = 2708�s (26 Bit) nach Beginn Startbit vom letzten Datenbyte)
			TL1=0x42;
			TR1=1;
			its_me=transparency;			// indiziert, ob dieses Ger�t adressiert wurde, bei transparecy=1 immer
			if(telegramm[5]&0x80) {
				if (gapos_in_gat(telegramm[3],telegramm[4])!=0xFF) its_me=1;	// Gruppenadresse
				if (telegramm[3]==0 && telegramm[4]==0) its_me=1;				// Broadcast
			}
			else if(telegramm[3]==eeprom[ADDRTAB+1] && telegramm[4]==eeprom[ADDRTAB+2]) its_me=1;	// phys. Adresse
			if ((cs==0xFF) && its_me) tel_arrived=1;
			if ((cs==0xFF)) tel_arrived=1;
			fb_state=5;					// n�chster state: ACK-Position erreicht
		}
		else {					// Telegramm soll nicht per ACK best�tigt werden

			if (telpos>1) tel_arrived=1;	// trotzdem den Eingang eines Telegramms anzeigen
			init_rx();						// wieder in den Empfang zur�ck
		}
		break;

	case 5:	// ACK-Position erreicht
		TR1=0;
		if (cs==0xFF) {	// Checksum OK
			if (its_me) {					// Ger�t adressiert
				send_ack=1;
				init_tx();						// Senden initiieren
				wait_for_ack=0;					// bei ACK senden nicht erneut auf ACK warten
			}
			else init_rx();					// Ger�t nicht adressiert, also zur�ck zu Empfang
		}
		else {							// Checksum oder Parity nicht OK
			send_nack=1;
			init_tx();
			wait_for_ack=0;					// bei NACK senden nicht erneut auf ACK warten
		}
		break;



	case 10:	// Byte Senden, T=0us
		send_byte=telegramm[telpos];
		if (send_ack) send_byte=0xCC;
		if (send_nack) send_byte=0x0C;
		EX1=0;					// Empfang verhindern
		if(fb_state==10){
			if (fb_pattern<=128) {			// Startbit und Datenbits (pattern=0:startbit, 1 2 4 8 .. 128 Datenbits, 129 parity)
				if ((send_byte & fb_pattern)==0) {	// wenn Bit logisch 0 ist
					FBOUTC=1;						// Bus runter ziehen
					fbtx_bit=0;						// fbtx_bit dient zur sp�teren Kollisionspr�fung
					fb_parity=!fb_parity;			// Parity-Bit berechnen
				}
				else fbtx_bit=1;
				if (fb_pattern==0) {				// wenn Startbit: parity zur�cksetzen
					fb_pattern=1;					// auf erstes Datenbit zeigen
					fb_parity=0;
				}
				else fb_pattern=fb_pattern<<1;		// n�chstes Datenbit
				if(fb_pattern==0) fb_pattern=129;	// alle Daten-Bits gesendet, Parity-Bit folgt
				fb_state=11;
			}
			else {							// Paritybit senden
				if (!fb_parity) {				// wenn Parity-Bit logisch 0
					FBOUTC=1;						// Bus runterziehen
					fbtx_bit=0;						// fbtx_bit dient zur sp�teren Kollisionspr�fung
				}
				else fbtx_bit=1;
				telpos++;					// n�chstes Byte
				fb_pattern=0;				// wieder mit Startbit beginnen
				fb_state=13;				// Pr�fen ob Telegramm fertig
			}
			TH1=18; //reload 70� -delay interrupt (von state 11 auf 13)
		}
		break;

	case 11:	// Sendestufe aus, T=35us
		if(telpos==0 && fb_pattern==1) {	// 1. Byte Startbit
			repeat_count++;		// angefangener Sendeversuch, also Z�hler erh�hen
		}
		FBOUTC=0;				// Sendestufe aus
		if (fbtx_bit & IE1) {	// Kollision
			wait_for_ack=0;
			init_rx();				// Senden abbrechen und Empfang initialisieren
		}
		else {
			fb_state=10;//12;
			TH1=110;// 35�s + delay (von state 10 auf 11)
		}
		IE1=0;					// Flipflop l�schen
		break;
/*
	case 12:	// T=70us
		fb_state=10;
		break;
*/
	case 13:	// Byte �bertragen, T=35us, pr�fen ob Telegramm fertig
		FBOUTC=0;
		TR1=0;
		TMOD=(TMOD & 0x0F) +0x10;	// Timer 1 als 16-Bit Timer
		TH1=0xFB;					// Timer 1 auf Interbyte Abstand setzen (3 Bit Pause = 312�s 
		TL1=0x90;
		TR1=1;
		if (send_ack || send_nack) {	// ACK/NACK senden
			init_rx();					// ACK senden abgeschlossen, also statemachine auf Anfang
		}
		else {	// Datenbyte senden
			if (telpos>((telegramm[5]&0x0F)+7)) {		// Telegramm fertig gesendet
				tel_sent=1;
				init_rx();	// Telegramm senden abgeschlossen, ggf. wiederholen wenn nicht geackt wird
			}
			else fb_state=14;		// n�chstes Byte: Interbyte Abstand einhalten
		}
		IE1=0;
		break;

	case 14:	// Interbyte Abstand erreicht (=Stopbit + 2 Bit)
		TR1=0;
		TMOD=(TMOD & 0x0F) +0x20;			// Timer 1 als 8-Bit autoreload
		TH1=110;							// Timer 1 auf 104/3 us laden
		TL1=128;
		TF1=0;
		TR1=1;
		fb_state=10;
		break;

//	default:
		//fb_state++;			// bei allen nicht angegebenen states nur state erh�hen
	}
	interrupted=1;			// zeigt der app, dass sie unterbrochen wurde
}


void init_rx(void) 	// Empfangen initiieren (statemachine auf Anfang)
{
	fb_state=0;
	IE1=0;
	TR1=0;
	ET1=0;
	cs=0;
	telpos=0;
	TR1=0;
	TMOD=(TMOD & 0x0F) +0x10;	// Timer 1 als 16-Bit Timer
	TH1=0x89;					// busfree Zeit = 15 Bit (auf ACK) + 11 Bit (ACK) + 53 Bit
	TL1=0xAF;
	send_ack=0;
	send_nack=0;
	TF1=0;
	TR1=1;
	EX1=1;
	ET1=1;
}


void init_tx(void) 		// Checksum des Telegramms berechnen und Senden initiieren
{
	unsigned char n, cs_pos;

	TR1=0;
	TMOD=(TMOD & 0x0F) +0x20;			// Timer 1 als 8-Bit autoreload
	TH1=110;							// von state 10 auf 11
	TL1=128;							// von hier bis state 10
	TF1=0;

	cs_pos=(telegramm[5]&0x0F)+7;		// Position der Checksum im Telegramm
	cs=0xFF;
	for(n=0;n<cs_pos;n++) {
		cs^=telegramm[n];				// Checksum berechnen
	}
	telegramm[cs_pos]=cs;				// Checksum hinter Nutzdaten anf�gen
	ack=0;
	nack=0;
	wait_for_ack=1;

	fb_state=10;						// n�chster state: senden
	fb_pattern=0;						// n�chstes zu sendendes Bit, 0=Startbit
	telpos=0;							// n�chstes zu sendende Byte
	//IE1=0;								// ext. int1 Flag zur�cksetzen
	EX1=0;								// ext. int1 inaktiv
	TR1=1;								// Timer 1 starten
	ET1=1;								// Timer 1 int. aktiv
}





// ermittelt die Position einer GA in der GA-Tabelle, 0xFF falls nicht gefunden
/*
unsigned char gapos_in_gat(unsigned char gah, unsigned char gal)
{
	unsigned char ga_position,n;
	
	ga_position=0xFF; 			// default return Wert 0xFF = nicht gefunden
	if (eeprom[ADDRTAB]<0xFF && !transparency){
		if (eeprom[ADDRTAB]) {
			for (n=1;n<=eeprom[ADDRTAB];n++) {
				if (gah==eeprom[ADDRTAB+n*2+1] && gal==eeprom[ADDRTAB+n*2+2]) ga_position=n;	// Berechnung in [] nicht in lokale var !!!
			}
		}
	}
  return (ga_position);
}
*/




__bit send_obj_value(unsigned char objno)
{
	__bit success=0;

	if(tx_nextsend!=((tx_nextwrite+1)&0x07)) {
		tx_buffer[tx_nextwrite]=objno;
		tx_nextwrite++;
		tx_nextwrite&=0x07;
		success=1;
	}
	TR1=1;	// statemachine starten falls vorher in state 0 gestoppt
	return(success);
}




void process_tel(void)
{
	unsigned char tpdu, apdu;

	tel_arrived=0;
	tpdu=telegramm[6]&0xC3;
	apdu=telegramm[7];

	// Broadcast,
	if(telegramm[3]==0 && telegramm[4]==0) {	// nur wenn wenn Zieladresse = 0
/*		if(status60 & 0x01) {					// und nur im prog mode
			if(tpdu==BROADCAST_PDU_SET_PA_REQ && apdu==SET_PHYSADDR_REQUEST) set_pa();				// 00000000 11000000
			if(tpdu==BROADCAST_PDU_READ_PA && apdu==READ_PHYSADDR_REQUEST) send_obj_value(READ_PHYSADDR_RESPONSE);	// 00000001 00000000
		}
*/	}

	else {	// Unicast oder Multiccast
		if((telegramm[5]&0x80)==0x00) {	// Destination Adress Flag Bit 7, 0=phys. Adr., 1=Gruppenadr.
/*			if(telegramm[3]==eeprom[ADDRTAB+1] && telegramm[4]==eeprom[ADDRTAB+2]) {	// nur wenn es die eigene phys. Adr. ist


				// Unicast
				switch (tpdu) {	// transport layer control field

				case DATA_PDU_MEMORY_OPERATIONS:
					senders_pcount=telegramm[6]&0x3C;
					apdu &= 0xF0;						// da bei memory operations nur obere 4 Bits aktiv
					if(apdu==WRITE_MEMORY_REQUEST) {	// 01pppp10 1000xxxx
						send_obj_value(NCD_ACK);
						write_memory();
					}
					if(apdu==READ_MEMORY_REQUEST) {		// 01pppp10 0000xxxx
						mem_length=telegramm[7];		// Anzahl Bytes f�r sp�teres(!) memory Auslesen
						mem_adrh = telegramm[8];		// Adresse
						mem_adrl = telegramm[9];
						send_obj_value(NCD_ACK);
						send_obj_value(READ_MEMORY_RESPONSE);
					}
					break;

				case DATA_PDU_MISC_OPERATIONS:
					senders_pcount=telegramm[6]&0x3C;
					if(apdu==RESTART_REQUEST) {		// 01pppp11 10000000
						AUXR1|=0x08;				// Software Reset
					}
					if(apdu==READ_MASK_VERSION_REQUEST) {		// 01pppp11 00000000
						send_obj_value(NCD_ACK);
						send_obj_value(READ_MASK_VERSION_RESPONSE);
					}
					break;

				case CONNECT_PDU:	// 10000000 xxxxxxxx
					if(!connected) {				// wenn bereits verbunden: ignorieren
						connected=1;
						conh=telegramm[1];			// phys. Adresse des Verbindungspartners
						conl=telegramm[2];
						pcount=0;					// Paketzaehler zuruecksetzen
						inc_pcount=0;
					}
					break;

				case DISCONNECT_PDU:	// 10000001 xxxxxxxx
					if(conh==telegramm[1] && conl==telegramm[2] && connected)	{	// nur abbauen, wenn verbunden und Anforderung vom Verbindungspartner, kein ACK senden
						connected=0;
						//pcount=0;					// Paketzaehler zuruecksetzen
						//inc_pcount=0;
					}
					break;

				case NACK_PDU:	// 11pppp11 xxxxxxxx
					send_obj_value(T_DISCONNECT);
					break;
				}
			}
*/		}

		// Multicast, wenn Zieladresse Gruppenadresse ist
		else {
			if(tpdu==GROUP_PDU){
				if(((apdu&0xC0)==WRITE_GROUP)||((apdu&0xC0)==0x40)) write_value_req();	// Objektwerte schreiben (zB. EISx)		00000000 10xxxxxx
				if(apdu==READ_GROUP_REQUEST) read_value_req();	// Objektwert lesen und read_value_response senden	00000000 00000000
			}
		}
	}
}





/*
void write_memory(void)
{
	unsigned char ab,n;

	ab=telegramm[7]&0x0F;		// Anzahl Bytes

	while(fb_state!=0);					// warten falls noch gesendet wird

	EA=0;
	START_WRITECYCLE;					// load command, leert das pageregister
	for(n=0;n<ab;n++) {
		if(telegramm[8]==0 && (telegramm[9]+n)==0x60) status60=telegramm[10+n];
		else WRITE_BYTE(telegramm[8],telegramm[9]+n,telegramm[n+10]);

		if ((((telegramm[9]+n)&0x3F)==0x3F) && n!=(ab-1)) {		// Ende des 64-Byte Pageregisters, also zwischendurch flashen
			STOP_WRITECYCLE;			// write command, schreibt pageregister ins flash und versetzt CPU in idle fuer 4ms
			START_WRITECYCLE;		// load command, leert das pageregister
		}
	}
	STOP_WRITECYCLE;					// write command, schreibt pageregister ins flash und versetzt CPU in idle fuer 4ms
	EA=1;
}
*/

/*
void set_pa(void)
{
	while(fb_state!=0);		// warten falls noch gesendet wird
	EA=0;
	FMCON=0x00;				// load command, leert das pageregister
    FMADRH=0x1D;
    FMADRL=ADDRTAB+1;
    FMDATA=telegramm[8];
    FMDATA=telegramm[9];	// n�chstes Byte, da autoinkrement
	FMCON=0x68;				// write command, schreibt pageregister ins flash und versetzt CPU in idle fuer 4ms
	EA=1;
}
*/


/*
unsigned char read_objflags(unsigned char objno)
{
	return(eeprom[eeprom[COMMSTABPTR]+3+3*objno]);
}
*/

/*
unsigned char find_first_objno(unsigned char gah, unsigned char gal)
{
	unsigned char gaposgat, gaposass, atp, assmax, n, objno, asspos;

	objno=0xFF;
	gaposgat=gapos_in_gat(gah,gal);

	atp=eeprom[ASSOCTABPTR];	// Pointer auf Assoziationstabelle
	assmax=eeprom[atp];			// Anzahl Assoziationen
	if (gaposgat!=0xFF) {	// falls Gruppenadresse nicht vorhanden
		n=0;
		asspos=atp+1;
		while(n<assmax) { 	// Schleife ueber Assoziationstabelle
			gaposass=eeprom[asspos];
		    if(gaposgat==gaposass) {
		    	objno=eeprom[asspos+1];
		    	n=assmax;
		    }
		    n++;
		    asspos+=2;
		}
	}
	return (objno);
}
*/

/*
unsigned int find_ga(unsigned char objno)
{
	unsigned char asstab,gapos,asspos;
	unsigned int ga;

	gapos=0xFE;
	asstab=eeprom[ASSOCTABPTR];		// Adresse der Assoziationstabelle
	asspos=asstab+1+2*objno;

	if(eeprom[asspos+1]==objno) gapos=eeprom[asspos];

    if(gapos!=0xFE) {

    	ga=eeprom[ADDRTAB+1+gapos*2]*256;
    	ga+=eeprom[ADDRTAB+2+gapos*2];
    }
    else ga=0;

    return(ga);
}
*/



void restart_hw(void)
{
	DIVM=0;			// Taktferquenz nicht teilen -> volles Tempo
  
	P1M1=0x14;		// Port 1 auf quasi-bidirektional,
	P1M2=0x4C;		// ausser P1.2(T0 als PWM Ausgang)=open-drain,
					// P1.3 open drain (muss sein),
					// P1.4(INT1)=Input only, P1.6(FBOUTC) push-pull
   
	FBOUTC=0;		// Bus-Ausgang auf low
  
	TMOD=0x21;		// Timer 0 als 16-Bit Timer, Timer 1 als 8 Bit autoreload
	TAMOD=0x00;
	TR0=0;			// Timer 0 (zur Verwendung in app) zunaechst stoppen 
	TR1=0;			// Timer 1 (Empfangs-Timer, nicht in app verwenden!) zunaechst stoppen
	TH1=128;		// Timer 1 auf 104us/3
	TL1=128;
	TF1=0;
 
	RTCH=0x0E;		// Real Time Clock auf 65ms laden
	RTCL=0xA0;		// (RTC ist ein down-counter mit 128 bit prescaler und osc-clock)
	RTCCON=0x61;	// ... und starten

	interrupted=0;	// wird durch die interrupt-routine auf 1 gesetzt
	IEN0=0x00;
	IEN1=0x00;

	ET1=0;			// Interrupt von Timer 1 sperren
	EX0=0;			// Externen Interrupt 0 sperren
	EX1=0;			// Externen Interrupt 1 sperren
	EA=1;			// Interrupts prinzipiell freigeben
	
	IP0=0x04;		// h�chste Priorit�t fuer ext1
	IP0H=0x0C;
	IT1=1;			// Interrupt 1 flankengetriggert=1

	status60=0x2E;	// Status-Byte (steht normal im userram an 0x60)

	ack=0;			// ack und nack flag zur�cksetzen
	nack=0;
	send_ack=0;
	send_nack=0;
	tel_arrived=0;	// kein Telegramm empfangen
	auto_ack=1;		// empfangene Telegramme automatisch mit ack best�tigen
	tx_nextwrite=0;	// Zeiger auf n�chste zu schreibende Objektnr. in tx_buffer
	tx_nextsend=0;	// Zeiger auf n�chste zu sendende Objektnr. in tx_buffer
	pcount=0;		// Paketzaehler initialisieren
	inc_pcount=0;
	connected=0;	// keine Verbindung
	transparency=0;	// fremde Gruppentelegramme werden ignoriert
	init_rx();		// Empfang initialisieren
}
