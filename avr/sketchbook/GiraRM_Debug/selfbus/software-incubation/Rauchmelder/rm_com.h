/*
 *  Copyright (c) 2013 Stefan Taferner <stefan.taferner@gmx.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#ifndef rm_com_h
#define rm_com_h

//
// Funktionen für die Kommunikation mit dem Rauchmelder
//

/**
 * Ein Byte über die serielle Schnittstelle vom Rauchmelder empfangen.
 *
 * Wenn die empfangene Nachricht komplett ist dann wird _process_msg()
 * zur Verarbeitung der Nachricht aufgerufen. Diese Funktion muss aus main()
 * aufgerufen werden wenn ein Byte empfangen wurde.
 */
extern void _recv_byte();

/**
 * Die empfangene Nachricht vom Rauchmelder verarbeiten.
 *
 * Wird von _receive() aufgerufen wenn eine Nachricht empfangen wurde.
 * Muss vom Verwender implementiert werden.
 *
 * @param bytes - die empfangene Nachricht, ohne STX, ETX, Prüfsumme.
 * @param len - die Anzahl der empfangenen Bytes
 */
extern void _process_msg(unsigned char* bytes, unsigned char len);

/**
 * Serielle Kommunikation mit dem Rauchmelder initialisieren
 */
extern void _init();

/**
 * Einen 1 Byte Befehl an den Rauchmelder senden.
 *
 * @param cmd - das Befehls-Byte.
 */
extern void _send_cmd(unsigned char cmd);

/**
 * Ein Byte an den Rauchmelder senden.
 *
 * @param ch - das zu sendende Byte.
 */
extern void _send_byte(unsigned char ch);

/**
 * Eine Nachricht an den Rauchmelder senden.
 *
 * Der Befehl wird als Hex String gesendet. Die Prüfsumme wird automatisch
 * berechnet und nach dem Befehl gesendet. Die gesammte Übertragung wird mit
 * STX begonnnen und mit ETX beendet.
 *
 * @param hexstr - die zu sendenden Bytes als Hex String, mit Nullbyte am Ende
 */
extern void _send_hexstr(unsigned char* hexstr);

/**
 * Wenn >= 0 dann wird gerade etwas vom Rauchmelder empfangen
 */
extern char recvCount;


#endif /*rm_com_h*/
