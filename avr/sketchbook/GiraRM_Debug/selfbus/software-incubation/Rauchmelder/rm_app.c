/*
 *  Copyright (c) 2013 Stefan Taferner <stefan.taferner@gmx.at>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include "rm_app.h"
#include "rm_const.h"
#include "rm_com.h"
#include "rm_conv.h"
#include "rm_eeprom.h"

#include <mcs51/P89LPC922.h>
#include <fb_lpc922_1.4x.h>


// Befehle an den Rauchmelder
const unsigned char CmdTab[RM_CMD_COUNT] =
{
	0x04,   // RM_CMD_SERIAL
	0x09,   // RM_CMD_OPERATING_TIME
	0x0B,   // RM_CMD_SMOKEBOX
	0x0C,   // RM_CMD_BATTEMP
	0x0D,   // RM_CMD_NUM_ALARMS
	0x0E	// RM_CMD_NUM_TEST_ALARMS
};


// Mapping von den Kommunikations-Objekten auf die Rauchmelder Requests
// und die Daten in der Rauchmelder Antwort. Der Index in die Tabelle ist
// die ID vom Kommunikations-Objekt (objid).
const struct
{
	unsigned const char cmd;       // Zu sendender RM_CMD Befehl
	unsigned const char offset;    // Byte-Offset in der Antwort
	unsigned const char dataType;  // Datentyp der Antwort
} objMappingTab[NUM_OBJS] =
{
	/* 0 OBJ_ALARM_BUS*/           { RM_CMD_INTERNAL,        0, RM_TYPE_NONE },
	/* 1 OBJ_TALARM_BUS*/          { RM_CMD_INTERNAL,        0, RM_TYPE_NONE },
	/* 2 OBJ_RESET_ALARM*/         { RM_CMD_INTERNAL,        0, RM_TYPE_NONE },
	/* 3 OBJ_STAT_ALARM*/          { RM_CMD_INTERNAL,        0, RM_TYPE_NONE },
	/* 4 OBJ_STAT_ALARM_DELAYED*/  { RM_CMD_INTERNAL,        0, RM_TYPE_NONE },
	/* 5 OBJ_STAT_TALARM*/         { RM_CMD_INTERNAL,        0, RM_TYPE_NONE },
	/* 6 OBJ_SERIAL*/              { RM_CMD_SERIAL,          0, RM_TYPE_LONG },
	/* 7 OBJ_OPERATING_TIME*/      { RM_CMD_OPERATING_TIME,  0, RM_TYPE_QSEC },
	/* 8 OBJ_SMOKEBOX_VALUE*/      { RM_CMD_SMOKEBOX,        0, RM_TYPE_INT  },
	/* 9 OBJ_POLLUTION*/           { RM_CMD_SMOKEBOX,        3, RM_TYPE_BYTE },
	/*10 OBJ_BAT_VOLTAGE*/         { RM_CMD_BATTEMP,         0, RM_TYPE_VOLT },
	/*11 OBJ_TEMP*/                { RM_CMD_BATTEMP,         2, RM_TYPE_TEMP },
	/*12 OBJ_ERRCODE*/             { RM_CMD_INTERNAL,        0, RM_TYPE_NONE },
	/*13 OBJ_BAT_LOW*/             { RM_CMD_INTERNAL,        0, RM_TYPE_NONE },
	/*14 OBJ_MALFUNCTION*/         { RM_CMD_INTERNAL,        0, RM_TYPE_NONE },
	/*15 OBJ_CNT_SMOKEALARM*/      { RM_CMD_SMOKEBOX,        2, RM_TYPE_BYTE },
	/*16 OBJ_CNT_TEMPALARM*/       { RM_CMD_NUM_ALARMS,      0, RM_TYPE_BYTE },
	/*17 OBJ_CNT_TESTALARM*/       { RM_CMD_NUM_ALARMS,      1, RM_TYPE_BYTE },
	/*18 OBJ_CNT_ALARM_WIRE*/      { RM_CMD_NUM_ALARMS,      2, RM_TYPE_BYTE },
	/*19 OBJ_CNT_ALARM_BUS*/	   { RM_CMD_NUM_ALARMS,      3, RM_TYPE_BYTE },
	/*20 OBJ_CNT_TALARM_WIRE*/     { RM_CMD_NUM_ALARMS_2,    0, RM_TYPE_BYTE },
	/*21 OBJ_CNT_TALARM_BUS*/	   { RM_CMD_NUM_ALARMS_2,    1, RM_TYPE_BYTE }
};


// Flag für lokalen Alarm und Wired Alarm (über grüne Klemme / Rauchmelderbus)
__bit alarmLocal;

// Flag für remote Alarm über EIB
__bit alarmBus;

// Flag für lokalen Testalarm und Wired Testalarm
__bit testAlarmLocal;

// Flag für remote Testalarm über EIB
__bit testAlarmBus;

// Flag für den gewünschten Alarm Status wie wir ihn über den EIB empfangen haben
__bit setAlarmBus;

// Flag für den gewünschten Testalarm Status wie wir ihn über den EIB empfangen haben
__bit setTestAlarmBus;

// Flag für Bus Alarm & -Testalarm ignorieren
__bit ignoreBusAlarm;

// Rauchmelder Fehlercodes
unsigned char errCode;


// Flags für Com-Objekte lesen
unsigned char objReadReqFlags[NUM_OBJ_FLAG_BYTES];

// Flags für Com-Objekte senden
unsigned char objSendReqFlags[NUM_OBJ_FLAG_BYTES];

// Werte der Com-Objekte. Index ist die der RM_CMD
unsigned long objValues[RM_CMD_COUNT];


// Nummer des Befehls an den Rauchmelder der gerade ausgeführt wird.
// RM_CMD_NONE wenn keiner.  So lange ein RM_CMD ausgeführt wird darf auf
// objValues[cmdCurrent] nicht zugegriffen werden. Es muss stattdessen objOldValue
// verwendet werden.
unsigned char cmdCurrent;

// Backup des Com-Objekt Wertebereichs der gerade von cmdCurrent neu vom
// Rauchmelder geholt wird.
__idata unsigned long objValueCurrent;


// Zähler für die Zeit die auf eine Antwort vom Rauchmelder gewartet wird.
// Ist der Zähler 0 dann wird gerade auf keine Antwort gewartet.
unsigned char answerWait;

// Initialwert für answerWait in 0,5s
#define INITIAL_ANSWER_WAIT 6

// Zähler für keine Antwort vom Rauchmelder
unsigned char noAnswerCount;

// Maximale Anzahl in noAnswerCount ab der ein Rauchmelder Fehler gemeldet wird
#define NO_ANSWER_MAX 5


// Zähler für Alarm am JP2 - EXTRA_ALARM_PIN
unsigned char extraAlarmCounter;

// Schwelle für extraAlarmCounter in 0,5s
#define EXTRA_ALARM_LIMIT 5


// Countdown Zähler für zyklisches Senden eines Alarms oder Testalarms.
unsigned char alarmCounter;

// Countdown Zähler für zyklisches Senden des Alarm Zustands.
unsigned char alarmStatusCounter;

// Countdown Zähler für verzögertes Senden eines Alarms
unsigned char delayedAlarmCounter;

// Countdown Zähler für zyklisches Senden der (Info) Com-Objekte
unsigned char infoCounter;

// Nummer des Com-Objekts das bei zyklischem Info Senden als nächstes geprüft/gesendet wird
unsigned char infoSendObjno;

// Halbsekunden Zähler 0..119
unsigned char halfSeconds;


// Tabelle für 1<<x, d.h. pow2[3] == 1<<3
const unsigned char pow2[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };

// Im Byte Array arr das bitno-te Bit setzen
#define ARRAY_SET_BIT(arr, bitno) arr[bitno>>3] |= pow2[bitno & 7]

// Im Byte Array arr das bitno-te Bit löschen
#define ARRAY_CLEAR_BIT(arr, bitno) arr[bitno>>3] &= ~pow2[bitno & 7]

// Testen ob im Byte Array arr das bitno-te Bit gesetzt ist
#define ARRAY_IS_BIT_SET(arr, bitno) (arr[bitno>>3] & pow2[bitno & 7])


// Verwendet um Response Telegramme zu kennzeichnen.
#define OBJ_RESPONSE_FLAG 0x40


/**
 * Den Alarm Status auf den Bus senden falls noch nicht gesendet.
 *
 * @param newAlarm - neuer Alarm Status
 */
void send_obj_alarm(__bit newAlarm)
{
	if (alarmLocal != newAlarm)
	{
		send_obj_value(OBJ_ALARM_BUS);
		send_obj_value(OBJ_STAT_ALARM);
	}
}


/**
 * Den Testalarm Status auf den Bus senden falls noch nicht gesendet.
 *
 * @param newAlarm - neuer Testalarm Status
 */
void send_obj_test_alarm(__bit newAlarm)
{
	if (testAlarmLocal != newAlarm)
	{
		send_obj_value(OBJ_TALARM_BUS);
		send_obj_value(OBJ_STAT_TALARM);
	}
}


/**
 * Fehlercode setzen
 *
 * @param newErrCode - neuer Fehlercode
 */
void set_errcode(unsigned char newErrCode)
{
	if (newErrCode == errCode)
		return;

	// Wenn sich der Status der Batterie geändert hat dann OBJ_BAT_LOW senden,
	// sonst den allgemeinen Fehler Indikator OBJ_MALFUNCTION.
	if ((errCode ^ newErrCode) & ERRCODE_BATLOW)
		ARRAY_SET_BIT(objSendReqFlags, OBJ_BAT_LOW);
	else ARRAY_SET_BIT(objSendReqFlags, OBJ_MALFUNCTION);

	errCode = newErrCode;
}


/**
 * Die empfangene Nachricht vom Rauchmelder verarbeiten.
 * Wird von _receive() aufgerufen.
 */
void _process_msg(unsigned char* bytes, unsigned char len)
{
	unsigned char objno, cmd, msgType;
	unsigned char byteno, mask;

	answerWait = 0;
	if (noAnswerCount)
	{
		noAnswerCount = 0;
		set_errcode(errCode & ~ERRCODE_COMM);
	}

	msgType = bytes[0];
	if ((msgType & 0xf0) == 0xc0) // Com-Objekt Werte empfangen
	{
		msgType &= 0x0f;

		for (cmd = 0; cmd < RM_CMD_COUNT; ++cmd)
		{
			if (CmdTab[cmd] == msgType)
				break;
		}

		if (cmd < RM_CMD_COUNT)
		{
			objValueCurrent = objValues[cmd];
			cmdCurrent = cmd;
			objValues[cmd] = *(unsigned long*)(bytes + 1);
			cmdCurrent = RM_CMD_NONE;

			// Versand der erhaltenen Com-Objekte einleiten. Dazu alle Com-Objekte suchen
			// auf die die empfangenen Daten passen und diese senden. Sofern sie für
			// den Versand vorgemerkt sind.
			for (objno = 0; objno < NUM_OBJS; ++objno, mask <<= 1)
			{
				byteno = objno >> 3;
				mask = pow2[objno & 7];

				if (objReadReqFlags[byteno] & mask)
				{
					send_obj_value(objno | OBJ_RESPONSE_FLAG);
					objReadReqFlags[byteno] &= ~mask;
				}
				if (objSendReqFlags[byteno] & mask)
				{
					send_obj_value(objno);
					objSendReqFlags[byteno] &= ~mask;
				}
			}
		}
	}
	else if (msgType == 0x82 && len >= 5) // Status Meldung
	{
		unsigned char subType = bytes[1];
		__bit newAlarm;

		// (Alarm) Status

		unsigned char status = bytes[2];

		// Lokaler Alarm: Rauch Alarm | Temperatur Alarm | Wired Alarm
		newAlarm = (subType & 0x10) | (status & (0x04 | 0x08));
		send_obj_alarm(newAlarm);
		alarmLocal = newAlarm;

		// Lokaler Testalarm: (lokaler) Testalarm || Wired Testalarm
		newAlarm = status & (0x20 | 0x40);
		send_obj_test_alarm(newAlarm);
		testAlarmLocal = newAlarm;

		// Bus Alarm
		alarmBus = status & 0x10;

		// Bus Testalarm
		testAlarmBus = status & 0x80;

		// Batterie schwach/leer
		if ((status ^ errCode) & ERRCODE_BATLOW)
			set_errcode((errCode & ~ERRCODE_BATLOW) | (status & ERRCODE_BATLOW));

		if (subType & 0x08)  // Taste am Rauchmelder gedrückt
		{
			if (setAlarmBus)
			{
				setAlarmBus = 0;
				send_obj_value(OBJ_STAT_ALARM);
			}

			if (setTestAlarmBus)
			{
				setTestAlarmBus = 0;
				send_obj_value(OBJ_STAT_TALARM);
			}
		}

		if (subType & 0x02)  // Defekt am Rauchmelder
		{
			unsigned char status = bytes[4];
			unsigned char newErrCode = errCode & (ERRCODE_BATLOW | ERRCODE_COMM);

			if (status & 0x04)
				newErrCode |= ERRCODE_TEMP1;

			if (status & 0x10)
				newErrCode |= ERRCODE_TEMP2;

			// TODO Rauchkammer Defekt behandeln

			set_errcode(newErrCode);
		}
	}
}


/**
 * Empfangenes read_value_request Telegramm verarbeiten.
 */
void read_value_req(unsigned char objno)
{
	ARRAY_SET_BIT(objReadReqFlags, objno);
}


/**
 * Die Rauchmelder Antwort als Long Zahl liefern.
 *
 * @param answer - das erste Byte der Rauchmelder Antwort.
 * @return Der Wert mit getauschten Bytes.
 */
unsigned long answer_to_long(unsigned char* cvalue)
{
	union
	{
		unsigned long l;
		unsigned char c[4];
	} result;

	result.c[3] = cvalue[0];
	result.c[2] = cvalue[1];
	result.c[1] = cvalue[2];
	result.c[0] = cvalue[3];

	return result.l;
}


/**
 * Die Rauchmelder Antwort als Integer Zahl liefern.
 *
 * @param answer - das erste Byte der Rauchmelder Antwort.
 * @return Der Wert mit getauschten Bytes.
 */
unsigned int answer_to_int(unsigned char* cvalue)
{
	union
	{
		unsigned int i;
		unsigned char c[2];
	} result;

	result.c[1] = cvalue[0];
	result.c[0] = cvalue[1];

	return result.i;
}


/**
 * Wert eines Com-Objekts liefern.
 *
 * @param objno - die ID des Kommunikations-Objekts
 * @return Den Wert des Kommunikations Objekts
 */
unsigned long read_obj_value(unsigned char objno)
{
	unsigned char cmd = objMappingTab[objno].cmd;

//	DEBUG_WRITE_BYTE(objno);
//	DEBUG_WRITE_BYTE(eeprom[COMMSTABPTR]);
//	//DEBUG_WRITE_BYTE(eeprom[eeprom[COMMSTABPTR]+objno*3+4]); // objtype

	// Interne Com-Objekte behandeln
	if (cmd == RM_CMD_INTERNAL)
	{
		switch (objno)
		{
		case OBJ_ALARM_BUS:
		case OBJ_STAT_ALARM:
			return alarmLocal;

		case OBJ_TALARM_BUS:
		case OBJ_STAT_TALARM:
			return testAlarmLocal;

		case OBJ_RESET_ALARM:
			return ignoreBusAlarm;

		case OBJ_STAT_ALARM_DELAYED:
			return delayedAlarmCounter != 0;

		case OBJ_BAT_LOW:
			return (errCode & ERRCODE_BATLOW) != 0;

		case OBJ_MALFUNCTION:
			return (errCode & ~ERRCODE_BATLOW) != 0;

		case OBJ_ERRCODE:
			return errCode;
		}
	}
	// Com-Objekte verarbeiten die Werte vom Rauchmelder darstellen
	else if (cmd != RM_CMD_NONE)
	{
		unsigned long lval;
		unsigned char* answer;

		if (cmd == cmdCurrent) answer = (unsigned char*) &objValueCurrent;
		else answer = (unsigned char*) &objValues[cmd];
		answer += objMappingTab[objno].offset;

		switch (objMappingTab[objno].dataType)
		{
		case RM_TYPE_BYTE:
			return *answer;

		case RM_TYPE_LONG:
			return answer_to_long(answer);

		case RM_TYPE_QSEC:
			return answer_to_long(answer) >> 2;

		case RM_TYPE_INT:
			return answer_to_int(answer);

		case RM_TYPE_TEMP:
			lval = answer[0] > answer[1] ? answer[0] : answer[1];
			lval *= 50;
			lval -= 2000;
			return conv_dpt_9_001(lval);

		case RM_TYPE_VOLT:
			lval = answer_to_int(answer);
			lval *= 9184;
			lval /= 5000;
			return conv_dpt_9_001(lval);

		default: // Fehler: unbekannter Datentyp
			return -2;
		}
	}

	// Fehler: unbekanntes Com Objekt
	return -1;
}


/**
 * Empfangenes write_value_request Telegramm verarbeiten
 *
 * @param objno - Nummer des betroffenen Kommunikations-Objekts
 */
void write_value_req(unsigned char objno)
{
 	if (objno == OBJ_ALARM_BUS) // Bus Alarm
	{
 		setAlarmBus = telegramm[7] & 0x01;

 		// Wenn wir lokalen Alarm haben dann Bus Alarm wieder auslösen
		// damit der Status der anderen Rauchmelder stimmt
 		if (!setAlarmBus && alarmLocal)
 			send_obj_value(OBJ_ALARM_BUS);

 		if (ignoreBusAlarm)
 			setAlarmBus = 0;
	}
	else if (objno == OBJ_TALARM_BUS) // Bus Test Alarm
	{
		setTestAlarmBus = telegramm[7] & 0x01;

 		// Wenn wir lokalen Testalarm haben dann Bus Testalarm wieder auslösen
		// damit der Status der anderen Rauchmelder stimmt
 		if (!setTestAlarmBus && testAlarmLocal)
 			send_obj_value(OBJ_TALARM_BUS);

 		if (ignoreBusAlarm)
 			setTestAlarmBus = 0;
 	}
	else if (objno == OBJ_RESET_ALARM) // Bus Alarm rücksetzen
	{
		setAlarmBus = 0;
		setTestAlarmBus = 0;
		ignoreBusAlarm = 1;
	}
}

/**
 * Ein Com-Objekt bearbeiten.
 *
 * @param objno - die Nummer des zu bearbeitenden Com Objekts
 */
void process_obj(unsigned char objno)
{
	unsigned char cmd = objMappingTab[objno].cmd;

	if (cmd == RM_CMD_NONE || cmd == RM_CMD_INTERNAL)
	{
		// Der Wert des Com-Objekts ist bekannt, also sofort senden

		unsigned char byteno = objno >> 3;
		unsigned char mask = pow2[objno & 7];

		if (objReadReqFlags[byteno] & mask)
		{
			send_obj_value(objno | OBJ_RESPONSE_FLAG);
			objReadReqFlags[byteno] &= ~mask;
		}
		if (objSendReqFlags[byteno] & mask)
		{
			send_obj_value(objno);
			objSendReqFlags[byteno] &= ~mask;
		}
	}
	else
	{
		// Den Wert des Com-Objekts vom Rauchmelder anfordern. Der Versand erfolgt
		// wenn die Antwort vom Rauchmelder erhalten wurde, in _process_msg().
		if (recvCount < 0)
		{
			_send_cmd(CmdTab[cmd]);
			answerWait = INITIAL_ANSWER_WAIT;
		}
	}
}


/**
 * Com-Objekte bearbeiten, Worker Funktion.
 *
 * Com-Objekte, die Daten vom Rauchmelder benötigen, werden nur bearbeitet wenn
 * nicht gerade auf Antwort vom Rauchmelder gewartet wird.
 *
 * @return 1 wenn ein Com-Objekt verarbeitet wurde, sonst 0.
 */
unsigned char do_process_objs(unsigned char *flags)
{
	unsigned char byteno, bitno, objno, cmd, flagsByte;

	for (byteno = 0; byteno < NUM_OBJ_FLAG_BYTES; ++byteno)
	{
		flagsByte = flags[byteno];
		if (!flagsByte) continue;

		for (bitno = 0; bitno < 8; ++bitno)
		{
			if (flagsByte & pow2[bitno])
			{
				objno = (byteno << 3) + bitno;
				cmd = objMappingTab[objno].cmd;
				if (!answerWait || cmd == RM_CMD_NONE || cmd == RM_CMD_INTERNAL)
				{
					process_obj(objno);
					return 1;
				}
			}
		}
	}

#ifdef OLD_CODE
	for (objno = 0; objno < NUM_OBJS; ++objno)
	{
		byteno = objno >> 3;
		mask = pow2[objno & 7];

		if (flags[byteno] & mask)
		{
			unsigned char cmd = objMappingTab[objno].cmd;
			if (!answerWait || cmd == RM_CMD_NONE || cmd == RM_CMD_INTERNAL)
			{
				process_obj(objno);
				return 1;
			}
		}
	}
#endif /*OLD_CODE*/

	return 0;
}

/**
 * Com-Objekte bearbeiten.
 */
void process_objs()
{
	if (do_process_objs(objReadReqFlags))
		return;

	do_process_objs(objSendReqFlags);
}


/**
 * Den Zustand der Alarme bearbeiten. Wenn wir der Meinung sind der Bus-Alarm soll einen
 * bestimmten Zustand haben dann wird das dem Rauchmelder so lange gesagt bis der auch
 * der gleichen Meinung ist.
 */
void process_alarm_stats()
{
	if (setAlarmBus && !alarmBus)
	{
		// Alarm auslösen
		_send_hexstr("030210");
		answerWait = INITIAL_ANSWER_WAIT;
	}
	else if (setTestAlarmBus && !testAlarmBus)
	{
		// Testalarm auslösen
		_send_hexstr("030280");
		answerWait = INITIAL_ANSWER_WAIT;
	}
	else if ((!setAlarmBus && alarmBus) || (!setTestAlarmBus && testAlarmBus))
	{
		// Alarm und Testalarm beenden
		_send_hexstr("030200");
		answerWait = INITIAL_ANSWER_WAIT;
	}
}


/**
 * Timer Event.
 */
void timer_event()
{
	RTCCON = 0x60;  // RTC anhalten und Flag löschen
	RTCH = 0x70;    // RTC neu laden (1s = 0xE100; 0,5s = 0x7080; 0,25s = 0x3840)
	RTCL = 0x80;
	RTCCON = 0x61;  // RTC starten

	--halfSeconds;

	// Wir warten auf eine Antwort vom Rauchmelder
	if (answerWait)
	{
		--answerWait;

		// Wenn keine Antwort vom Rauchmelder kommt dann den noAnswerCount Zähler
		// erhöhen. Wenn der Zähler NO_ANSWER_MAX erreicht dann ist es ein Rauchmelder
		// Fehler.
		if (!answerWait && noAnswerCount < 255)
		{
			++noAnswerCount;
			if (noAnswerCount >= NO_ANSWER_MAX)
			{
				set_errcode(errCode | ERRCODE_COMM);
			}
		}
	}

	// Alles danach wird nur jede Sekunde gemacht
	if (halfSeconds & 1)
		return;

	// Alarm: verzögert und zyklisch senden
	if (alarmLocal)
	{
		// Alarm verzögert senden
		if (delayedAlarmCounter)
		{
			--delayedAlarmCounter;
			if (!delayedAlarmCounter)
			{
				ARRAY_SET_BIT(objSendReqFlags, OBJ_ALARM_BUS);
				ARRAY_SET_BIT(objSendReqFlags, OBJ_STAT_ALARM);
			}
		}
		else // Alarm zyklisch senden
		{
			if (eeprom[CONF_SEND_ENABLE] & CONF_ENABLE_ALARM_INTERVAL)
			{
				--alarmCounter;
				if (!alarmCounter)
				{
					alarmCounter = eeprom[CONF_ALARM_INTERVAL];
					ARRAY_SET_BIT(objSendReqFlags, OBJ_STAT_ALARM);
				}
			}
		}
	}

	// Testalarm: zyklisch senden
	if (testAlarmLocal)
	{
		if (eeprom[CONF_SEND_ENABLE] & CONF_ENABLE_TALARM_INTERVAL)
		{
			--alarmCounter;
			if (!alarmCounter)
			{
				alarmCounter = eeprom[CONF_TALARM_INTERVAL];
				ARRAY_SET_BIT(objSendReqFlags, OBJ_STAT_TALARM);
			}
		}
	}

	// Jede Sekunde ein Status Com-Objekt senden.
	// Aber nur senden wenn kein Alarm anliegt.
	if (infoSendObjno && !(alarmLocal | alarmBus | testAlarmLocal | testAlarmBus))
	{
		// Info Objekt zum Senden vormerken wenn es dafür konfiguriert ist.
		if ((infoSendObjno >= 14 && (eeprom[CONF_INFO_14TO21] & pow2[infoSendObjno - 14])) ||
			(infoSendObjno >= 6 && (eeprom[CONF_INFO_6TO13] & pow2[infoSendObjno - 6])))
		{
			ARRAY_SET_BIT(objSendReqFlags, infoSendObjno);
		}

		--infoSendObjno;
	}

	if (!halfSeconds) // einmal pro Minute
	{
		halfSeconds = 120;

		// Bus Alarm ignorieren Flag rücksetzen wenn kein Alarm mehr anliegt
		if (ignoreBusAlarm & !(alarmBus | testAlarmBus))
			ignoreBusAlarm = 0;

		// Status Informationen zyklisch senden
		if (eeprom[CONF_SEND_ENABLE] & CONF_ENABLE_INFO_INTERVAL)
		{
			--infoCounter;
			if (!infoCounter)
			{
				infoCounter = eeprom[CONF_INFO_INTERVAL];
				infoSendObjno = OBJ_HIGH_INFO_SEND;
			}
		}
	}
}


/**
 * Alle Applikations-Parameter zurücksetzen
 */
void restart_app()
{
	unsigned char i;

	P0M1 = 0x03;   // P0.0 and P0.1 open drain. all other pins of P0 as bidirectional
	P0M2 = 0x03;
	P0 = ~0x04;	   // P0.2 low to enable serial communication. all other pins of p0 high

//	P1M1 |= (1 << 2); // P1.2 to Rauchmelder open drain with external pullup
//	P1M2 |= (1 << 2);
//	P1 |= (1 << 2);	  // P1.2 high

	_init();

	RTCH = 0x70;	// Reload Real Time Clock (1s = 0xE100; 0,5s = 0x7080; 0,25s = 0x3840)
	RTCL = 0x80;	// (RTC ist ein down-counter mit 128 bit prescaler und osc-clock)
	RTCCON = 0x61;	// ... und starten

	// Werte initialisieren

	for (i = 0; i < NUM_OBJ_FLAG_BYTES; ++i)
	{
		objReadReqFlags[i] = 0;
		objSendReqFlags[i] = 0;
	}

	answerWait = 0;
	noAnswerCount = 0;
	cmdCurrent = RM_CMD_NONE;
	recvCount = -1;
	halfSeconds = eeprom[ADDRTAB + 2] & 127;

	alarmBus = 0;
	alarmLocal = 0;

	testAlarmBus = 0;
	testAlarmLocal = 0;

	setAlarmBus = 0;
	setTestAlarmBus = 0;
	ignoreBusAlarm = 0;

	infoSendObjno = 0;
	infoCounter = 1;
	alarmCounter = 1;
	alarmStatusCounter = 1;
	delayedAlarmCounter = 0;
	extraAlarmCounter = 0;

	errCode = 0;

	// EEPROM initialisieren

	EA = 0;							// Interrupts sperren, damit Flashen nicht unterbrochen wird
	START_WRITECYCLE;
	WRITE_BYTE(0x01, 0x03, 0x00);	// Herstellercode 0x004C = Bosch
	WRITE_BYTE(0x01, 0x04, 0x4C);
//	WRITE_BYTE(0x01, 0x05, 0x03);	// Devicetype 1010 (0x03F2)
//	WRITE_BYTE(0x01, 0x06, 0xF2);
//	WRITE_BYTE(0x01, 0x07, 0x21);	// Version der Applikation: 2.1
	WRITE_BYTE(0x01, 0x0C, 0x00);	// PORT A Direction Bit Setting
	WRITE_BYTE(0x01, 0x0D, 0xFF);	// Run-Status (00=stop FF=run)
//	WRITE_BYTE(0x01, 0x12, 0xA0);	// COMMSTAB Pointer
	STOP_WRITECYCLE;
	EA = 1;							// Interrupts freigeben

	// init reload value and prescaler
	// select Watchdog clock at 400kHz
	// start watchdog
	WDL = 0xFF;
	EA = 0;
	WDCON = 0xE5;
	WFEED1 = 0xA5;
	WFEED2 = 0x5A;
	EA = 1;

	_send_byte(ACK);
	_send_byte(ACK);

	// TODO Alarm-Status vom Rauchmelder abfragen
}
