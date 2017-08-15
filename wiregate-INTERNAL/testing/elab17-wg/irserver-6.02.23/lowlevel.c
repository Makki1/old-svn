/*
 * Copyright (c) 2007, IRTrans GmbH
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer. 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution. 
 *     * Neither the name of IRTrans GmbH nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY IRTrans GmbH ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL IRTrans GmbH BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */



#ifdef WIN32
#include "winsock2.h"
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>

#define MSG_NOSIGNAL	0
#endif

#ifdef WINCE
#include "winsock2.h"
#include <windows.h>
#include <time.h>
#endif

#include <stdio.h>

#ifdef LINUX
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <sys/time.h>

typedef int DWORD;
#define closesocket		close
#endif


#include "remote.h"
#include "errcode.h"
#include "network.h"
#include "dbstruct.h"
#include "lowlevel.h"
#include "fileio.h"
#include "global.h"
#include "flash.h"

#ifdef WIN32
#include "winio.h"
#include "winusbio.h"
BOOL WINAPI ShutdownHandler (DWORD type);
extern WSAEVENT IrtLanEvent;
#endif

#define RESEND_WAIT_SER		5
#define RESEND_WAIT_USB		10


byte byteorder;
extern STATUS_BUFFER remote_statusex[MAX_IR_DEVICES];
extern byte new_lcd_flag;
extern int display_bus;
extern SOCKET irtlan_socket;
int		OpenIRTransLANSocket (void);
int OpenIRTransBroadcastSockets (void);
void DecodeFunctions (DEVICEINFO *dev);
extern SOCKET irtlan_outbound;
extern int protocol_version;
extern char pidfile[256];

byte DispConvTable[TABLE_CNT][256];
byte rcmmflag;

#define FN_IR		1
#define	FN_SBUS		2
#define FN_SER		4
#define FN_USB		8
#define FN_POWERON	16
#define FN_B_O		32
#define FN_TEMP		64
#define FN_SOFTID	128
#define FN_EEPROM	256
#define FN_TRANSL	512
#define FN_HWCARR	1024
#define FN_DUALRCV	2048

void	CopyStatusBuffer (STATUS_LINE *target,STATUS_LINE *source,int flag,int bus);
int		IRTransLanFlash (DEVICEINFO *dev,IRDATA_LAN_FLASH *ird,int len,uint32_t ip);
void	SwapWordN (word *pnt);
void	SwapIntN (int32_t *pnt);

extern IRCOMMAND *cmd_pnt;



int OpenComFiles (void)
{
	FILE *fp;
	int i,bus;
	char *pnt;
	char st[255];

	i = 0;
	while (virt_comfiles[i][0]) {

		strtok (virt_comfiles[i],":");

		pnt = strtok (NULL,":");

		if (pnt) bus = atoi (pnt);
		else bus = 0;

#ifdef LINUX
		fp = fopen (virt_comfiles[i],"a+");
#else
		fp = fopen (virt_comfiles[i],"a+b");
#endif
		IRDevices[bus].comport_file = fp;
		if (fp == NULL) {
			sprintf (st,"Error opening COM Port redirection %s\n",virt_comfiles[i]);
			log_print (st,LOG_FATAL);
			return (1);
		}
		else {
			sprintf (st,"COM Port redirection %s opened for Bus %d\n",virt_comfiles[i],bus);
			log_print (st,LOG_DEBUG);
		}

		i++;
	}

	return (0);
}

int OpenVirtualComPorts (void)
{
	int res,i,bus;
	char *pnt;
	char st[255];

	for (i=0;i < MAX_IR_DEVICES;i++) IRDevices[i].virtual_comport = INVALID_HANDLE_VALUE;

	i = 0;
	while (virt_comnames[i][0]) {

		strtok (virt_comnames[i],":");

		pnt = strtok (NULL,":");

		if (pnt) bus = atoi (pnt);
		else bus = 0;

		res = OpenVirtualComport (virt_comnames[i],&IRDevices[bus].virtual_comport); 
		if (res) {
			sprintf (st,"Error opening virtual COM Port %s\n",virt_comnames[i]);
			log_print (st,LOG_FATAL);
			return (res);
		}
		else {

#ifdef WIN32
			IRDevices[bus].com_event = CreateEvent (NULL,TRUE,FALSE,NULL);
#endif

			sprintf (st,"Virtual COM Port %s opened for Bus %d\n",virt_comnames[i],bus);
			log_print (st,LOG_DEBUG);
		}

		i++;
	}

	return (0);
}

int StoreTimerEntry (int bus,TIMERCOMMAND *tim)
{
	int res;
	TIMER_ENTRY_STORE te;

	memset (&te,0,sizeof (te));

	te.len = sizeof (te);
	te.command = STORE_TIMER_PARAMETER;
	te.mode = tim->num & 3;
	te.tim.mode = tim->tim_mode;
	te.tim.status = tim->status;
	te.tim.year = tim->year;
	te.tim.month = tim->month;
	te.tim.day = tim->day;
	te.tim.hour = tim->hour;
	te.tim.min = tim->min;
	te.tim.weekday = tim->weekday;
	memcpy (te.remote,tim->remote,81);
	memcpy (te.ircommand,tim->ircommand,21);
	te.ledselect = tim->ledselect;
	te.targetmask = tim->targetmask;

	res = WriteTransceiverEx (IRDevices + bus,(IRDATA *)&te);
	if (res) return (res);
	return (0);
}


int	SendSerialBlock (int bus,byte data[],byte len,byte param)
{
	int i,res;
	RS232_DATA rs232;
	IRDATA_LAN_FLASH irdf;


	if (IRDevices[bus].io.if_type == IF_LAN && (IRDevices[bus].fw_capabilities & FN_SER) && !(IRDevices[bus].fw_capabilities2 & FN2_AUX_RS232)) {
		irdf.adr = 0;
		irdf.len = len;
		SwapWordN (&irdf.len);

		memcpy (irdf.data,data,len);
		irdf.netcommand = COMMAND_SEND_RS232;
	#ifdef WIN32
		IRTransLanFlash (IRDevices+bus,&irdf,len,IRDevices[bus].io.IPAddr[0].sin_addr.S_un.S_addr);
	#else
		IRTransLanFlash (IRDevices+bus,&irdf,len,IRDevices[bus].io.IPAddr[0].sin_addr.s_addr);
	#endif
		msSleep (30);

		return (0);
	}
	
	if ((IRDevices[bus].fw_capabilities2 & FN2_RS232_IO_SBUS) || (IRDevices[bus].fw_capabilities2 & FN2_RS232_OUT_IR) || (IRDevices[bus].fw_capabilities2 & FN2_AUX_RS232)) {
		memset (&rs232,0,sizeof (RS232_DATA));
		
		rs232.command = HOST_SEND_RS232;
		rs232.len = len + 5;
		rs232.parameter = param;
		memcpy (rs232.data,data,len);

		if (bus == 0xffff) for (i=0;i<device_cnt;i++) {
			res = WriteTransceiverEx (&IRDevices[i],(IRDATA *)&rs232);
		}
		else res = WriteTransceiverEx (&IRDevices[bus],(IRDATA *)&rs232);

		return (res);
	}

	return (ERR_NO_RS232);
}


void GetBusList (REMOTEBUFFER *buf,int offset)
{
	int i;
	BUSLINE *bus;
	memset (buf,0,sizeof (REMOTEBUFFER));
	buf->statustype = STATUS_BUSLIST;
	buf->statuslen = sizeof (REMOTEBUFFER);
	buf->offset = (short)offset;

	i = 0;
	while (i < 40 && offset < device_cnt) {
		bus = (BUSLINE *)(buf->remotes + i);
		memset (bus->name,0,68);
		sprintf (bus->name,"%3d: %s [%s]",offset,IRDevices[offset].name,IRDevices[offset].device_node);
		memcpy (bus->version,IRDevices[offset].version,8);
		bus->capabilities = IRDevices[offset].fw_capabilities;
		bus->capabilities2 = IRDevices[offset].fw_capabilities2;
		bus->capabilities3 = IRDevices[offset].fw_capabilities3;
		bus->capabilities4 = IRDevices[offset].fw_capabilities4;
		bus->extended_mode = IRDevices[offset].extended_mode;
		bus->extended_mode2 = IRDevices[offset].extended_mode2;
		bus->extended_mode_ex[0] = IRDevices[offset].extended_mode_ex[0];
		bus->extended_mode_ex[1] = IRDevices[offset].extended_mode_ex[1];
		i++;
		offset++;
	}

	buf->count_buffer = i;
	buf->count_total = (word)device_cnt;
	if (i == 40) buf->count_remaining = (short)(device_cnt - offset);
	else buf->count_remaining = 0;
}




int SetTransceiverModusEx2 (int bus,byte addr,char *hotcode,int hotlen)
{
	MODE_BUFFER md;
	byte res;
	md.sbus_command = HOST_SET_MODE2;
	md.sbus_address = 0;
	if (addr <= 15)  md.sbus_address |= addr;
	if (addr == 'L') md.sbus_address |= ADRESS_LOCAL;
	if (addr == '*') md.sbus_address |= ADRESS_ALL;
	md.hotcode_len = hotlen;
	memcpy (md.hotcode,hotcode,hotlen);

	md.sbus_len = sizeof (MODE_BUFFER) + md.hotcode_len - CODE_LENRAW;

	res = WriteTransceiverEx (IRDevices + bus,(IRDATA *)&md);
	if (res) return (res);

	return (0);
}

int SetPowerLED (int bus,byte mode,byte val)
{
	byte res;
	IRDATA ird;

	ird.command = SET_POWER_LED;
	ird.len = 6;
	ird.address = mode;
	ird.target_mask = val;

	res = WriteTransceiverEx (IRDevices + bus,&ird);
	if (res) return (res);

	return (0);

}

int SetTransceiverModusEx (int bus,byte mode,word send_mask,byte addr,char *hotcode,int hotlen,byte extended_mode,byte extended_mode2,byte extended_mode_ex[],byte *mac)
{
	MODE_BUFFER md;
	byte res,offset;

	memset (&md,0,sizeof (MODE_BUFFER));

	md.sbus_command = HOST_SETMODE;
	md.sbus_address = 0;
	if (addr <= 15)  md.sbus_address |= addr;
	if (addr == 'L') md.sbus_address |= ADRESS_LOCAL;
	if (addr == '*') md.sbus_address |= ADRESS_ALL;

	md.hotcode_len = hotlen;
	memcpy (md.hotcode,hotcode,hotlen);

	md.hotcode[hotlen] = extended_mode;
	md.hotcode[hotlen+1] = extended_mode2;
	if (extended_mode_ex) {
		md.hotcode[hotlen+2] = extended_mode_ex[0];
		md.hotcode[hotlen+3] = extended_mode_ex[1];
	}
	offset = 4;

	if (strcmp (IRDevices[bus].version+1,"5.20.01") >= 0) {
		if (extended_mode_ex) {
			md.hotcode[hotlen+4] = extended_mode_ex[2];
			md.hotcode[hotlen+5] = extended_mode_ex[3];
			md.hotcode[hotlen+6] = extended_mode_ex[4];
			md.hotcode[hotlen+7] = extended_mode_ex[5];
			md.hotcode[hotlen+8] = extended_mode_ex[6];
			md.hotcode[hotlen+9] = extended_mode_ex[7];
		}
		offset += 6;
	}

	IRDevices[bus].extended_mode = extended_mode;
	IRDevices[bus].extended_mode2 = extended_mode2;
	IRDevices[bus].extended_mode_ex[0] = extended_mode_ex[0];
	IRDevices[bus].extended_mode_ex[1] = extended_mode_ex[1];

	md.sbus_len = sizeof (MODE_BUFFER) + md.hotcode_len - (CODE_LENRAW - offset);

	if (mac) {
		memcpy (md.hotcode+hotlen+offset,mac,6);
		md.sbus_len += 6;
	}

	md.mode = mode;
	md.target_mask = send_mask;

	res = WriteTransceiverEx (IRDevices + bus,(IRDATA *)&md);
	if (res) return (res);

	return (0);
}

int rcv_status_timeout (int timeout,uint32_t ip);

int TransferFlashdataEx (int bus,word data[],int adr,int len,byte active,int iradr)
{
	IRDATA md;
	int res,cnt;
	int sl;
	unsigned int cap;

	if (bus > MAX_IR_DEVICES) bus = 0;						// Fix für Kompatibilität mit Clients < V5.0
	if (iradr == IRDevices[bus].my_addr) cap = IRDevices[bus].fw_capabilities;
	else cap = remote_statusex[bus].stat[iradr].capabilities;

	if ((cap & FN_EEPROM) == 0) {
		fprintf (stderr,"No IRTrans with EEPROM connected\n\n");
		return (-1);
	}

	if (len > 128) {
		fprintf (stderr,"Maximum Transfer length is 128 Bytes [%d]\n\n",len);
		return (-1);
	}

	md.command = SET_TRANSLATE_DATA;
	md.address = (byte)(iradr & 0xff);
	md.target_mask = adr;
	md.ir_length = len;
	md.mode = active;
	md.len = (sizeof (IRDATA) - CODE_LEN) + md.ir_length;

	memcpy (md.data,data,len);

	if (time_len != TIME_LEN) ConvertToIRTRANS3 (&md);

	if (IRDevices[bus].io.if_type == IF_LAN) {
		res = cnt = 0;
		while (res != COMMAND_FLASH_ACK && res != COMMAND_FLASH_ACK_1 && res != COMMAND_FLASH_ACK_2 && cnt < 10) {	
			res = WriteTransceiverEx (IRDevices + bus,&md);
			if (res) return (res);
#ifdef WIN32
			res = rcv_status_timeout (500,IRDevices[bus].io.IPAddr[0].sin_addr.S_un.S_addr);
#else
			res = rcv_status_timeout (500,IRDevices[bus].io.IPAddr[0].sin_addr.s_addr);
#endif
			cnt++;
		}
		if (cnt == 10) fprintf (stderr,"Flash Error !\n");
	}

	else {
		res = WriteTransceiverEx (IRDevices + bus,&md);
		if (res) return (res);

		if ((iradr & 0xff) != IRDevices[bus].my_addr) {
			if (remote_statusex[bus].stat[iradr & 0xff].device_mode & DEVMODE_SBUS_UART) {
				sl = 250;
				if ((remote_statusex[bus].stat[iradr & 0xff].extended_mode_ex[2] & SBUS_BAUD_MASK) == SBUS_BAUD_4800) sl = 400;
				else if ((remote_statusex[bus].stat[iradr & 0xff].extended_mode_ex[2] & SBUS_BAUD_MASK) == SBUS_BAUD_19200) sl = 150;
				else if ((remote_statusex[bus].stat[iradr & 0xff].extended_mode_ex[2] & SBUS_BAUD_MASK) == SBUS_BAUD_38400) sl = 80;
			}
			else sl = 400;
			msSleep (sl);
		}
	}



	return (0);
}

int ReadFlashdataEx (int bus,int adr)
{
	IRDATA md;
	byte res;
	int offset = 0,len = 128;
	byte databuffer[132];


	if ((IRDevices[bus].fw_capabilities  & FN_EEPROM) == 0) {
		fprintf (stderr,"No IRTrans with EEPROM connected\n\n");
		return (-1);
	}


	md.command = 202;
	md.address = 0;
	md.target_mask = adr / 2;
	md.ir_length = len;
	md.len = 20;

	res = WriteTransceiverEx (IRDevices + bus,&md);
	if (res) return (res);

	msSleep (500);
	offset = 2;
	len = recv (irtlan_socket,databuffer,129,0);
//	res = ReadIRStringEx (IRDevices+bus,databuffer,len,2000);
//	if (res != len) return (-1);

	printf ("%04x  ",0);
	for (res=0;res < len;res++) {
		printf ("%02x ",databuffer[res+offset]);
		if (!((res + 1) % 16)) {
			printf ("\n");
			if ((res + 1) < len) printf ("%04x  ",res+1+adr);
		}
	}
	printf ("\n\n");

	return (0);
}


byte crc;

void crc_1wire (byte val)
{
	byte i,fb;

	for (i=0;i <= 7;i++) {
		fb = (crc & 1) ^ (val & 1);

		crc = crc >> 1;

		if (fb == 1) crc = crc ^ 0x8c;
		val = val >> 1;
	}
}


int ReadAnalogInputs (int bus,byte mask,ANALOG_INPUTS *inputs)

{
	IRDATA md;
	int res,i;
	GET_ANALOG_INPUTS ai;
	struct sockaddr_in send_adr;

#ifdef LINUX
	fd_set events;
	int maxfd,wait;
	struct timeval tv;
#endif

	md.command = READ_ANALOG_INPUTS;
	md.address = mask;

	md.len = 4;
	res = WriteTransceiverEx (IRDevices + bus,&md);
	if (res) return (res);


	if (IRDevices[bus].io.if_type == IF_LAN) {
#ifdef WIN32
		res = WaitForSingleObject (IrtLanEvent,2000);
		if (res == WAIT_TIMEOUT)  return (ERR_TIMEOUT);
		WSAResetEvent (IrtLanEvent);
#endif

#ifdef LINUX
		FD_ZERO (&events);

		FD_SET (irtlan_socket,&events);
		maxfd = irtlan_socket + 1;

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		wait = select (maxfd,&events,NULL,NULL,&tv);
		if (!wait) return (ERR_TIMEOUT);
#endif
		i = sizeof (send_adr);
		res = recvfrom(irtlan_socket,(byte *)&ai,sizeof (GET_ANALOG_INPUTS),MSG_NOSIGNAL,(struct sockaddr *)&send_adr,&i);
		if (res <= 0 || res != ai.ai.len + 1 || ai.netcommand != RESULT_ANALOG_INPUT) return (ERR_TIMEOUT);

	}

	else {
		res = ReadIRStringEx (IRDevices,(byte *)&(ai.ai),sizeof (ANALOG_INPUTS),500);
		if (res != sizeof (ANALOG_INPUTS)) return (ERR_TIMEOUT);
	}

	memcpy (inputs,&(ai.ai),sizeof (ANALOG_INPUTS));

	return (0);
}


/*
int ReadAnalogInputs (int bus,byte mask,ANALOG_DATA *inputs)
{
	IRRAW md;
	int res,i;
	GET_ANALOG_DATA ad;
	struct sockaddr_in send_adr;

#ifdef LINUX
	fd_set events;
	int maxfd,wait;
	struct timeval tv;
#endif

	md.command = READ_ANALOG_INPUTS;
	md.address = mask;

	md.len = 18;

	md.address = 64;				// Addresse / modus
	md.ir_length = 1;				// Modus One Wire Temp Read


	((byte *)md.data)[0] = 0x10;
	((byte *)md.data)[1] = 0x05;
	((byte *)md.data)[2] = 0xcc;
	((byte *)md.data)[3] = 0x71;
	((byte *)md.data)[4] = 0x01;
	((byte *)md.data)[5] = 0x08;
	((byte *)md.data)[6] = 0x00;
	((byte *)md.data)[7] = 0xe9;

	res = WriteTransceiverEx (IRDevices + bus,(IRDATA *)&md);
	if (res) return (res);


	if (IRDevices[bus].io.if_type == IF_LAN) {
#ifdef WIN32
		res = WaitForSingleObject (IrtLanEvent,2000);
		if (res == WAIT_TIMEOUT)  return (ERR_TIMEOUT);
		WSAResetEvent (IrtLanEvent);
#endif

#ifdef LINUX
		FD_ZERO (&events);

		FD_SET (irtlan_socket,&events);
		maxfd = irtlan_socket + 1;

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		wait = select (maxfd,&events,NULL,NULL,&tv);
		if (!wait) return (ERR_TIMEOUT);
#endif
		i = sizeof (send_adr);
		res = recvfrom(irtlan_socket,(byte *)&ad,sizeof (GET_ANALOG_DATA),MSG_NOSIGNAL,(struct sockaddr *)&send_adr,&i);
		if (res <= 0 || res != ad.ad.a_input.len + 1 || ad.netcommand != RESULT_ANALOG_INPUT) return (ERR_TIMEOUT);

		for (res=0;res <= 15;res++) {
			if (ad.ad.a_info.mode[res] == 3) {
				printf ("Mode[%d]: %d:  ",res,ad.ad.a_info.mode[res]);

				crc = 0;
				for (i=0;i <= 7;i++) {
					crc_1wire (ad.ad.a_info.id[res][i]);
					printf ("%02x.",ad.ad.a_info.id[res][i]);
				}
				printf ("%d\n",crc);
			}
		}
		
	}

	else {
//		res = ReadIRStringEx (IRDevices,(byte *)&(ai.ai),sizeof (ANALOG_INPUTS),500);
//		if (res != sizeof (ANALOG_INPUTS)) return (ERR_TIMEOUT);
	}

	memcpy (inputs,&(ad.ad.a_input),sizeof (ANALOG_DATA));

	return (0);
}

*/


int SetRelaisEx (int bus,byte val,byte rel)
{
	IRDATA md;
	byte res;

	md.command = SET_ID;
	md.address = 0;
	md.ir_length = 0x83;

	md.target_mask = rel;
	md.transmit_freq = val;

	md.len = 8;


	res = WriteTransceiverEx (IRDevices + bus,&md);
	if (res) return (res);

	return (0);
}



int SetTransceiverIDEx (int bus,byte id)
{
	IRDATA md;
	byte res;


	if ((IRDevices[bus].fw_capabilities & FN_SOFTID) == 0) {
		fprintf (stderr,"No SoftID IRTrans connected. Cannot set ID\n\n");
		return (-1);
	}

	md.command = SET_ID;
	md.address = 0;
	md.ir_length = 0x4b;
	md.transmit_freq = id * 37;

	md.len = 8;

	md.target_mask = id;

	res = WriteTransceiverEx (IRDevices + bus,&md);
	if (res) return (res);

	return (0);
}


int ResetTransceiverEx (int bus)
{
	IRDATA md;
	byte res;


	md.command = SBUS_RESET;
	md.len = 3;

	res = WriteTransceiverEx (IRDevices + bus,&md);
	if (res) return (res);

	return (0);
}


int GetBusInfoEx (STATUS_BUFFER *sb,int bus)
{
	int i,res;
	IRDATA ir;

	ir.command = HOST_NETWORK_STATUS;
	ir.len = 3;

	if (bus == 0xffff) {
		for (i=0;i<device_cnt;i++) {
			res = WriteTransceiverEx  (IRDevices + i,&ir);
			if (res) return (res);
			if (IRDevices[i].io.if_type == IF_LAN) {
				res = GetBusInfoDetail (sb + i,(byte)i);
				if (res) return (res);
				}
		}
		for (i=0;i<device_cnt;i++) {
			if (IRDevices[i].io.if_type != IF_LAN) res = GetBusInfoDetail (sb + i,(byte)i);
			if (res) return (res);
		}
	}
	else {
		res = WriteTransceiverEx  (IRDevices + bus,&ir);
		if (res) return (res);
		res = GetBusInfoDetail (sb,bus);
		if (res) return (res);
	}

	return (0);
}


int GetBusInfoDetail (STATUS_BUFFER *sb,int bus)
{
	int i,res,j;
	char st[255];
	STATUS_LINE_LAN sl;
	STATUS_LINE_LAN_1 sl1;
	STATUS_BUFFER	sbc;
	STATUS_BUFFER_1	sb1;
	STATUS_BUFFER_2	sb2;
	STATUS_BUFFER_3	sb3;
	STATUS_BUFFER_4	sb4;
	STATUS_BUFFER_5	sb5;

	
	struct sockaddr_in send_adr;

#ifdef LINUX
	fd_set events;
	int maxfd,wait;
	struct timeval tv;
#endif

	if (IRDevices[bus].io.if_type == IF_LAN) {
		sb->my_adress = 0;
#ifdef WIN32
		if (IRDevices[bus].fw_capabilities & FN_SBUS) 
			for (j=0;j < 16;j++) while (1) {
			res = WaitForSingleObject (IrtLanEvent,5000);

			if (res == WAIT_TIMEOUT)  return (ERR_TIMEOUT);

			WSAResetEvent (IrtLanEvent);
			i = sizeof (send_adr);
			memset (&sl,0,sizeof (sl));
			res = recvfrom(irtlan_socket,(byte *)&sl,sizeof (sl),MSG_NOSIGNAL,(struct sockaddr *)&send_adr,&i);
			if (res <= 0 || res != sl.stat.sbus_len + 1) return (ERR_TIMEOUT);
			if (sl.netcommand == RESULT_DEVSTATUS && send_adr.sin_addr.S_un.S_addr == IRDevices[bus].io.IPAddr[0].sin_addr.S_un.S_addr) {
				IRDevices[bus].my_addr = 0;
				if (res == sizeof (STATUS_LINE_LAN_1) || res == (sizeof (STATUS_LINE_LAN_1) - 102)) {
					memcpy (&sl1,&sl,sizeof (STATUS_LINE_LAN_1));
					memset (&sl,0,sizeof (STATUS_LINE_LAN));
					sl.stat.capabilities = sl1.stat.capabilities;
					sl.stat.device_mode = sl1.stat.device_mode;
					sl.stat.extended_mode = sl1.stat.extended_mode;
					sl.stat.extended_mode2 = sl1.stat.extended_mode2;
					sl.stat.extended_mode_ex[0] = sl1.stat.extended_mode_ex[0];
					sl.stat.extended_mode_ex[1] = sl1.stat.extended_mode_ex[1];
					sl.stat.send_mask = sl1.stat.send_mask;
					memcpy (sl.stat.version ,sl1.stat.version,10);
					memcpy (sl.stat.wake_mac,sl1.stat.wake_mac,6);
					if (res == sizeof (STATUS_LINE_LAN_1)) {
						memcpy (sl.stat.remote,sl1.stat.remote,81);
						memcpy (sl.stat.command,sl1.stat.command,21);
					}
				}

				memcpy (&sb->stat[j],&sl.stat,sizeof (STATUS_LINE));
				break;
			}
		}
		else while (1) {
			res = WaitForSingleObject (IrtLanEvent,5000);
			if (res == WAIT_TIMEOUT)  return (ERR_TIMEOUT);

			WSAResetEvent (IrtLanEvent);
			i = sizeof (send_adr);
			memset (&sl,0,sizeof (sl));
			res = recvfrom(irtlan_socket,(byte *)&sl,sizeof (sl),MSG_NOSIGNAL,(struct sockaddr *)&send_adr,&i);
			if (res <= 0 || res != sl.stat.sbus_len + 1) return (ERR_TIMEOUT);
			if (sl.netcommand == RESULT_DEVSTATUS && send_adr.sin_addr.S_un.S_addr == IRDevices[bus].io.IPAddr[0].sin_addr.S_un.S_addr) {
				IRDevices[bus].my_addr = 0;
				if (res == sizeof (STATUS_LINE_LAN_1) || res == (sizeof (STATUS_LINE_LAN_1) - 102)) {
					memcpy (&sl1,&sl,sizeof (STATUS_LINE_LAN_1));
					memset (&sl,0,sizeof (STATUS_LINE_LAN));
					sl.stat.capabilities = sl1.stat.capabilities;
					sl.stat.device_mode = sl1.stat.device_mode;
					sl.stat.extended_mode = sl1.stat.extended_mode;
					sl.stat.extended_mode2 = sl1.stat.extended_mode2;
					sl.stat.extended_mode_ex[0] = sl1.stat.extended_mode_ex[0];
					sl.stat.extended_mode_ex[1] = sl1.stat.extended_mode_ex[1];
					sl.stat.send_mask = sl1.stat.send_mask;
					memcpy (sl.stat.version ,sl1.stat.version,10);
					memcpy (sl.stat.wake_mac,sl1.stat.wake_mac,6);
					if (res == sizeof (STATUS_LINE_LAN_1)) {
						memcpy (sl.stat.remote,sl1.stat.remote,81);
						memcpy (sl.stat.command,sl1.stat.command,21);
					}
				}
				memcpy (&sb->stat[0],&sl.stat,sizeof (STATUS_LINE));
				break;
			}
		}
#endif
#ifdef LINUX
		if (IRDevices[bus].fw_capabilities & FN_SBUS) for (j=0;j < 16;j++) while (1) {
			FD_ZERO (&events);
			FD_SET (irtlan_socket,&events);
			maxfd = irtlan_socket + 1;
			tv.tv_sec = 5;
			tv.tv_usec = 0;

			wait = select (maxfd,&events,NULL,NULL,&tv);
			if (!wait) return (ERR_TIMEOUT);
			i = sizeof (send_adr);
			memset (&sl,0,sizeof (sl));
			res = recvfrom(irtlan_socket,(byte *)&sl,sizeof (sl),MSG_NOSIGNAL,(struct sockaddr *)&send_adr,&i);
			if (res <= 0 || res != sl.stat.sbus_len + 1) return (ERR_TIMEOUT);
			if (sl.netcommand == RESULT_DEVSTATUS && send_adr.sin_addr.s_addr == IRDevices[bus].io.IPAddr[0].sin_addr.s_addr) {
				IRDevices[bus].my_addr = 0;
				if (res == sizeof (STATUS_LINE_LAN_1) || res == (sizeof (STATUS_LINE_LAN_1) - 102)) {
					memcpy (&sl1,&sl,sizeof (STATUS_LINE_LAN_1));
					memset (&sl,0,sizeof (STATUS_LINE_LAN));
					sl.stat.capabilities = sl1.stat.capabilities;
					sl.stat.device_mode = sl1.stat.device_mode;
					sl.stat.extended_mode = sl1.stat.extended_mode;
					sl.stat.extended_mode2 = sl1.stat.extended_mode2;
					sl.stat.extended_mode_ex[0] = sl1.stat.extended_mode_ex[0];
					sl.stat.extended_mode_ex[1] = sl1.stat.extended_mode_ex[1];
					sl.stat.send_mask = sl1.stat.send_mask;
					memcpy (sl.stat.version ,sl1.stat.version,10);
					memcpy (sl.stat.wake_mac,sl1.stat.wake_mac,6);
					if (res == sizeof (STATUS_LINE_LAN_1)) {
						memcpy (sl.stat.remote,sl1.stat.remote,81);
						memcpy (sl.stat.command,sl1.stat.command,21);
					}
				}
				memcpy (&sb->stat[j],&sl.stat,sizeof (STATUS_LINE));
				break;
			}
		}
		else while (1) {
			FD_ZERO (&events);
			FD_SET (irtlan_socket,&events);
			maxfd = irtlan_socket + 1;
			tv.tv_sec = 5;
			tv.tv_usec = 0;

			wait = select (maxfd,&events,NULL,NULL,&tv);
			if (!wait) return (ERR_TIMEOUT);
			i = sizeof (send_adr);
			memset (&sl,0,sizeof (sl));
			res = recvfrom(irtlan_socket,(byte *)&sl,sizeof (sl),MSG_NOSIGNAL,(struct sockaddr *)&send_adr,&i);
			if (res <= 0 || res != sl.stat.sbus_len + 1) return (ERR_TIMEOUT);
			if (sl.netcommand == RESULT_DEVSTATUS && send_adr.sin_addr.s_addr == IRDevices[bus].io.IPAddr[0].sin_addr.s_addr) {
				IRDevices[bus].my_addr = 0;
				if (res == sizeof (STATUS_LINE_LAN_1) || res == (sizeof (STATUS_LINE_LAN_1) - 102)) {
					memcpy (&sl1,&sl,sizeof (STATUS_LINE_LAN_1));
					memset (&sl,0,sizeof (STATUS_LINE_LAN));
					sl.stat.capabilities = sl1.stat.capabilities;
					sl.stat.device_mode = sl1.stat.device_mode;
					sl.stat.extended_mode = sl1.stat.extended_mode;
					sl.stat.extended_mode2 = sl1.stat.extended_mode2;
					sl.stat.extended_mode_ex[0] = sl1.stat.extended_mode_ex[0];
					sl.stat.extended_mode_ex[1] = sl1.stat.extended_mode_ex[1];
					sl.stat.send_mask = sl1.stat.send_mask;
					memcpy (sl.stat.version ,sl1.stat.version,10);
					memcpy (sl.stat.wake_mac,sl1.stat.wake_mac,6);
					if (res == sizeof (STATUS_LINE_LAN_1)) {
						memcpy (sl.stat.remote,sl1.stat.remote,81);
						memcpy (sl.stat.command,sl1.stat.command,21);
					}
				}
				memcpy (&sb->stat[0],&sl.stat,sizeof (STATUS_LINE));
				break;
			}
		}
#endif
	}

	else {


// LINUX
// - RS232 / USB

		res = ReadIRStringEx_ITo (IRDevices+bus,(byte *)sb,sizeof (STATUS_BUFFER),500);
		sprintf (st,"Get Status for Bus ID %d. Len: %d\n",bus,res);
		log_print (st,LOG_DEBUG);

		if (res != sizeof (STATUS_BUFFER) && res != sizeof (STATUS_BUFFER_1) && res != sizeof (STATUS_BUFFER_2) && 
			res != sizeof (STATUS_BUFFER_3) && res != sizeof (STATUS_BUFFER_4) && res != sizeof (STATUS_BUFFER_5)) return (ERR_TIMEOUT);


		if (res == sizeof (STATUS_BUFFER)) {
			memcpy (&sbc,sb,sizeof (STATUS_BUFFER));
			memset (sb,0,sizeof (STATUS_BUFFER));
			sb->my_adress = sbc.my_adress;

			for (i=0;i <= 15;i++) CopyStatusBuffer (&sbc.stat[i],&sb->stat[i],(i == sb->my_adress),bus);
		}
		if (res == sizeof (STATUS_BUFFER_1)) {
			memcpy (&sb1,sb,sizeof (STATUS_BUFFER_1));
			memset (sb,0,sizeof (STATUS_BUFFER));
			sb->my_adress = sb1.my_adress;

			for (i=0;i <= 15;i++) CopyStatusBuffer ((STATUS_LINE *)&sb1.stat[i],&sb->stat[i],(i == sb->my_adress),bus);
		}

		if (res == sizeof (STATUS_BUFFER_2)) {
			memcpy (&sb2,sb,sizeof (STATUS_BUFFER_2));
			memset (sb,0,sizeof (STATUS_BUFFER));
			sb->my_adress = sb2.my_adress;

			for (i=0;i <= 15;i++) CopyStatusBuffer ((STATUS_LINE *)&sb2.stat[i],&sb->stat[i],(i == sb->my_adress),bus);
		}

		if (res == sizeof (STATUS_BUFFER_3)) {
			memcpy (&sb3,sb,sizeof (STATUS_BUFFER_3));
			memset (sb,0,sizeof (STATUS_BUFFER));
			sb->my_adress = sb3.my_adress;

			for (i=0;i <= 15;i++) CopyStatusBuffer ((STATUS_LINE *)&sb3.stat[i],&sb->stat[i],(i == sb->my_adress),bus);
		}

		if (res == sizeof (STATUS_BUFFER_4)) {
			memcpy (&sb4,sb,sizeof (STATUS_BUFFER_4));
			memset (sb,0,sizeof (STATUS_BUFFER));
			sb->my_adress = sb4.my_adress;

			for (i=0;i <= 15;i++) CopyStatusBuffer ((STATUS_LINE *)&sb4.stat[i],&sb->stat[i],(i == sb->my_adress),bus);
		}
		if (res == sizeof (STATUS_BUFFER_5)) {
			memcpy (&sb5,sb,sizeof (STATUS_BUFFER_5));
			memset (sb,0,sizeof (STATUS_BUFFER));
			sb->my_adress = sb5.my_adress;

			for (i=0;i <= 15;i++) CopyStatusBuffer ((STATUS_LINE *)&sb5.stat[i],&sb->stat[i],(i == sb->my_adress),bus);
		}

		IRDevices[bus].my_addr = sb->my_adress;
	}

	SwapStatusbuffer (sb);
	return (0);
}


void CopyStatusBuffer (STATUS_LINE *source,STATUS_LINE *target,int flg,int bus)
{
	STATUS_LINE_1	*sl1;
	STATUS_LINE_2	*sl2;
	STATUS_LINE_3	*sl3;
	STATUS_LINE_4	*sl4;
	STATUS_LINE_5	*sl5;

	if (source->sbus_len == sizeof (STATUS_LINE)) {
		memcpy (target,source,sizeof (STATUS_LINE));
	}
	
	else if (source->sbus_len == sizeof (STATUS_LINE_1)) {
		sl1 = (STATUS_LINE_1 *)source;

		target->device_mode		= sl1->device_mode;
		target->send_mask		= sl1->send_mask;
		memcpy (target->version,sl1->version,10);
		if (flg) target->capabilities = IRDevices[bus].fw_capabilities;
	}

	else if (source->sbus_len == sizeof (STATUS_LINE_2)) {
		sl2 = (STATUS_LINE_2 *)source;

		target->device_mode = sl2->device_mode;
		target->extended_mode = sl2->extended_mode;
		target->send_mask  = sl2->send_mask;
		memcpy (target->version,sl2->version,10);
		if (flg) target->capabilities = IRDevices[bus].fw_capabilities;
	}

	else if (source->sbus_len == sizeof (STATUS_LINE_3)) {
		sl3 = (STATUS_LINE_3 *)source;

		target->device_mode = sl3->device_mode;
		target->extended_mode = sl3->extended_mode;
		target->send_mask  = sl3->send_mask;
		memcpy (target->version,sl3->version,10);
		target->capabilities = sl3->capabilities;

	}

	else if (source->sbus_len == sizeof (STATUS_LINE_4)) {
		sl4 = (STATUS_LINE_4 *)source;

		target->device_mode = sl4->device_mode;
		target->extended_mode = sl4->extended_mode;
		target->send_mask  = sl4->send_mask;
		memcpy (target->version,sl4->version,10);
		target->capabilities = sl4->capabilities;
		target->extended_mode2 = sl4->extended_mode2;
	}

	else if (source->sbus_len == sizeof (STATUS_LINE_5)) {
		sl5 = (STATUS_LINE_5 *)source;

		target->device_mode = sl5->device_mode;
		target->extended_mode = sl5->extended_mode;
		target->send_mask  = sl5->send_mask;
		memcpy (target->version,sl5->version,10);
		target->capabilities = sl5->capabilities;
		target->extended_mode2 = sl5->extended_mode2;
		target->extended_mode_ex[0] = sl5->extended_mode_ex[0];
		target->extended_mode_ex[1] = sl5->extended_mode_ex[1];
	}
}


int ResendIREx (int bus,IRDATA *ir_data)
{
	int res,i;

	ir_data -> command = HOST_RESEND;
	ir_data -> len = 6;

	if (bus == 0xffff) for (i=0;i<device_cnt;i++) res = WriteTransceiverEx (&IRDevices[i],ir_data);
	else res = (WriteTransceiverEx (&IRDevices[bus],ir_data));
	if (res == ERR_RESEND) {
		ir_data -> command = HOST_SEND;
		if (ir_data -> mode & RAW_DATA)
			ir_data -> len = (sizeof (IRDATA) - (CODE_LEN + (RAW_EXTRA))) + ir_data -> ir_length;
		else
			ir_data -> len = (sizeof (IRDATA) - CODE_LEN) + ir_data -> ir_length;
		if (bus == 0xffff) for (i=0;i<device_cnt;i++) res = WriteTransceiverEx (&IRDevices[i],ir_data);
		else res = (WriteTransceiverEx (&IRDevices[bus],ir_data));
	}

	return (res);
}

int GetDeviceData (int cmd_num,DATABUFFER *dat)
{
	int res;
	IRDATA ird;
	IRDATA send;
	int mac_len,mac_pause,rpt_len;

	res = DBGetIRCode (cmd_num,&ird,0,&mac_len,&mac_pause,&rpt_len,0,0);
	if (res) return (res);
	if (mac_len || rpt_len) return (ERR_ISMACRO);

	ird.command = HOST_SEND;
	if (ird.mode & RAW_DATA) ird.len = (sizeof (IRDATA) - (CODE_LEN + (RAW_EXTRA))) + ird.ir_length;
	else {
		ird.len = (sizeof (IRDATA) - CODE_LEN) + ird.ir_length;
		if (ird.mode & RC6_DATA) ird.len++;
	}

	swap_irdata (&ird,&send);

	if (time_len != TIME_LEN) {
		if (ird.mode & RAW_DATA) {
			if (ird.ir_length > OLD_LENRAW) return (ERR_LONGRAW);
		}
		else {
			if (ird.time_cnt > 6) return (ERR_LONGDATA);
			ConvertToIRTRANS3 (&send);
		}
	}
	send.checksumme = get_checksumme (&send);
	dat->statustype = STATUS_DEVICEDATA;
	dat->statuslen = sizeof (DATABUFFER);
	memcpy (dat->data,&send,sizeof (IRDATA));

	return (0);
	
}

int SendIR (int cmd_num,int address,byte netcommand)
{
	byte cal = 0,tog = 0;
	int i,res,bus = 0,rpt;
	IRDATA ird,ird_rpt;
	int mac_len,mac_pause,rpt_len;

	if (protocol_version >= 210) bus = (address >> 19) & (MAX_IR_DEVICES - 1);
	else bus = (address >> 20) & (MAX_IR_DEVICES - 1);
	if (address & 0x40000000) {
		bus = 0xffff;
		cal = (IRDevices[0].fw_capabilities & FN_CALIBRATE) != 0;
		tog = (IRDevices[0].io.toggle_support) != 0;
	}
	else {
		cal = (IRDevices[bus].fw_capabilities & FN_CALIBRATE) != 0;
		tog = (IRDevices[bus].io.toggle_support) != 0;
	}

	if (cmd_pnt[cmd_num].timing == RS232_IRCOMMAND) {

		if (bus == 0xffff) for (i=0;i<device_cnt;i++) {
			res = SendSerialBlock (i,cmd_pnt[cmd_num].data,(byte)cmd_pnt[cmd_num].ir_length,(byte)cmd_pnt[cmd_num].pause);
		}
		else res = SendSerialBlock (bus,cmd_pnt[cmd_num].data,(byte)cmd_pnt[cmd_num].ir_length,(byte)cmd_pnt[cmd_num].pause);
		return (res);
	}

	res = DBGetIRCode (cmd_num,&ird,0,&mac_len,&mac_pause,&rpt_len,cal,tog);
	if (res) return (res);

	rpt = DBGetRepeatCode (cmd_num,&ird_rpt,cal,tog);

	if (address & 0x10000) {
		ird.target_mask = (word)address & 0xffff;
	}
	
	ird.address = 0;
	if (address & 0x80000000) {
		ird.address |= ((address >> 25) & 28) + 4;
		ird.address += ((address >> 17) & 3) * 32;
	}
	else if (address & 0x60000) ird.address = (byte)((address >> 17) & 3);

	if (!rpt)res = DoSendIR (&ird,&ird_rpt,rpt_len,mac_pause,bus,netcommand);
	else res = DoSendIR (&ird,NULL,rpt_len,mac_pause,bus,netcommand);
	
	if (mac_len) msSleep (mac_pause);
	
	for (i=1;i < mac_len;i++) {
		res = DBGetIRCode (cmd_num,&ird,i,&mac_len,&mac_pause,&rpt_len,cal,tog);
		if (res) return (res);

		if (address & 0x10000) {
			ird.target_mask = (word)address & 0xffff;
		}
		
		ird.address = 0;
		if (address & 0x80000000) {
			ird.address |= ((address >> 25) & 28) + 4;
			ird.address += ((address >> 17) & 3) * 32;
		}
		else if (address & 0x60000) ird.address = (byte)((address >> 17) & 3);

		if (!rpt)res = DoSendIR (&ird,&ird_rpt,rpt_len,mac_pause,bus,netcommand);
		else res = DoSendIR (&ird,NULL,rpt_len,mac_pause,bus,netcommand);
		
		if (mac_len) msSleep (mac_pause);
	}

	return (res);
}

int TransferToTimelen18 (IRDATA *src,IRDATA *snd,int bus)
{
	int i;
	char st[255];

	if (src ->mode == TIMECOUNT_18 && !(IRDevices[bus].fw_capabilities2 & FN2_PULSELEN_18)) {
		sprintf (st,"Error - IR Mode Timelen 18 not supported\n");
		log_print (st,LOG_ERROR);
		return (0);
	}


	if ((src->mode & SPECIAL_IR_MODE) == PULSE_200 && !(IRDevices[bus].fw_capabilities2 & FN2_PULSE200)) {
		IRDATA_18 *ir18 = (IRDATA_18 *)snd;
		IRDATA_SINGLE *irs = (IRDATA_SINGLE *)src;

		if (!(IRDevices[bus].fw_capabilities2 & FN2_PULSELEN_18)) {
			sprintf (st,"Error - IR Mode Pulse 200 not supported\n");
			log_print (st,LOG_ERROR);
			return (0);
		}

		memset (ir18,0,sizeof (IRDATA));
		
		ir18->command = irs->command;
		ir18->address = irs->address;
		ir18->target_mask = irs->target_mask;
		ir18->ir_length = irs->ir_length;
		ir18->transmit_freq = irs->transmit_freq;
		ir18->mode = TIMECOUNT_18;
		ir18->time_cnt = irs->time_cnt;
		if (irs->time_cnt > 18) {
			sprintf (st,"Error - IR Mode Pulse 200 not convertible. Timecnt = %d\n",irs->time_cnt);
			log_print (st,LOG_ERROR);
			return (0);
		}
		ir18->ir_repeat = irs->ir_repeat;
		ir18->repeat_pause = irs->repeat_pause;
		memcpy (ir18->data,irs->data,irs->ir_length);
		memcpy (ir18->pause_len,irs->multi_len,18 * 2);
		for (i=0;i < 18;i++) ir18->pulse_len[i] = irs->single_len;

		ir18->len = (sizeof (IRDATA_18) - CODE_LEN_18) + ir18->ir_length;
		if (mode_flag & CODEDUMP) PrintPulseData (snd);
	
	}

	else memcpy (snd,src,sizeof (IRDATA));

	return (1);
}


int DoSendIR (IRDATA *ir_data,IRDATA *ir_rep,int rpt_len,int rpt_pause,int bus,byte netcommand)
{
	byte cmd,rcmd;
	int res = 0,i;
	unsigned int end_time;
	IRDATA snd;

	if (netcommand == COMMAND_SENDMASK) {
		cmd = HOST_SEND_LEDMASK;
		rcmd = HOST_RESEND_LEDMASK;
	}
	else {
		cmd = HOST_SEND;
		rcmd = HOST_RESEND;
	}

	ir_data -> command = cmd;
	
	if (ir_data->mode == TIMECOUNT_18) {
		ir_data -> len = (sizeof (IRDATA_18) - CODE_LEN_18) + ((IRDATA_18 *)ir_data) -> ir_length;
		if (mode_flag & CODEDUMP) PrintPulseData (ir_data);
	}
	else if ((ir_data->mode & SPECIAL_IR_MODE) == PULSE_200) {
		ir_data -> len = (sizeof (IRDATA_SINGLE) - CODE_LEN_SINGLE) + ir_data -> ir_length;
		if (mode_flag & CODEDUMP) PrintPulseData (ir_data);
	}
	else if (ir_data -> mode & RAW_DATA) {
		ir_data -> len = (sizeof (IRDATA) - (CODE_LEN + (RAW_EXTRA))) + ir_data -> ir_length;
		if (mode_flag & CODEDUMP) PrintRawData ((IRRAW *)ir_data);
	}
	else {
		ir_data -> len = (sizeof (IRDATA) - CODE_LEN) + ir_data -> ir_length;
		if (ir_data->mode & RC6_DATA) ir_data->len++;
		if (mode_flag & CODEDUMP) PrintPulseData (ir_data);
	}


	if (!rpt_len) {
		if (bus == 0xffff) for (i=0;i<device_cnt;i++) {
			if (TransferToTimelen18 (ir_data,&snd,i)) {
				if (!IRDevices[i].io.ext_carrier) snd.transmit_freq = Convert2OldCarrier (ir_data->transmit_freq);
				res = WriteTransceiverEx (&IRDevices[i],&snd);
			}
		}
		else {
			if (TransferToTimelen18 (ir_data,&snd,bus)) {
				if (!IRDevices[bus].io.ext_carrier) snd.transmit_freq = Convert2OldCarrier (ir_data->transmit_freq);
				res = WriteTransceiverEx (&IRDevices[bus],&snd);
			}
		}
		return (res);
	}

	if (ir_rep) {
		ir_rep -> command = cmd;

		if (ir_rep->mode == TIMECOUNT_18) {
			ir_rep -> len = (sizeof (IRDATA_18) - CODE_LEN_18) + ((IRDATA_18 *)ir_rep) -> ir_length;
			if (mode_flag & CODEDUMP) PrintPulseData (ir_rep);
		}
		else if ((ir_rep -> mode & SPECIAL_IR_MODE) == PULSE_200) {
			ir_rep -> len = (sizeof (IRDATA_SINGLE) - CODE_LEN_SINGLE) + ir_rep -> ir_length;
			if (mode_flag & CODEDUMP) PrintPulseData (ir_rep);
		}
		else if (ir_rep -> mode & RAW_DATA) {
			ir_rep -> len = (sizeof (IRDATA) - (CODE_LEN + (RAW_EXTRA))) + ir_rep -> ir_length;
			if (mode_flag & CODEDUMP) PrintRawData ((IRRAW *)ir_rep);
		}
		else {
			ir_rep -> len = (sizeof (IRDATA) - CODE_LEN) + ir_rep -> ir_length;
			if (ir_rep->mode & RC6_DATA) ir_rep->len++;
			if (mode_flag & CODEDUMP) PrintPulseData (ir_rep);
		}
	}


	end_time = GetMsTime () + rpt_len;
	if (bus == 0xffff) for (i=0;i<device_cnt;i++) {
		if (TransferToTimelen18 (ir_data,&snd,i)) {
			if (!IRDevices[i].io.ext_carrier) snd.transmit_freq = Convert2OldCarrier (ir_data->transmit_freq);
			res = WriteTransceiverEx (&IRDevices[i],&snd);
		}
	}
	else {
		if (TransferToTimelen18 (ir_data,&snd,bus)) {
			if (!IRDevices[bus].io.ext_carrier) snd.transmit_freq = Convert2OldCarrier (ir_data->transmit_freq);
			res = WriteTransceiverEx (&IRDevices[bus],&snd);
		}
	}

	msSleep (rpt_pause);

	if (ir_rep && GetMsTime () < end_time) {
		if (bus == 0xffff) for (i=0;i<device_cnt;i++) {
			if (TransferToTimelen18 (ir_data,&snd,i)) {
				if (!IRDevices[i].io.ext_carrier) snd.transmit_freq = Convert2OldCarrier (ir_data->transmit_freq);
				res = WriteTransceiverEx (&IRDevices[i],&snd);
			}
		}
		else {
			if (TransferToTimelen18 (ir_data,&snd,bus)) {
				if (!IRDevices[bus].io.ext_carrier) snd.transmit_freq = Convert2OldCarrier (ir_data->transmit_freq);
				res = WriteTransceiverEx (&IRDevices[bus],&snd);
			}
		}
		msSleep (rpt_pause);
	}

	while (GetMsTime () < end_time) {
		ir_data -> command = rcmd;
		ir_data -> len = 3;

		if (bus == 0xffff) for (i=0;i<device_cnt;i++) res = WriteTransceiverEx (&IRDevices[i],ir_data);
		else res = (WriteTransceiverEx (&IRDevices[bus],ir_data));
		if (res == ERR_RESEND) {
			if (ir_rep) memcpy (ir_data,ir_rep,sizeof (IRDATA));
			ir_data -> command = cmd;
			
			if (ir_data->mode == TIMECOUNT_18) ir_data -> len = (sizeof (IRDATA_18) - CODE_LEN_18) + ((IRDATA_18 *)ir_data) -> ir_length;
			else if ((ir_data->mode & SPECIAL_IR_MODE) == PULSE_200) ir_data -> len = (sizeof (IRDATA_SINGLE) - CODE_LEN_SINGLE) + ir_data -> ir_length;
			else if (ir_data -> mode & RAW_DATA) ir_data -> len = (sizeof (IRDATA) - (CODE_LEN + (RAW_EXTRA))) + ir_data -> ir_length;
			else {
				ir_data -> len = (sizeof (IRDATA) - CODE_LEN) + ir_data -> ir_length;
				if (ir_data->mode & RC6_DATA) ir_data->len++;
			}
			if (bus == 0xffff) for (i=0;i<device_cnt;i++) {
				if (TransferToTimelen18 (ir_data,&snd,i)) {
					if (!IRDevices[i].io.ext_carrier) snd.transmit_freq = Convert2OldCarrier (ir_data->transmit_freq);
					res = WriteTransceiverEx (&IRDevices[i],&snd);
				}
			}
			else {
				if (TransferToTimelen18 (ir_data,&snd,bus)) {
					if (!IRDevices[bus].io.ext_carrier) snd.transmit_freq = Convert2OldCarrier (ir_data->transmit_freq);
					res = WriteTransceiverEx (&IRDevices[bus],&snd);
				}
			}
		}
		msSleep (rpt_pause);
	}
	return (res);
}


byte Convert2OldCarrier (byte carrier)
{
	word oc;
	if (carrier == 255 || carrier < 128) return (carrier);
	oc = (carrier & 127) << 2;
	if (oc > 255) oc = 255;
	return ((byte)oc);
}


int SendIRDataEx (IRDATA *ir_data,int address)
{
	IRDATA snd;
	int bus = 0,res,i;
	if (address & 0x10000) {
		ir_data->target_mask = (word)address & 0xffff;
	}
	if (protocol_version >= 210) bus = (address >> 19) & (MAX_IR_DEVICES - 1);
	else bus = (address >> 20) & (MAX_IR_DEVICES - 1);
	if (address & 0x40000000) bus = 0xffff;

	ir_data -> address = 0;
	if (address & 0x80000000) {
		ir_data -> address |= ((address >> 25) & 28) + 4;
		ir_data -> address += ((address >> 17) & 3) * 32;
	}
	else if (address & 0x60000) ir_data -> address = (byte)((address >> 17) & 3);


	ir_data -> command = HOST_SEND;

	if (ir_data->mode == TIMECOUNT_18) {
		ir_data -> len = (sizeof (IRDATA_18) - CODE_LEN_18) + ((IRDATA_18 *)ir_data) -> ir_length;
		if (mode_flag & CODEDUMP) PrintPulseData (ir_data);
	}
	else if ((ir_data->mode & SPECIAL_IR_MODE) == PULSE_200) {
		ir_data -> len = (sizeof (IRDATA_SINGLE) - CODE_LEN_SINGLE) + ir_data -> ir_length;
		if (mode_flag & CODEDUMP) PrintPulseData (ir_data);
	}
	else if (ir_data -> mode & RAW_DATA) {
		ir_data -> len = (sizeof (IRDATA) - (CODE_LEN + (RAW_EXTRA))) + ir_data -> ir_length;
		if (mode_flag & CODEDUMP) PrintRawData ((IRRAW *)ir_data);
	}
	else {
		ir_data -> len = (sizeof (IRDATA) - CODE_LEN) + ir_data -> ir_length;
		if (ir_data->mode & RC6_DATA) ir_data->len++;
		if (mode_flag & CODEDUMP) PrintPulseData (ir_data);
	}
	if (bus == 0xffff) for (i=0;i<device_cnt;i++) {
		if (TransferToTimelen18 (ir_data,&snd,i)) {
			if (!IRDevices[i].io.ext_carrier) snd.transmit_freq = Convert2OldCarrier (ir_data->transmit_freq);
			res = WriteTransceiverEx (&IRDevices[i],&snd);
		}
	}
	else {
		if (TransferToTimelen18 (ir_data,&snd,bus)) {
			if (!IRDevices[bus].io.ext_carrier) snd.transmit_freq = Convert2OldCarrier (ir_data->transmit_freq);
			res = WriteTransceiverEx (&IRDevices[bus],&snd);
		}
	}
	return (res);
}


void LCDTimeCommand (byte mode)
{
#ifndef WINCE
	int i;
	IRRAW irw;
#ifdef WIN32
	struct _timeb tb;
#endif
#ifdef LINUX
	struct timeval tb;
	struct timezone tz;
#endif

	memset (&irw,0,sizeof (IRRAW));

	irw.command = HOST_SEND;
	irw.mode = LCD_DATA | mode;
	irw.target_mask = 0xffff;

#ifdef WIN32
	_ftime (&tb);
	if (tb.dstflag) i = 3600;
	else i = 0;
	*((unsigned int *)irw.data) = (unsigned int)(tb.time - 60 * tb.timezone + i);
	irw.data[4] = tb.millitm / 48;
#endif

#ifdef LINUX
	i = gettimeofday (&tb,&tz);
	if (i) {
		gettimeofday (&tb,NULL);
		*((unsigned int *)irw.data) = tb.tv_sec;
	}
	else *((unsigned int *)irw.data) = tb.tv_sec - 60 * tz.tz_minuteswest;
	irw.data[4] = tb.tv_usec / 48000;
#endif

	swap_int ((int *)irw.data);
	irw.len = (sizeof (IRDATA) - (CODE_LEN + (RAW_EXTRA))) + 5;
	for (i=0;i<device_cnt;i++) WriteTransceiverEx (&IRDevices[i],(IRDATA *)&irw);
#endif
}


void LCDBrightness (int val)
{
	byte data[10];
	
	*data = val;
	AdvancedLCD (LCD_DATA | LCD_BRIGHTNESS,data,1);
}

int AdvancedLCD (byte mode,byte data[],int len)
{
	int res,i;
	IRRAW irr;

	if (display_bus != 0xffff && !IRDevices[display_bus].io.advanced_lcd) return (0);

	memset (&irr,0,sizeof (IRRAW));

	irr.command = HOST_SEND;
	irr.ir_length = 0;
	irr.mode = mode;

	irr.target_mask = 0xffff;

	memcpy (irr.data,data,len);

	irr.len = (byte)(sizeof (IRRAW) - CODE_LENRAW + len);
	
	if (display_bus == 0xffff) for (i=0;i<device_cnt;i++) {
		res = WriteTransceiverEx (&IRDevices[i],(IRDATA *)&irr);
	}
	else res = WriteTransceiverEx (&IRDevices[display_bus],(IRDATA *)&irr);

	return (res);
}



int SendLCD (IRRAW *ir_data,int address)
{
	int i,res,bus = 0;

	ir_data -> command = HOST_SEND;
	ir_data -> ir_length = 0;
	if (address & 0x10000) {
		ir_data->target_mask = (word)address & 0xffff;
	}
	bus = display_bus;

	ir_data -> len = sizeof (IRRAW) - CODE_LENRAW + (byte)strlen (ir_data -> data) + 1;
	if (bus == 0xffff) for (i=0;i<device_cnt;i++) {
		res = WriteTransceiverEx (&IRDevices[i],(IRDATA *)ir_data);
	}
	else res = WriteTransceiverEx (&IRDevices[bus],(IRDATA *)ir_data);
	return (res);
}

int ReadLearndataLAN (DEVICEINFO *dev,byte *ird,int timeout)
{
	int res,i;
	byte buf[sizeof (IRDATA) + 10];
	struct sockaddr_in send_adr;
#ifdef LINUX
	fd_set events;
	int maxfd,wait;
	struct timeval tv;
#endif

	memset (buf,0,10);
	do {
#ifdef WIN32
		res = WaitForSingleObject (IrtLanEvent,timeout);
		if (res == WAIT_TIMEOUT) {
			buf[0] = 5;
			IRTransLanSend (dev,(IRDATA *)buf);
			return (ERR_TIMEOUT);
		}
		WSAResetEvent (IrtLanEvent);
#endif

#ifdef LINUX
		FD_ZERO (&events);

		FD_SET (irtlan_socket,&events);
		maxfd = irtlan_socket + 1;

		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;

		wait = select (maxfd,&events,NULL,NULL,&tv);
		if (!wait) return (ERR_TIMEOUT);
#endif

		i = sizeof (send_adr);
		res = recvfrom(irtlan_socket,buf,sizeof (IRDATA),MSG_NOSIGNAL,(struct sockaddr *)&send_adr,&i);
		if (res <= 0 || res != buf[1] + 1) {
			sprintf (buf,"Learn Error: %d - %d\n",res,buf[1]);
			log_print (buf,LOG_ERROR);
			return (ERR_TIMEOUT);
		}
 
	} while (buf[0] != RESULT_LEARN || send_adr.sin_addr.s_addr != dev->io.IPAddr[0].sin_addr.s_addr);

	memcpy (ird,buf+1,buf[1]);

	return (0);
}



int LearnNextIREx (IRDATA *ir_data,word addr,word timeout,word ir_timeout,byte carrier,byte modes)
{
	int res;
	byte len;
	int bus;
	IRDATA buffer;
	IRDATA_SINGLE *ird,*buf;
	IRDATA_18 *ird18,*buf18;

	do {
		ir_data -> address = 0;
		if ((addr & 0x7f) <= 15)  ir_data -> address |= addr;
		if ((addr & 0x7f) == 'L') ir_data -> address |= ADRESS_LOCAL;
		if ((addr & 0x7f) == '*') ir_data -> address |= ADRESS_ALL;

		bus = (addr >> 8) & (MAX_IR_DEVICES - 1);

		ir_data -> command = HOST_LEARNIRQUICK;

		if (ir_data->mode == TIMECOUNT_18) ir_data -> len = (sizeof (IRDATA_18) - CODE_LEN_18) + ((IRDATA_18 *)ir_data) -> ir_length;
		else if ((ir_data->mode & SPECIAL_IR_MODE) == PULSE_200) ir_data -> len = sizeof (IRDATA_SINGLE) - CODE_LEN_SINGLE;
		else ir_data -> len = sizeof (IRDATA) - CODE_LEN;

		if (addr & 0x80) {
			ir_data->target_mask = ir_timeout;			// Neue Lernmodi
			ir_data -> ir_length = modes;
		}

		else {
			if (strcmp (IRDevices[bus].version+1,"5.20.01") >= 0) {
				ir_data -> target_mask = ir_timeout & 15;
				if (ir_timeout & 32) ir_data -> target_mask |= LONG_LEARN_MODE;
			}
			else {
				res = ir_timeout & 15;
				if (strcmp (IRDevices[bus].version+1,"4.04.35") < 0 && res > 14) res = 14;
				ir_data -> target_mask = res;
				ir_data->target_mask |= (ir_timeout / 2) & (LONG_LEARN_MODE | RECEIVER_SELECT_MASK);
			}
		}
		
		res = WriteTransceiverEx (IRDevices + bus,ir_data);
		if (res) return (res);

		if (IRDevices[bus].io.if_type == IF_LAN) len = sizeof (IRDATA);

		else {
			res = ReadIRStringEx (IRDevices + bus,&len,1,timeout);

			if (!res) {
				CancelLearnEx (IRDevices + bus);
				return (ERR_TIMEOUT);
			}
		}

	} while (len <= sizeof (IRDATA) - CODE_LEN);

	if (IRDevices[bus].io.if_type == IF_LAN) {
		if (ReadLearndataLAN (IRDevices + bus,(byte *)&buffer,timeout)) return (ERR_TIMEOUT);
	}
	else {
		if (ReadIRStringEx (IRDevices + bus,&buffer.checksumme,len-1,200) != len-1) return (ERR_TIMEOUT);
	}


	if (TIME_LEN != time_len) ConvertToIRTRANS4 ((IRDATA3 *)&buffer);

	swap_irdata (&buffer,NULL);

	if (ir_data->mode == TIMECOUNT_18) {
		ird18 = (IRDATA_18 *)ir_data;
		buf18 = (IRDATA_18 *)&buffer;

		memcpy (ird18 -> data,buf18->data,CODE_LEN_18);
		ird18 -> ir_length = buf18 -> ir_length;
		ird18 -> address = buf18 -> address;
		ird18 -> data[ird18 -> ir_length] = 0;
		ird18 -> time_cnt = buf18 -> time_cnt;
	}

	else if ((ir_data->mode & SPECIAL_IR_MODE) == PULSE_200) {
		ird = (IRDATA_SINGLE *)ir_data;
		buf = (IRDATA_SINGLE *)&buffer;

		memcpy (ird -> data,buf->data,CODE_LEN_SINGLE);
		ird -> ir_length = buf -> ir_length;
		ird -> address = buf -> address;
		ird -> data[ird -> ir_length] = 0;
		ird -> time_cnt = buf -> time_cnt;
	}
	else {
		memcpy (ir_data -> data,buffer.data,CODE_LEN);
		ir_data -> ir_length = buffer.ir_length;
		ir_data -> address = buffer.address;
		ir_data -> data[ir_data -> ir_length] = 0;
		ir_data -> time_cnt = buffer.time_cnt;
	}

	return (0);
}

void ConvertToIRTRANS4 (IRDATA3 *ird)
{
	IRDATA irnew;

	memset (&irnew,0,sizeof (IRDATA));

	memcpy (&irnew,ird,21);
	memcpy (irnew.pulse_len,ird->pulse_len,12);
	memcpy (&irnew.time_cnt,&ird->time_cnt,CODE_LEN + 3);
	irnew.len = ird->len + 8;
	memcpy (ird,&irnew,sizeof (IRDATA));
}


void CancelLearnEx (DEVICEINFO *dev)
{
	byte res = 1;
	char st[10];
	WriteIRStringEx (dev,(byte *)&res,1);
	res = ReadIRStringEx (dev,st,1,500);
}





int LearnIREx (IRDATA *ir_data,word addr,word timeout,word ir_timeout,byte carrier,byte modes)
{
	int res;
	byte len,carrmeas = 0;
	int bus;

	do {
		ir_data -> address = 0;
		if ((addr & 0x7f) <= 15)  ir_data -> address |= addr;
		if ((addr & 0x7f) == 'L') ir_data -> address |= ADRESS_LOCAL;
		if ((addr & 0x7f) == '*') ir_data -> address |= ADRESS_ALL;

		bus = (addr >> 8) & (MAX_IR_DEVICES - 1);

		ir_data -> command = HOST_LEARNIR;

		ir_data -> len = 7;

		if (addr & 0x80) {
			ir_data->target_mask = ir_timeout;			// Neue Lernmodi

			if ((((ir_timeout & LRN_RCV_SELECT) / 64) & 7) == 6) carrmeas = 1;		// Carriermessung gewählt

			ir_data -> ir_length = modes;
		}

		else {
			if (strcmp (IRDevices[bus].version+1,"5.20.01") >= 0) {
				ir_data -> target_mask = ir_timeout & 15;
				if (ir_timeout & 32) ir_data -> target_mask |= LONG_LEARN_MODE;
				if ((IRDevices[bus].extended_mode_ex[0] & 7) == 6 && (IRDevices[bus].extended_mode & STANDARD_RCV)) carrmeas = 1;
				if ((IRDevices[bus].extended_mode_ex[0] & 0x70) == 0x60 && (IRDevices[bus].extended_mode & HI_RCV)) carrmeas = 1;
			}
			else {
				res = ir_timeout & 15;
				if (strcmp (IRDevices[bus].version+1,"4.04.35") < 0 && res > 14) res = 14;
				ir_data -> target_mask = res;
				ir_data->target_mask |= (ir_timeout / 2) & (LONG_LEARN_MODE | RECEIVER_SELECT_MASK);
				ir_data -> len = 6;
			}
		}


		res = WriteTransceiverEx (IRDevices + bus,ir_data);
		if (res) return (res);

		if (IRDevices[bus].io.if_type == IF_LAN) len = sizeof (IRDATA);

		else {
			res = ReadIRStringEx (IRDevices + bus,&len,1,timeout);

			if (!res) {
				CancelLearnEx (IRDevices + bus);
				return (ERR_TIMEOUT);
			}
		}

	} while (len <= sizeof (IRDATA) - CODE_LEN);

	if (IRDevices[bus].io.if_type == IF_LAN) {
		if (ReadLearndataLAN (IRDevices + bus,(byte *)ir_data,timeout)) return (ERR_TIMEOUT);
	}
	else {
		if (ReadIRStringEx (IRDevices + bus,&ir_data->checksumme,len-1,200) != len-1) return (ERR_TIMEOUT);
	}

	if (TIME_LEN != time_len) ConvertToIRTRANS4 ((IRDATA3 *)ir_data);

	if (ir_data->mode == TIMECOUNT_18) ((IRDATA_18 *)ir_data) -> data[((IRDATA_18 *)ir_data) -> ir_length] = 0;
	else if ((ir_data->mode & SPECIAL_IR_MODE) == PULSE_200) ((IRDATA_SINGLE *)ir_data) -> data[((IRDATA_SINGLE *)ir_data) -> ir_length] = 0;
	else ir_data -> data[ir_data -> ir_length] = 0;

	swap_irdata (ir_data,NULL);

	res = 0;
	if ((ir_data->command & 0xf0) > 0x10) res = ((ir_data->command-RCV_STATUS_LENGTH) / 16) + ERR_LEARN_LENGTH;

	ir_data->command = 0;
	
	if (carrmeas) {
		if (ir_data->transmit_freq < 10) {
			ir_data->transmit_freq = 38;									// Carriermessung Timeout -> 38kHz Default
			ir_data->command = 0xfe;
		}
		else ir_data->command = 0xff;										// Messung eintragen
	}

	else {
		if (carrier && ir_data->transmit_freq != 255) ir_data->transmit_freq = carrier;
	}
	
	PrintPulseData (ir_data);

	return (res);
}


int LearnRepeatIREx (IRDATA *ir_data,word addr,word timeout,word ir_timeout,byte carrier,byte modes)
{
	int res;
	byte len,carrmeas = 0;
	int bus;

	do {
		ir_data -> address = 0;
		if ((addr & 0x7f) <= 15)  ir_data -> address |= addr;
		if ((addr & 0x7f) == 'L') ir_data -> address |= ADRESS_LOCAL;
		if ((addr & 0x7f) == '*') ir_data -> address |= ADRESS_ALL;

		bus = (addr >> 8) & (MAX_IR_DEVICES - 1);

		ir_data -> command = HOST_LEARNIRREPEAT;

		ir_data -> len = 7;

		if (addr & 0x80) {
			ir_data->target_mask = ir_timeout;			// Neue Lernmodi

			if ((((ir_timeout & LRN_RCV_SELECT) / 64) & 7) == 6) carrmeas = 1;		// Carriermessung gewählt

			ir_data -> ir_length = modes;
		}

		else {
			if (strcmp (IRDevices[bus].version+1,"5.20.01") >= 0) {
				ir_data -> target_mask = ir_timeout & 15;
				if (ir_timeout & 32) ir_data -> target_mask |= LONG_LEARN_MODE;
				if ((IRDevices[bus].extended_mode_ex[0] & 7) == 6 && (IRDevices[bus].extended_mode & STANDARD_RCV)) carrmeas = 1;
				if ((IRDevices[bus].extended_mode_ex[0] & 0x70) == 0x60 && (IRDevices[bus].extended_mode & HI_RCV)) carrmeas = 1;
			}
			else {
				res = ir_timeout & 15;
				if (strcmp (IRDevices[bus].version+1,"4.04.35") < 0 && res > 14) res = 14;
				ir_data -> target_mask = res;
				ir_data->target_mask |= (ir_timeout / 2) & (LONG_LEARN_MODE | RECEIVER_SELECT_MASK);
				ir_data -> len = 6;
			}
		}


		res = WriteTransceiverEx (IRDevices + bus,(IRDATA *)ir_data);
		if (res) return (res);
		if (IRDevices[bus].io.if_type == IF_LAN) len = sizeof (IRDATA);

		else {
			res = ReadIRStringEx (IRDevices + bus,&len,1,timeout);

			if (!res) {
				CancelLearnEx (IRDevices + bus);
				return (ERR_TIMEOUT);
			}
		}

	} while (len <= sizeof (IRDATA) - CODE_LEN);

	if (IRDevices[bus].io.if_type == IF_LAN) {
		if (ReadLearndataLAN (IRDevices + bus,(byte *)ir_data,timeout)) return (ERR_TIMEOUT);
	}
	else {
		if (ReadIRStringEx (IRDevices + bus,&ir_data->checksumme,len-1,200) != len-1) return (ERR_TIMEOUT);
	}

	if (TIME_LEN != time_len) ConvertToIRTRANS4 ((IRDATA3 *)ir_data);

	if (ir_data->mode == TIMECOUNT_18) ((IRDATA_18 *)ir_data) -> data[((IRDATA_18 *)ir_data) -> ir_length] = 0;
	else if ((ir_data->mode & SPECIAL_IR_MODE) == PULSE_200) ((IRDATA_SINGLE *)ir_data) -> data[((IRDATA_SINGLE *)ir_data) -> ir_length] = 0;
	else ir_data -> data[ir_data -> ir_length] = 0;

	swap_irdata (ir_data,NULL);

	res = 0;
	if ((ir_data->command & 0xf0) > 0x10) res = ((ir_data->command-RCV_STATUS_LENGTH) / 16) + ERR_LEARN_LENGTH;

	ir_data->command = 0;
	
	if (carrmeas) {
		if (ir_data->transmit_freq < 10) {
			ir_data->transmit_freq = 38;									// Carriermessung Timeout -> 38kHz Default
			ir_data->command = 0xfe;
		}
		else ir_data->command = 0xff;										// Messung eintragen
	}

	else {
		if (carrier && ir_data->transmit_freq != 255) ir_data->transmit_freq = carrier;
	}

	PrintPulseData (ir_data);

	return (res);
}


int LearnRawIREx (IRRAW *ir_data,word addr,word timeout,word ir_timeout,byte carrier)
{
	int res;
	byte len,carrmeas = 0;
	int bus;

	do {
		ir_data -> address = 0;
		if ((addr & 0x7f) <= 15)  ir_data -> address |= addr;
		if ((addr & 0x7f) == 'L') ir_data -> address |= ADRESS_LOCAL;
		if ((addr & 0x7f) == '*') ir_data -> address |= ADRESS_ALL;

		bus = (addr >> 8) & (MAX_IR_DEVICES - 1);

		ir_data -> command = HOST_LEARNIRRAW;

		if (addr & 0x80) {
			ir_data->ir_length = 0;
			ir_data->target_mask = ir_timeout;			// Neue Lernmodi

			if ((((ir_timeout & LRN_RCV_SELECT) / 64) & 7) == 6) carrmeas = 1;		// Carriermessung gewählt
		}

		else {
			if (strcmp (IRDevices[bus].version+1,"5.20.01") >= 0) {
				ir_data -> target_mask = ir_timeout & 15;
				if (ir_timeout & 32) ir_data -> target_mask |= LONG_LEARN_MODE;
				if ((IRDevices[bus].extended_mode_ex[0] & 7) == 6 && (IRDevices[bus].extended_mode & STANDARD_RCV)) carrmeas = 1;
				if ((IRDevices[bus].extended_mode_ex[0] & 0x70) == 0x60 && (IRDevices[bus].extended_mode & HI_RCV)) carrmeas = 1;
			}
			else {
				res = ir_timeout & 15;
				if (strcmp (IRDevices[bus].version+1,"4.04.35") < 0 && res > 14) res = 14;
				ir_data -> target_mask = res;
				ir_data->target_mask |= (ir_timeout / 2) & (LONG_LEARN_MODE | RECEIVER_SELECT_MASK);
			}
		}

		ir_data -> len = 7;

		res = WriteTransceiverEx (IRDevices + bus,(IRDATA *)ir_data);

		if (res) return (res);
		if (IRDevices[bus].io.if_type == IF_LAN) len = sizeof (IRDATA);

		else {
			res = ReadIRStringEx (IRDevices + bus,&len,1,timeout);

			if (!res) {
				CancelLearnEx (IRDevices + bus);
				return (ERR_TIMEOUT);
			}
		}

	} while (len <= sizeof (IRRAW) - CODE_LENRAW);

	if (IRDevices[bus].io.if_type == IF_LAN) {
		if (ReadLearndataLAN (IRDevices + bus,(byte *)ir_data,timeout)) return (ERR_TIMEOUT);
	}
	else {
		if (ReadIRStringEx (IRDevices + bus,&ir_data->checksumme,len-1,200) != len-1) return (ERR_TIMEOUT);
	}
	ir_data -> data[ir_data -> ir_length] = 0;

	swap_irdata ((IRDATA *)ir_data,NULL);

	res = 0;
	if ((ir_data->command & 0xf0) > 0x10) res = ((ir_data->command-RCV_STATUS_LENGTH) / 16) + ERR_LEARN_LENGTH;

	ir_data->command = 0;
	
	if (carrmeas) {
		if (ir_data->transmit_freq < 10) {
			ir_data->transmit_freq = 38;									// Carriermessung Timeout -> 38kHz Default
			ir_data->command = 0xfe;
		}
		else ir_data->command = 0xff;										// Messung eintragen
	}

	else {
		if (carrier && ir_data->transmit_freq != 255) ir_data->transmit_freq = carrier;
	}

	return (res);
}

int LearnRawIRRepeatEx (IRRAW *ir_data,word addr,word timeout,word ir_timeout,byte carrier)
{
	int res;
	byte len,carrmeas = 0;
	int bus;

	do {
		ir_data -> address = 0;
		if ((addr & 0x7f) <= 15)  ir_data -> address |= addr;
		if ((addr & 0x7f) == 'L') ir_data -> address |= ADRESS_LOCAL;
		if ((addr & 0x7f) == '*') ir_data -> address |= ADRESS_ALL;

		bus = (addr >> 8) & (MAX_IR_DEVICES - 1);

		ir_data -> command = HOST_LEARNIRRAWREPEAT;

		if (addr & 0x80) {
			ir_data->ir_length = 0;
			ir_data->target_mask = ir_timeout;			// Neue Lernmodi

			if ((((ir_timeout & LRN_RCV_SELECT) / 64) & 7) == 6) carrmeas = 1;		// Carriermessung gewählt
		}

		else {
			if (strcmp (IRDevices[bus].version+1,"5.20.01") >= 0) {
				ir_data -> target_mask = ir_timeout & 15;
				if (ir_timeout & 32) ir_data -> target_mask |= LONG_LEARN_MODE;
				if ((IRDevices[bus].extended_mode_ex[0] & 7) == 6 && (IRDevices[bus].extended_mode & STANDARD_RCV)) carrmeas = 1;
				if ((IRDevices[bus].extended_mode_ex[0] & 0x70) == 0x60 && (IRDevices[bus].extended_mode & HI_RCV)) carrmeas = 1;
			}
			else {
				res = ir_timeout & 15;
				if (strcmp (IRDevices[bus].version+1,"4.04.35") < 0 && res > 14) res = 14;
				ir_data -> target_mask = res;
				ir_data->target_mask |= (ir_timeout / 2) & (LONG_LEARN_MODE | RECEIVER_SELECT_MASK);
			}
		}

		ir_data -> len = 7;

		res = WriteTransceiverEx (IRDevices + bus,(IRDATA *)ir_data);
		if (res) return (res);
		if (IRDevices[bus].io.if_type == IF_LAN) len = sizeof (IRDATA);

		else {
			res = ReadIRStringEx (IRDevices + bus,&len,1,timeout);

			if (!res) {
				CancelLearnEx (IRDevices + bus);
				return (ERR_TIMEOUT);
			}
		}

	} while (len <= sizeof (IRRAW) - CODE_LENRAW);

	if (IRDevices[bus].io.if_type == IF_LAN) {
		if (ReadLearndataLAN (IRDevices + bus,(byte *)ir_data,timeout)) return (ERR_TIMEOUT);
	}
	else {
		if (ReadIRStringEx (IRDevices + bus,&ir_data->checksumme,len-1,200) != len-1) return (ERR_TIMEOUT);
	}
	ir_data -> data[ir_data -> ir_length] = 0;

	swap_irdata ((IRDATA *)ir_data,NULL);

	res = 0;
	if ((ir_data->command & 0xf0) > 0x10) res = ((ir_data->command-RCV_STATUS_LENGTH) / 16) + ERR_LEARN_LENGTH;

	ir_data->command = 0;
	
	if (carrmeas) {
		if (ir_data->transmit_freq < 10) {
			ir_data->transmit_freq = 38;									// Carriermessung Timeout -> 38kHz Default
			ir_data->command = 0xfe;
		}
		else ir_data->command = 0xff;										// Messung eintragen
	}

	else {
		if (carrier && ir_data->transmit_freq != 255) ir_data->transmit_freq = carrier;
	}

	return (res);
}

void CorrectIRTimings (IRDATA *ir_data)
{
	int i;

	if (ir_data->transmit_freq >= 48 && ir_data->transmit_freq <= 100) {
		for (i=0;i < ir_data->time_cnt;i++) {
			ir_data->pulse_len[i] += 4;
			ir_data->pause_len[i] -= 4;
		}
	}
}

void CorrectIRTimingsRAW (IRRAW *ir_data)
{
	int i = 0;
	word pulse,pause;
	
	if (ir_data->transmit_freq >= 48 && ir_data->transmit_freq <= 100) {

		while (i < ir_data -> ir_length) {
			pulse = ir_data ->data [i++];
			if (pulse) ir_data ->data [i-1] += 4;
			else {
				pulse = ir_data ->data [i++] << 8;
				pulse |= ir_data->data [i++];
				pulse += 4;
				ir_data->data[i-2] = (pulse >> 8);
				ir_data->data[i-1] = pulse & 0xff;
			}
			pause = ir_data ->data [i++];
			if (pause) ir_data ->data [i-1] -= 4;
			else {
				pause = ir_data ->data[i++] << 8;
				pause |= ir_data->data[i++];
				pause -= 4;
				ir_data->data[i-2] = (pause >> 8);
				ir_data->data[i-1] = pause & 0xff;
			}
		}
	}
}

void PrintPulseData (IRDATA *ir_data)
{
	int i,frq;
	char msg[256];

	frq = ir_data->transmit_freq;
	
	if (frq == 255) frq = 1000;
	else if (frq & 128) frq = (frq & 127) * 4;
 
	if (ir_data->mode == TIMECOUNT_18) {
		sprintf (msg,"Timecount18 Mode\n");
		log_print (msg,LOG_INFO);
		for (i=0;i < TIME_LEN_18;i++) {
			sprintf (msg,"TIME: %2d %4d  %4d\n",i,((IRDATA_18 *)ir_data)->pulse_len[i] * 8,((IRDATA_18 *)ir_data)->pause_len[i] * 8);
			log_print (msg,LOG_INFO);
		}
		sprintf (msg,"T.REP   :   %4d\n",((IRDATA_18 *)ir_data)->repeat_pause);
		log_print (msg,LOG_INFO);
		sprintf (msg,"REP/MODE:   %4d %d\n",((IRDATA_18 *)ir_data)->ir_repeat,ir_data -> mode);
		log_print (msg,LOG_INFO);
		sprintf (msg,"FREQ    :   %4d\n",frq);
		log_print (msg,LOG_INFO);
		sprintf (msg,"DATA    :   %s\n",((IRDATA_18 *)ir_data)->data);
		log_print (msg,LOG_INFO);
		sprintf (msg,"LEN     :   %4d\n",((IRDATA_18 *)ir_data)->ir_length);
		log_print (msg,LOG_INFO);
		sprintf (msg,"ADR     :   %d\n",((IRDATA_18 *)ir_data) -> address);
		log_print (msg,LOG_INFO);
		sprintf (msg,"MASK    :   %d\n",((IRDATA_18 *)ir_data) -> target_mask);
		log_print (msg,LOG_INFO);
	}
	else if ((ir_data->mode & SPECIAL_IR_MODE) == PULSE_200) {
		sprintf (msg,"PULSE:    %4d\n",((IRDATA_SINGLE *)ir_data)->single_len * 8);
		log_print (msg,LOG_INFO);
		for (i=0;i < TIME_LEN_SINGLE;i++) {
			sprintf (msg,"PAUSE: %2d %4d\n",i,((IRDATA_SINGLE *)ir_data)->multi_len[i] * 8);
			log_print (msg,LOG_INFO);
		}
		sprintf (msg,"T.REP   :   %4d\n",((IRDATA_SINGLE *)ir_data)->repeat_pause);
		log_print (msg,LOG_INFO);
		sprintf (msg,"REP/MODE:   %4d %d\n",((IRDATA_SINGLE *)ir_data)->ir_repeat,ir_data -> mode);
		log_print (msg,LOG_INFO);
		sprintf (msg,"FREQ    :   %4d\n",frq);
		log_print (msg,LOG_INFO);
		sprintf (msg,"DATA    :   %s\n",((IRDATA_SINGLE *)ir_data)->data);
		log_print (msg,LOG_INFO);
		sprintf (msg,"LEN     :   %4d\n",((IRDATA_SINGLE *)ir_data)->ir_length);
		log_print (msg,LOG_INFO);
		sprintf (msg,"ADR     :   %d\n",((IRDATA_SINGLE *)ir_data) -> address);
		log_print (msg,LOG_INFO);
		sprintf (msg,"MASK    :   %d\n",((IRDATA_SINGLE *)ir_data) -> target_mask);
		log_print (msg,LOG_INFO);
	}
	else {
		sprintf (msg,"TIME CNT:   %d\n",ir_data->time_cnt);
		log_print (msg,LOG_INFO);
		for (i=0;i < TIME_LEN;i++) {
			sprintf (msg,"TIME : %d %4d  %4d\n",i,ir_data -> pulse_len[i] * 8,ir_data -> pause_len[i] * 8);
			log_print (msg,LOG_INFO);
		}
		sprintf (msg,"T.REP   :   %4d\n",ir_data->repeat_pause);
		log_print (msg,LOG_INFO);
		sprintf (msg,"REP/MODE:   %4d %d\n",ir_data->ir_repeat,ir_data -> mode);
		log_print (msg,LOG_INFO);
		sprintf (msg,"FREQ    :   %4d\n",frq);
		log_print (msg,LOG_INFO);
		if (ir_data->data[0] & 128) {
			for (i=0;i < ir_data->ir_length;i++) {
				sprintf (msg,"DATA [%03d]:%x  -  %x\n",i,ir_data->data[i] & 0x77,ir_data->data[i]);
				log_print (msg,LOG_INFO);
			}
		}

		else {
			if (ir_data->data[0] < '0') {
				for (i=0;i < ir_data->data[0];i++) {
					sprintf (msg,"OFFSET  :   %d\n",ir_data->data[i]);
					log_print (msg,LOG_INFO);
				}
			sprintf (msg,   "DATA    :   %s\n",ir_data->data + i);
			}
			else sprintf (msg,   "DATA    :   %s\n",ir_data->data);
			log_print (msg,LOG_INFO);
		}
		sprintf (msg,"LEN     :   %4d\n",ir_data->ir_length);
		log_print (msg,LOG_INFO);
		sprintf (msg,"ADR     :   %d\n",ir_data -> address);
		log_print (msg,LOG_INFO);
		sprintf (msg,"MASK    :   %d\n",ir_data -> target_mask);
		log_print (msg,LOG_INFO);
	}

}


void PrintRawData (IRRAW *ir_data)
{
	int i,frq;
	char msg[256];
	word pulse,pause;

	frq = ir_data->transmit_freq;
	
	if (frq == 255) frq = 455;
	else if (frq & 128) frq = (frq & 127) * 4;
	sprintf (msg,"MODE    :   %d\n",ir_data -> mode & 0xf0);
	log_print (msg,LOG_INFO);
	sprintf (msg,"Repeat  :   %d\n",ir_data -> mode & 0xf);
	log_print (msg,LOG_INFO);
	sprintf (msg,"FREQ    :   %4d\n",frq);
	log_print (msg,LOG_INFO);
	sprintf (msg,"LEN     :   %4d\n",ir_data->ir_length);
	log_print (msg,LOG_INFO);
	sprintf (msg,"ADR     :   %d\n",ir_data -> address);
	log_print (msg,LOG_INFO);

	i = 0;
	
	while (i < ir_data -> ir_length) {
		pulse = ir_data ->data [i++];
		if (!pulse) {
			pulse = ir_data ->data [i++] << 8;
			pulse |= ir_data->data [i++];
		}
		pause = ir_data ->data [i++];
		if (!pause) {
			pause = ir_data ->data[i++] << 8;
			pause |= ir_data->data[i++];
		}
		sprintf (msg,"%5d %5d\n",pulse * 8,pause * 8);
		log_print (msg,LOG_INFO);
	}


}

void PrintCommand (IRDATA *ir_data)
{
	char msg[256];
	
	if ((ir_data->mode & SPECIAL_IR_MODE) == PULSE_200) sprintf (msg,"DATA    :   %s\n",((IRDATA_SINGLE *)ir_data)->data);
	else sprintf (msg,"DATA    :   %s\n",ir_data->data);
	log_print (msg,LOG_INFO);

}

#ifdef LINUX

typedef void (*sighandler_t)(int); 

void ShutdownHandler (void)
{
	mode_flag |= NO_RECONNECT;

	if (!(mode_flag & NO_CLOCK)) LCDTimeCommand (LCD_DISPLAYTIME);
	LCDBrightness (4);
	if (pidfile[0]) unlink (pidfile);

	exit (0);
}

#endif


DEVICEINFO IRDevices[MAX_IR_DEVICES];
int device_cnt;

int InitCommunicationEx (char devicesel[])
{
	int i,res;
	char msg[255];

	byteorder = GetByteorder ();

	res = get_devices (devicesel,0);

	if (res) return (res);

	raw_repeat = 1;
	if (device_cnt)	time_len = IRDevices[0].io.time_len;
	else time_len = TIME_LEN;

	for (i=0;i<device_cnt;i++) {
		sprintf (msg,"Name   : %s\n",IRDevices[i].name);
		log_print (msg,LOG_INFO);
		sprintf (msg,"Version: %s\n",IRDevices[i].version);
		log_print (msg,LOG_INFO);
		sprintf (msg,"FW SNo : %d\n",IRDevices[i].fw_serno);
		log_print (msg,LOG_INFO);
		sprintf (msg,"Capab  : %s\n",IRDevices[i].cap_string);
		log_print (msg,LOG_INFO);
		sprintf (msg,"FW Cap : 0x%x\n",IRDevices[i].fw_capabilities);
		log_print (msg,LOG_INFO);
		if (IRDevices[i].fw_capabilities2 & FN2_FUNCTIONVAL2) {
			sprintf (msg,"FW Cap2: 0x%x\n",IRDevices[i].fw_capabilities2);
			log_print (msg,LOG_INFO);
		}
		sprintf (msg,"USB SNo: %s\n",IRDevices[i].usb_serno);
		log_print (msg,LOG_INFO);
		sprintf (msg,"Node   : %s\n\n",IRDevices[i].device_node);
		log_print (msg,LOG_INFO);
	}
		
	for (i=0;i<device_cnt;i++) {
		raw_repeat &= IRDevices[i].io.raw_repeat;
		if (IRDevices[i].io.time_len != time_len) {
			sprintf (msg,"Devices < Version 4.xx.xx and Devices > 4.xx.xx can not be mixed.\nExiting now\n\n");
			log_print (msg,LOG_FATAL);
			return (-1);
		}
	}

	if (new_lcd_flag) display_bus = 0xffff;

	for (i=0;i < device_cnt;i++) if (IRDevices[i].version[0] == 'D' || (IRDevices[i].fw_capabilities & FN_DISPMASK)) {
		display_bus = i;
		break;
	}

#ifdef WIN32
	SetConsoleCtrlHandler (ShutdownHandler,TRUE);
#endif

#ifdef LINUX
	signal (SIGINT,(sighandler_t)ShutdownHandler);
#endif

	return (0);
}


#ifdef WIN32

BOOL WINAPI ShutdownHandler (DWORD type)
{
	mode_flag |= NO_RECONNECT;

	if (!(mode_flag & NO_CLOCK)) LCDTimeCommand (LCD_DISPLAYTIME);
	LCDBrightness (4);

	if (pidfile[0]) _unlink (pidfile);

	exit (0);
}

int GetAvailableDataEx (DEVICEINFO *dev)
{
	if (dev->io.if_type == IF_USB) return GetUSBAvailableEx (dev);
	if (dev->io.if_type == IF_RS232 || dev->io.if_type == IF_SERUSB) return GetSerialAvailableEx (dev);

	return (0);
}

#endif

int ReadInstantReceive (DEVICEINFO *dev,byte pnt[],int len)
{
	word to,cnt = 0;
	
#ifdef WIN32
	if (dev->io.if_type == IF_USB) to = 5;
	else to = 25;
#endif

#ifdef LINUX
	to = 25;
#endif

	cnt = ReadIRStringEx (dev,pnt,len,to);

	if (!cnt) return (0);

	if ((dev->io.inst_receive_mode & 1) && pnt[0] != 255) {
		if (pnt[cnt-1]) {
			cnt += ReadIRStringEx (dev,pnt+cnt,len-cnt,100);
		}
	}
	pnt[cnt] = 0;
	return (cnt);
}


#ifdef LINUX

extern SOCKET irt_bcast[32];
extern int if_count;

#endif


int	get_devices (char sel[],byte testflag)  // Errflag = Continue bei USB Error
{
	DEVICEINFO *dev;
	unsigned int adr;
	int loop,res,i,p,q,autoflag = 0;
	char st[1024],msg[256];
	GETVERSION_LAN_EX2 ver;
	IRDATA_LAN irdlan;
	struct sockaddr_in target,send_adr;

#ifdef LINUX
	fd_set events;
	int maxfd,wait;
	struct timeval tv;
#endif

#ifdef WIN32
	int err,num;
	WORD	wVersionRequired;
	WSADATA	wsaData;
	enum FT_STATUS stat;
	char *buf_pnt[260];
	char buf_value[256][64];
	int numDevs;
#else
	char dst[50];
#endif


	strcpy (st,sel);
	st[strlen (st) + 1] = 0;
	q = p = 0;
	device_cnt = 0;
	if (testflag) {
		printf ("Probing IRTrans Devices ..");
		fflush (stdout);
	}
	while (st[p]) {
		if (testflag) {
			printf (".");
			fflush (stdout);
		}
		while (st[p] && sel[p] != ' ' && st[p] != ';') p++;		st[p] = 0;
#ifdef WIN32
		if ((!strncmp (st + q,"usb",3) || !strncmp (st + q,"USB",3)) && !autoflag) {
			res = LoadUSBLibrary ();
			if (res) {
				if (testflag) {
					q = ++p;
					continue;
				}
				return (ERR_OPENUSB);
			}

			num = 1;
			err = 0;
			do {
				if (err == ERR_USBCOM) {
					F_Reload (0x403,0xfc60);
					F_Reload (0x403,0xfc61);
					Sleep (500);
				}

				autoflag = 1;
				for (i=0;i<256;i++) buf_pnt[i] = buf_value[i];
				buf_pnt[i] = NULL;

				stat = F_ListDevices(buf_pnt,&numDevs,FT_LIST_ALL|FT_OPEN_BY_SERIAL_NUMBER);

				if (FT_SUCCESS(stat)) {
					sprintf (msg,"%d USB Devices found\n",numDevs);
					log_print (msg,LOG_DEBUG);
					for (i=0;i<numDevs && device_cnt < MAX_IR_DEVICES && err != ERR_USBCOM;i++) err = get_detail_deviceinfo (buf_pnt[i],"USB",IF_USB);
				}

				else {
					if (testflag) {
						q = ++p;
					}
					else err = ERR_USBCOM;
				}
				
			} while (err == ERR_USBCOM && num++ < 2);
			
			if (st[q + 3] == ':') sort_ir_devices (st + q + 4);
		}
		else if (!strncmp (st + q,"com",3) || !strncmp (st + q,"COM",3)) {
			res = get_detail_deviceinfo ("",st+q,IF_RS232);
		}
		else if (!strcmp (st + q,"dummy")) res = 0;
#endif
#ifdef LINUX
		//LINUX Autofind USB devices
		if ((!strncmp (st + q,"usb",3) || !strncmp (st + q,"USB",3)) && !autoflag) {
			autoflag = 1;
			res = 0;
			for (i=0;i < 16;i++) {
				sprintf (dst,"/dev/ttyUSB%d",i);
				if (get_detail_deviceinfo ("",dst,IF_USB)) {
					sprintf (dst,"/dev/usb/ttyUSB%d",i);
					if (get_detail_deviceinfo ("",dst,IF_USB)) {
						sprintf (dst,"/dev/tts/USB%d",i);
						if (get_detail_deviceinfo ("",dst,IF_USB)) {
							sprintf (dst,"/dev/usb/tts/%d",i);
							get_detail_deviceinfo ("",dst,IF_USB);
						}
					}
				}
			}
		}
		else if ((!strncmp (st + q,"/dev/usb/tty",12) || !strncmp (st + q,"/dev/ttyUSB",11) || !strncmp (st + q,"/dev/usb/tts",12) ||
				  !strncmp (st + q,"/dev/ttyusb",11) || !strncmp (st + q,"/dev/tts/USB",12)) && !autoflag) {
			if (st[q + strlen (st+q) - 1] == ']') {
				int j,beg,end;
				char base[256];

				i = q + strlen (st+q) - 1;
				while (i >= q && st[i] != '[') i--;
				if (st[i] != '[') return (0); 
				i++;
				j = i;
				while (st[i] >= '0' && st[i] <= '9') i++;
				beg = atoi (st+j);
				i++;
				end = atoi (st+i);
				memset (base,0,sizeof (base));
				memcpy (base,st+q,j-q-1);

				res = 0;
				for (i=beg;i<=end;i++) {
					sprintf (dst,"%s%d",base,i);
					get_detail_deviceinfo ("",dst,IF_USB);
				}

			}
			else res = get_detail_deviceinfo ("",st+q,IF_USB);
		}
		else if (!strncmp (st + q,"/dev/tty",8)) res = get_detail_deviceinfo ("",st+q,IF_RS232);
		else if (!strcmp (st + q,"dummy")) res = 0;
#endif
		else if (st[q] >= '1' && st[q] <= '9') {
	
			dev = IRDevices + device_cnt;
			
			strcpy (dev->device_node,st+q);
			dev->io.if_type = IF_LAN;
			dev->usb_serno[0] = 0;
#ifdef WIN32
			dev->io.comport = NULL;
			dev->io.event = NULL;
#endif

#ifdef LINUX
			dev->io.comport = 0;
			dev->io.event = 0;
#endif

			memset (&dev->io.IPAddr,0,sizeof (dev->io.IPAddr));
			dev->io.IPAddr[0].sin_family = AF_INET;

			adr = inet_addr(dev->device_node);
			// Hier Erweiterung auf Devicenamen
/*
			haddr = gethostbyaddr ((char *)&adr,4,AF_INET);
			if (haddr == NULL) {
				sprintf (msg,"\nCan not open device %s.\nAborting ...\n\n",st+q);
				log_print (msg,LOG_FATAL);
				exit (-1);
			}
*/
			dev->io.IPAddr[0].sin_addr.s_addr = adr;
			dev->io.IPAddr[0].sin_port = htons ((word)IRTRANS_PORT);

			res = GetTransceiverVersionEx (dev);
			if (res) return (ERR_OPEN);
			else device_cnt++;
			
		}
		else if ((!strncmp (st + q,"lan",3) || !strncmp (st + q,"LAN",3))) {
			// Find LAN Devices ...

			if (testflag) {
#ifdef WIN32
				wVersionRequired = MAKEWORD(2,2);
				err = WSAStartup(wVersionRequired, &wsaData);
				if (err != 0) exit(1);
#endif
				OpenIRTransLANSocket ();
#ifdef LINUX
				OpenIRTransBroadcastSockets ();
#endif
			}

			memset (&irdlan,0,sizeof (irdlan));
			irdlan.netcommand = COMMAND_LAN;
			irdlan.ir_data.command = HOST_VERSION;
			irdlan.ir_data.len = 8;

			irdlan.ir_data.address = VERSION_MAGIC_1;
			irdlan.ir_data.target_mask = VERSION_MAGIC_2;
			irdlan.ir_data.ir_length = VERSION_MAGIC_3;
			irdlan.ir_data.transmit_freq = VERSION_MAGIC_4;

			irdlan.ir_data.checksumme = get_checksumme (&irdlan.ir_data);

			memset (&target,0,sizeof (struct sockaddr));
			target.sin_family = AF_INET;
			target.sin_addr.s_addr = INADDR_BROADCAST;
			target.sin_port = htons ((word)IRTRANS_PORT);

#ifdef LINUX
			for (i=0;i < if_count;i++) {
				res = send (irt_bcast[i],(char *)&irdlan,irdlan.ir_data.len + sizeof (IRDATA_LAN) - sizeof (IRDATA),MSG_NOSIGNAL);
			}
#else
			if (res = connect (irtlan_outbound,(struct sockaddr *)&target,sizeof (struct sockaddr_in)) < 0) return (ERR_BINDSOCKET);
			res = send (irtlan_outbound,(char *)&irdlan,irdlan.ir_data.len + sizeof (IRDATA_LAN) - sizeof (IRDATA),MSG_NOSIGNAL);
#endif

#ifdef WIN32
			IrtLanEvent = WSACreateEvent ();
			WSAEventSelect (irtlan_socket, IrtLanEvent,FD_READ);
#endif
			loop = 0;
			while (loop++ < 3) {
#ifdef WIN32
				res = WaitForSingleObject (IrtLanEvent,1000);
				if (res == WAIT_TIMEOUT) break;
				WSAResetEvent (IrtLanEvent);

#endif

#ifdef LINUX
				FD_ZERO (&events);

				FD_SET (irtlan_socket,&events);
				maxfd = irtlan_socket + 1;

				tv.tv_sec = 1;
				tv.tv_usec = 0;

				wait = select (maxfd,&events,NULL,NULL,&tv);
				if (!wait) break;
#endif

				i = sizeof (send_adr);
				res = recvfrom(irtlan_socket,(byte *)&ver,sizeof (GETVERSION_LAN_EX2),MSG_NOSIGNAL,(struct sockaddr *)&send_adr,&i);

				if (res == ver.len + 1 && ver.netcommand == RESULT_GETVERSION) {
					dev = IRDevices + device_cnt;
					dev->io.if_type = IF_LAN;
					strcpy (dev->name,"IRTrans Ethernet");
					memcpy (dev->version,ver.ir_version,8);
					memcpy (dev->lan_version,ver.lan_version,8);
					dev->fw_capabilities = ver.ir_capabilities;
					dev->fw_serno = ver.ir_serno;
					memcpy (dev->mac_adr,ver.mac_adr,6);
					dev->cap_string[0] = 0;
					swap_int (&dev->fw_serno);
					swap_int (&dev->fw_capabilities);

					if (res == sizeof (GETVERSION_LAN_EX2)) {
						dev->extended_mode = ver.extended_mode;
						dev->extended_mode2 = ver.extended_mode2;
						memcpy (&dev->extended_mode_ex,&ver.extended_mode_ex,2);
					}

					if (res >= sizeof (GETVERSION_LAN_EX)) {
						dev->fw_capabilities2 = ver.ir_capabilities2;
						dev->fw_capabilities3 = ver.ir_capabilities3;
						dev->fw_capabilities4 = ver.ir_capabilities4;
						swap_int (&dev->fw_capabilities2);
						swap_int (&dev->fw_capabilities3);
						swap_int (&dev->fw_capabilities4);
					}
					else {
						dev->fw_capabilities2 = 0;
						dev->fw_capabilities3 = 0;
						dev->fw_capabilities4 = 0;
					}

#ifdef WIN32
					dev->io.comport = NULL;
					dev->io.event = NULL;
#endif

#ifdef LINUX
					dev->io.comport = 0;
					dev->io.event = 0;
#endif

					memset (&dev->io.IPAddr,0,sizeof (dev->io.IPAddr));
					dev->io.IPAddr[0].sin_family = AF_INET;

					dev->io.IPAddr[0].sin_addr.s_addr = send_adr.sin_addr.s_addr;
					dev->io.IPAddr[0].sin_port = htons ((word)IRTRANS_PORT);

					DecodeFunctions (dev);

					device_cnt++;
				}
			}
			res = 0;
		}
		else if (st[q] == ':') sort_ir_devices (st + q + 1);
		if (res && !testflag) {
			return (ERR_OPEN);
		}
		q = ++p;
	}

	if (testflag) {
		printf ("\n\n");
		if (!device_cnt) {
			sprintf (st,"No IRTrans Devices found.\nAborting ...\n\n");
			log_print (st,LOG_FATAL);
			return (ERR_OPEN);
		}
		else {
			for (i=0;i<device_cnt;i++) {
				sprintf (st,"Name   : %s\n",IRDevices[i].name);
				log_print (st,LOG_INFO);
				sprintf (st,"Version: %s\n",IRDevices[i].version);
				log_print (st,LOG_INFO);
				sprintf (st,"FW SNo : %d\n",IRDevices[i].fw_serno);
				log_print (st,LOG_INFO);
				sprintf (st,"Capab  : %s\n",IRDevices[i].cap_string);
				log_print (st,LOG_INFO);
				sprintf (st,"FW Cap : %x\n",IRDevices[i].fw_capabilities);
				log_print (st,LOG_INFO);
				if (IRDevices[i].io.if_type == IF_USB) {
					sprintf (st,"USB SNo: %s\n",IRDevices[i].usb_serno);
					log_print (st,LOG_INFO);
					sprintf (st,"Node   : %s\n\n",IRDevices[i].device_node);
					log_print (st,LOG_INFO);
				}
				if (IRDevices[i].io.if_type == IF_LAN) {
					sprintf (st,"LAN Ver: %s\n",IRDevices[i].lan_version);
					log_print (st,LOG_INFO);
					sprintf (st,"Mac Adr: %s\n",IRDevices[i].usb_serno);
					log_print (st,LOG_INFO);
					sprintf (st,"IP Adr : %s\n\n",IRDevices[i].device_node);
					log_print (st,LOG_INFO);
				}
			}
		}
	}

	return (0);
}

void sort_ir_devices (char selstring[])
{
	int i,cnt,start,p,q;
	DEVICEINFO di[MAX_IR_DEVICES];

	memcpy (di,IRDevices,sizeof (DEVICEINFO) * MAX_IR_DEVICES);

	i = 0;
	while ((di[i].io.if_type == IF_RS232 || di[i].io.if_type == IF_SERUSB) && i < device_cnt) i++;
	start = cnt = i;
	p = q = 0;
	while (selstring[p]) {
		while (selstring[p] && selstring[p] != ' ' && selstring[p] != ',') p++;
		selstring[p] = 0;
		for (i = start;i < device_cnt;i++) {
			if (!strcmp (selstring+q,di[i].usb_serno) || (atoi (selstring+q) && (word)atoi (selstring+q) == di[i].fw_serno)) {
				memcpy(&IRDevices[cnt++],&di[i],sizeof (DEVICEINFO));
			}
		}
		q = ++p;
	}
	device_cnt = cnt;

}

int get_detail_deviceinfo (char serno[],char devnode[],byte if_type)
{
	int res;
	DEVICEINFO *dev;
	DWORD deviceID;
	char SerialNumber[16];
	char Description[64],msg[512];

#ifdef WIN32
	FT_DEVICE ftDevice;
	FT_STATUS ftStatus;
	enum FT_STATS stat;

	dev = IRDevices + device_cnt;
	strcpy (dev->device_node,devnode);
	dev->io.if_type = if_type;
	if (if_type == IF_USB) {
		if (memcmp (serno,"MM",2) && memcmp (serno,"IR",2)) return (ERR_OPEN);


		strcpy (dev->usb_serno,serno);
		stat = F_OpenEx(dev->usb_serno,FT_OPEN_BY_SERIAL_NUMBER,&(dev->io.usbport));

		if (!FT_SUCCESS(stat)) return (ERR_OPEN);

		ftStatus = F_GetDeviceInfo(dev->io.usbport,&ftDevice,&deviceID,SerialNumber,Description,NULL);

		if (deviceID != 0x0403fc60 && deviceID != 0x0403fc61) {
			F_Close (dev->io.usbport);
			return (ERR_OPEN);
		}

		if (!strcmp (Description,"IRTrans USB A")) {
			sprintf (msg,"Wrong USB Device found: %s\n",Description);
			log_print (msg,LOG_DEBUG);
			F_Close (dev->io.usbport);
			return (ERR_OPEN);
		}
	}

	else {
		if (if_type == IF_SERUSB) res = OpenSerialPortEx (devnode,&(dev->io.comport),250);
		else res = OpenSerialPortEx (devnode,&(dev->io.comport),1000);
		if (res) return (ERR_OPEN);
	}
	dev->io.event = CreateEvent (NULL,TRUE,FALSE,NULL);
#endif

#ifdef LINUX
	sprintf (msg,"Opening Device: %s\n",devnode);
	log_print (msg,LOG_DEBUG);

	dev = IRDevices + device_cnt;
	dev->usb_serno[0] = 0;
	strcpy (dev->device_node,devnode);
	dev->io.if_type = if_type;

	res = OpenSerialPortEx (devnode,&(dev->io.comport),0);
	if (res) return (ERR_OPEN);
#endif

	dev->io.io_sequence = 120;
	
	res = GetTransceiverVersionEx (dev);
	if (res) {
#ifdef WIN32
		if (dev->io.if_type == IF_USB) F_Close (dev->io.usbport);
		CloseHandle (dev->io.comport);
#else
		close (dev->io.comport);
#endif
		if (dev->io.if_type == IF_USB) return (ERR_USBCOM);
		return (ERR_OPEN);
	}
	
	device_cnt++;
	return (0);
}

int GetWiFiStatusEx (WLANBUFFER *buf,DEVICEINFO *dev)
{
	int i,res,cnt;
	byte data[512];
	IRDATA ir;

	ir.command = HOST_VERSION;
	ir.len = 8;

	ir.address = VERSION_MAGIC_1;
	ir.target_mask = VERSION_MAGIC_2;
	ir.ir_length = VERSION_MAGIC_5;
	ir.transmit_freq = VERSION_MAGIC_4;

	ir.checksumme = get_checksumme (&ir);

	FlushIoEx (dev);

	if (dev->io.if_type == IF_LAN) return (ERR_SSID_WLAN);

	res = WriteTransceiverEx (dev,&ir);

	res = ReadIRStringEx (dev,data,512,8000);
	if (!res) return (ERR_READVERSION);

	cnt = 0;
	memset (buf,0,sizeof (WLANBUFFER));
	for (i=1;i < data[0];) {
		strncpy (buf->WLAN_SSID[cnt],data + i,32);
		i += strlen (data + i);
		i++;
		buf->WLAN_type[cnt] = data[i++];
		buf->WLAN_rssi[cnt] = data[i++];
		i++;
		cnt++;
	}
	buf->statustype = STATUS_SSIDLIST;
	buf->statuslen = sizeof (WLANBUFFER);
	buf->count_buffer = cnt;
	buf->count_total = cnt;
	buf->offset = 0;
	return (0);
}


int GetTransceiverVersionEx (DEVICEINFO *dev)
{
	int i,res;
	char data[100];
	IRDATA ir;
	GETVERSION_LAN_EX2 ver;
	struct sockaddr_in send_adr;

#ifdef LINUX
	fd_set events;
	int maxfd,wait;
	struct timeval tv;
#endif

	if (dev->io.if_type == IF_RS232) {
		ir.command = HOST_VERSION;
		ir.len = 6;

		ir.address = VERSION_MAGIC_1;
		ir.target_mask = VERSION_MAGIC_2;

		ir.checksumme = get_checksumme (&ir);

		FlushIoEx (dev);
		res = WriteTransceiverEx (dev,&ir);
		FlushIoEx (dev);
	}

	ir.command = HOST_VERSION;
	ir.len = 8;

	ir.address = VERSION_MAGIC_1;
	ir.target_mask = VERSION_MAGIC_2;
	ir.ir_length = VERSION_MAGIC_3;
	ir.transmit_freq = VERSION_MAGIC_4;

	ir.checksumme = get_checksumme (&ir);

	FlushIoEx (dev);

	dev->version[0] = 0;


	if (dev->io.if_type == IF_LAN) while (1) {
		res = WriteTransceiverEx (dev,&ir);
		dev->version[8] = 0;
		dev->usb_serno[0] = 0;
#ifdef WIN32
		res = WaitForSingleObject (IrtLanEvent,2000);
		if (res == WAIT_TIMEOUT)  {
#endif

#ifdef LINUX
		FD_ZERO (&events);

		FD_SET (irtlan_socket,&events);
		maxfd = irtlan_socket + 1;

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		wait = select (maxfd,&events,NULL,NULL,&tv);
		if (!wait) {
#endif
			if (!(mode_flag & NO_INIT_LAN) || dev == IRDevices)	return (ERR_VERSION);
			memcpy (dev->version,IRDevices[0].version,8);
			memcpy (dev->lan_version,IRDevices[0].lan_version,8);
			dev->fw_capabilities = IRDevices[0].fw_capabilities;
			dev->fw_capabilities2 = IRDevices[0].fw_capabilities2;
			dev->fw_capabilities3 = IRDevices[0].fw_capabilities3;
			dev->fw_capabilities4 = IRDevices[0].fw_capabilities4;
			dev->extended_mode = IRDevices[0].extended_mode;
			dev->extended_mode2 = IRDevices[0].extended_mode2;
			memcpy (&dev->extended_mode_ex,&IRDevices[0].extended_mode,2);
			dev->io.ext_carrier = IRDevices[0].io.ext_carrier;
			dev->io.inst_receive_mode = IRDevices[0].io.inst_receive_mode;
			dev->io.raw_repeat = IRDevices[0].io.raw_repeat;
			dev->io.time_len = IRDevices[0].io.time_len;
			dev->io.toggle_support = IRDevices[0].io.toggle_support;
			dev->fw_serno = 0;
			memset (dev->mac_adr,0,6);
			break;
		}
		else {
#ifdef WIN32
			WSAResetEvent (IrtLanEvent);
#endif
			i = sizeof (send_adr);
			res = recvfrom(irtlan_socket,(byte *)&ver,sizeof (GETVERSION_LAN_EX2),MSG_NOSIGNAL,(struct sockaddr *)&send_adr,&i);
			if (res == ver.len + 1 && ver.netcommand == RESULT_GETVERSION) {
				dev = IRDevices + device_cnt;

				dev->io.if_type = IF_LAN;
				strcpy (dev->name,"IRTrans Ethernet");
				memcpy (dev->version,ver.ir_version,8);
				memcpy (dev->lan_version,ver.lan_version,8);
				dev->fw_capabilities = ver.ir_capabilities;
				dev->fw_serno = ver.ir_serno;
				memcpy (dev->mac_adr,ver.mac_adr,6);
				dev->cap_string[0] = 0;
				swap_int (&dev->fw_serno);
				swap_int (&dev->fw_capabilities);

				if (res == sizeof (GETVERSION_LAN_EX2)) {
					dev->extended_mode = ver.extended_mode;
					dev->extended_mode2 = ver.extended_mode2;
					memcpy (&dev->extended_mode_ex,&ver.extended_mode_ex,2);
				}

				if (res >= sizeof (GETVERSION_LAN_EX)) {
					dev->fw_capabilities2 = ver.ir_capabilities2;
					dev->fw_capabilities3 = ver.ir_capabilities3;
					dev->fw_capabilities4 = ver.ir_capabilities4;
					swap_int (&dev->fw_capabilities2);
					swap_int (&dev->fw_capabilities3);
					swap_int (&dev->fw_capabilities4);
				}
				else {
					dev->fw_capabilities2 = 0;
					dev->fw_capabilities3 = 0;
					dev->fw_capabilities4 = 0;
				}
				break;
			}
		}
	}

	// LAN VERSION LENGTH !!
	// Bus Module !!

	else {
		res = WriteTransceiverEx (dev,&ir);
		dev->version[8] = 0;
		res = ReadIRStringEx (dev,dev->version,8,500);
		if (!res) {										// Retry bei Sequence Error
			WriteTransceiverEx (dev,&ir);
			res = ReadIRStringEx (dev,dev->version,8,500);
		}
		if (res != 8) return (ERR_READVERSION);

		if (strcmp (dev->version+1,MINIMUM_SW_VERSION) < 0) return (ERR_VERSION);
		
		if (strcmp (dev->version+1,"5.10.01") >= 0) {
			res = ReadIRStringEx (dev,data,24,500);
			if (res != 20 && res != 24) return (ERR_READVERSION);

			memcpy (&dev->fw_capabilities,data,4);
			swap_int (&dev->fw_capabilities);
			memcpy (&dev->fw_capabilities2,data+4,4);
			swap_int (&dev->fw_capabilities2);
			memcpy (&dev->fw_capabilities3,data+8,4);
			swap_int (&dev->fw_capabilities3);
			memcpy (&dev->fw_capabilities4,data+12,4);
			swap_int (&dev->fw_capabilities4);
			
			memcpy (&dev->fw_serno,data + 16,4);
			swap_int (&dev->fw_serno);
			
			if (res == 24) memcpy (&dev->extended_mode,data + 20,4);
			else memset (&dev->extended_mode,0,4);
			
			if (dev->fw_serno == 2802 || dev->fw_serno == 0x11223344) dev->fw_serno = 0;
		}

		else if (strcmp (dev->version+1,"2.24.01") >= 0) {
			res = ReadIRStringEx (dev,data,8,500);
			if (res != 8) return (ERR_READVERSION);

			if (strcmp (dev->version+1,"4.04.01") >= 0) {
				memcpy (&dev->fw_capabilities,data,4);
				swap_int (&dev->fw_capabilities);
				memcpy (&dev->fw_serno,data + 4,4);
				swap_int (&dev->fw_serno);
			}
			else {
				memcpy (&dev->fw_capabilities,data + 1,2);
				swap_int (&dev->fw_capabilities);
				memcpy (&dev->fw_serno,data + 3,4);
				swap_int (&dev->fw_serno);
			}
			if (dev->fw_serno == 2802 || dev->fw_serno == 0x11223344) dev->fw_serno = 0;

		}
	}

	DecodeFunctions (dev);

	return (0);
}

void DecodeFunctions (DEVICEINFO *dev)
{
	dev->io.raw_repeat = 0;
	dev->io.ext_carrier = 0;
	dev->io.inst_receive_mode = 0;
	dev->io.advanced_lcd = 0;
	dev->io.toggle_support = 0;

	if (strcmp (dev->version+1,"4.00.00") >= 0) dev->io.time_len = 8;									// Version 4.0 with TIME_LEN = 8
	else dev->io.time_len = 6;
	if (strcmp (dev->version+1,"4.02.00") >= 0) dev->io.raw_repeat = 1;										// Version 4.2 with RAW_REPEAT feature
	if (strcmp (dev->version+1,"4.03.00") >= 0) dev->io.ext_carrier = 1;									// Version 4.3 mit erweitertem Carrier
	if (strcmp (dev->version+1,"4.04.06") >= 0) dev->io.inst_receive_mode |= 1;								// Version 4.4.6 mit geändertem Instant Rcv
	if (strcmp (dev->version+1,"4.04.07") >= 0 && 
		(dev->version[0] == 'D')) dev->io.inst_receive_mode |= 2;											// Version 4.4.7 mit geändertem Instant Rcv
	if (strcmp (dev->version+1,"4.04.17") >= 0 &&
		(dev->version[0] == 'D')) dev->io.advanced_lcd |= 1;												// Version 4.4.17 mit zusätzlichen LCD Funktionen
	if (strcmp (dev->version+1,"4.04.23") >= 0 &&
		(dev->version[0] == 'D')) dev->io.advanced_lcd |= 2;												// Version 4.4.23 mit neuem LCD Protokoll
	
	if (dev->version[0] == 'F') dev->io.advanced_lcd = 7;													// Uneed V2

	if (dev->fw_capabilities & FN_DEBOUNCE) dev->io.inst_receive_mode |= 2;

	if (strcmp (dev->version+1,"6.00.00") >= 0) rcmmflag = 1;												// Version 6.0 mit RCMM Erkennung

	if (strcmp (dev->version+1,"6.01.00") >= 0) dev->io.toggle_support = 1;									// Version 6.1 mit Togglesend Support

	if (strcmp (dev->version+1,"5.05.15") >= 0 && (dev->io.if_type == IF_USB || (
		dev->io.if_type == IF_SERUSB && (dev->version[0] == 'U' || dev->version[0] == 'X' || dev->version[0] == 'D')))) dev->io.io_seq_mode = 1;		// IO Sequence Mode

	switch (dev->version[0]) {
		case 'X':
			strcpy (dev->name,"IRTrans Translator");
			break;
		case 'U':
			strcpy (dev->name,"IRTrans USB");
			break;
		case 'V':
			strcpy (dev->name,"IRTrans USB 455kHz");
			break;
		case 'S':
			strcpy (dev->name,"IRTrans RS232");
			break;
		case 'T':
			strcpy (dev->name,"IRTrans RS232 455kHz");
			break;
		case 'E':
			strcpy (dev->name,"IRTrans LAN");
			break;
		case 'G':
			strcpy (dev->name,"IRTrans LAN 455kHz");
			break;
	}
	dev->cap_string[0] = 0;

	if (dev->fw_capabilities & FN_POWERON) strcat (dev->cap_string,"Power On; ");
	if (dev->fw_capabilities & FN_SOFTID) strcat (dev->cap_string,"Soft ID; ");
	if (dev->fw_capabilities & FN_DUALSND) strcat (dev->cap_string,"Dual Transmitter Drivers; ");
	if (dev->fw_capabilities & FN_IRDB) strcat (dev->cap_string,"IR Database; ");
	if (dev->fw_capabilities & FN_FLASH128) strcat (dev->cap_string,"128KB IR Flash; ");

	if (dev->io.if_type == IF_LAN) {
		sprintf (dev->usb_serno,"%02x-%02x-%02x-%02x-%02x-%02x",dev->mac_adr[0],dev->mac_adr[1],dev->mac_adr[2],dev->mac_adr[3],dev->mac_adr[4],dev->mac_adr[5]);
		sprintf (dev->device_node,"%d.%d.%d.%d",dev->io.IPAddr[0].sin_addr.s_addr & 0xff,(dev->io.IPAddr[0].sin_addr.s_addr >> 8) & 0xff,(dev->io.IPAddr[0].sin_addr.s_addr >> 16) & 0xff,(dev->io.IPAddr[0].sin_addr.s_addr >> 24) & 0xff);
	}
}


void FlushIoEx (DEVICEINFO *dev)
{
#ifdef WIN32
	if (dev->io.if_type == IF_USB) FlushUSBEx (dev->io.usbport);
	else FlushComEx (dev->io.comport);
#endif

#ifdef LINUX
	FlushComEx (dev->io.comport);
#endif

}

void Hexdump_File (IRDATA *ird)
{
	int i;
	byte *pnt;

	pnt = (byte *)ird;
	for (i=0;i < ird->len;i++) {
		fprintf (hexfp,"0x%02x ",pnt[i]);
		if (((i+1)%16) == 0) fprintf (hexfp,"\n");
	}
	fprintf (hexfp,"\n\n");
	fflush (hexfp);
	hexflag = 0;
}

void Hexdump_IO (IRDATA *ird)
{
	int i,j;
	char st[2048],nm[100];
	byte *pnt;

	strcpy (st,"IODUMP:\n000  ");
	pnt = (byte *)ird;
	for (i=0;i < ird->len;i++) {
		sprintf (nm,"%02x ",pnt[i]);
		strcat (st,nm);
		if ((i & 0xf) == 0xf) {
			strcat (st,"  ");
			for (j=i-15; j <= i;j++) {
				if (pnt[j] < 32) 
					strcat (st,".");
				else {
					sprintf (nm,"%c",pnt[j]);
					strcat (st,nm);
				}
			}
			sprintf (nm,"\n%03d  ",i+1);
			strcat (st,nm);
		}
	}
	if ((i & 0xf) != 0) {
		for (j=0;j <= (15 - (i & 15));j++) strcat (st,"   ");
		strcat (st,"  ");
		for (j=i - (i & 15); j < i;j++) {
			if (pnt[j] < 32) 
				strcat (st,".");
			else {
				sprintf (nm,"%c",pnt[j]);
				strcat (st,nm);
			}
		}
	}

	strcat (st,"\n");

	log_print (st,LOG_DEBUG);
}

void Hexdump_Medialon (IRDATA *ird)
{
	int i;
	char st[2048],nm[100];
	byte *pnt;

	st[0] = 0;
	pnt = (byte *)ird;
	for (i=0;i < ird->len;i++) {
		sprintf (nm,"!%02x",pnt[i]);
		strcat (st,nm);
	}
	strcat (st,"\n");

	log_print (st,LOG_DEBUG);
}



int WriteTransceiverEx (DEVICEINFO *dev,IRDATA *src)
{
	byte res = 0,i,pos;
	int count = 0,max,timeo;
	IRDATA_LARGE send;
	byte buffer[1024],sbuffer[1024];

	dev->io.io_sequence++;

	if (dev->version[0] == 0 && src->command != HOST_VERSION) return (ERR_WRONGBUS);

	swap_irdata (src,(IRDATA *)&send);

	if (time_len != TIME_LEN && ((src->command == HOST_SEND && !(src->mode & NON_IRMODE)) || src->command == HOST_LEARNIRQUICK)) {
		if (src->mode & RAW_DATA) {
			if (src->ir_length > OLD_LENRAW) return (ERR_LONGRAW);
		}
		else {
			if (src->time_cnt > 6) return (ERR_LONGDATA);
			ConvertToIRTRANS3 ((IRDATA *)&send);
		}
	}

	if (dev->io.io_seq_mode || (src->command == HOST_VERSION && (dev->io.if_type == IF_USB || dev->io.if_type == IF_SERUSB))) {
		if (dev->version[0]) max = 10;
		else max = 4;
		timeo = 250;
	}
	else {
		if (dev->version[0]) max = 5;
		else max = 2;
		timeo = 500;
	}

	send.checksumme = get_checksumme ((IRDATA *)&send);
	do {
		if (mode_flag & HEXDUMP) Hexdump_IO ((IRDATA *)&send);
		if (mode_flag & MEDIALON) Hexdump_Medialon ((IRDATA *)&send);
		if (hexflag) Hexdump_File ((IRDATA *)&send);

		if (dev->io.if_type == IF_LAN) return (IRTransLanSend (dev,(IRDATA *)&send));

		if (dev->io.io_seq_mode || (src->command == HOST_VERSION && (dev->io.if_type == IF_USB || dev->io.if_type == IF_SERUSB))) {
			memcpy (sbuffer,&send,send.len);
			sbuffer[send.len] = dev->io.io_sequence;

			res = WriteIRStringEx (dev,sbuffer,send.len + 1);
			if (dev->version[0]) max = 10;
			else max = 4;
		}
		else {
			if (dev->version[0]) max = 10;
			else max = 4;
			res = WriteIRStringEx (dev,(byte *)&send,send.len);
		}

		if (res) return (ERR_TIMEOUT);

		*buffer = 0;
		res = ReadIRStringEx (dev,buffer,1,(word)timeo);
		count++;
		if (res != 1 || *buffer != 'O') {
			sprintf (sbuffer,"IRTRans Send status: %d - %d  SEQ:%d  TO:%d\n",res,*buffer,dev->io.io_sequence,timeo);
			log_print (sbuffer,LOG_DEBUG);
			if (res == 1) {
				if (src->command == HOST_VERSION) res = ReadIRStringEx (dev,buffer+1,250,500);
				else res = ReadIRStringEx (dev,buffer+1,250,20);
				if (res <= 1) {
					if (*buffer == 'R') return (ERR_RESEND);
				}
				else {
					res++;
					pos = 0;
					while (pos < res) {
						if (dev->io.receive_buffer_cnt == 4) {
							i = 1;
							while (buffer[pos+i] && buffer[pos+i] != 'E' && buffer[pos+i] != 'O' && buffer[pos+i] != 'E' && (pos+i) < res) i++;
						}
						else {
							dev->io.receive_buffer[dev->io.receive_buffer_cnt][0] = buffer[pos];
							i = 1;
							while (buffer[pos+i] && buffer[pos+i] != 'E' && buffer[pos+i] != 'O' && buffer[pos+i] != 'E' && (pos+i) < res) {
								dev->io.receive_buffer[dev->io.receive_buffer_cnt][i] = buffer[pos+i];
								i++;
							}
							dev->io.receive_buffer[dev->io.receive_buffer_cnt][i] = 0;
							dev->io.receive_cnt[dev->io.receive_buffer_cnt] = i;
						}
						pos += i;
						if (dev->io.receive_buffer_cnt < 4) dev->io.receive_buffer_cnt++;
						if (dev->io.inst_receive_mode & 1 && buffer[pos] == 0) pos++;
					}
					if (buffer[res-1] == 'O') return (0);
					res = ReadIRStringEx (dev,buffer,1,500);
					if (*buffer == 'O') return (0);

				}
			}
			FlushIoEx (dev);
			msSleep (150);
			if (count > 5) msSleep (timeo * 2);
			res = 0;
		}
	} while (res == 0 && count < max && !(mode_flag & NO_RECONNECT));

	sprintf (sbuffer,"IRTRans Send Done: %d\n",count);
	log_print (sbuffer,LOG_DEBUG);

	if (count == 10) return (ERR_TIMEOUT);
	return (0);

}

void ConvertToIRTRANS3 (IRDATA *ird)
{
	IRDATA3 irold;

	memcpy (&irold,ird,21);
	memcpy (irold.pulse_len,ird->pulse_len,12);
	memcpy (&irold.time_cnt,&ird->time_cnt,CODE_LEN + 3);
	irold.len = ird->len - 8;
	memcpy (ird,&irold,irold.len);
}


void SwapStatusbuffer (STATUS_BUFFER *sb)
{
	int i;

	if (!byteorder) return;

	for (i=0;i<16;i++) {
		swap_word (&sb->stat[i].send_mask);
		swap_int ((int32_t *)&sb->stat[i].capabilities);
	}
}


void swap_irdata (IRDATA *src,IRDATA *tar)
{	
	int i;
	IRDATA *ir;

	if (tar) {
		memcpy (tar,src,src->len);
		ir = tar;
	}
	else ir = src;

	if (!byteorder || ir ->command == HOST_SEND_RS232) return;

	swap_word (&ir->target_mask);

	if (ir->mode == TIMECOUNT_18) {
		for (i=0;i < 18;i++) {
			swap_word (((IRDATA_18 *)ir)->pulse_len + i);
			swap_word (((IRDATA_18 *)ir)->pause_len + i);
		}
	}
	else if ((ir->mode & SPECIAL_IR_MODE) == PULSE_200) {
		swap_word (&(((IRDATA_SINGLE *)ir)->single_len));
		for (i=0;i < TIME_LEN_SINGLE;i++) swap_word (((IRDATA_SINGLE *)ir)->multi_len + i);
	}
	else if (!(ir -> mode & (RAW_DATA | LCD_DATA))) {

		for (i=0;i < TIME_LEN;i++) {
			swap_word (ir->pause_len + i);
			swap_word (ir->pulse_len + i);
		}
	}
}


void swap_word (word *pnt)
{
	byte *a,v;

	if (!byteorder) return;

	a = (byte *)pnt;
	v = a[0];
	a[0] = a[1];
	a[1] = v;
}


void swap_int (int32_t *pnt)
{
	byte *a,v;

	if (!byteorder) return;

	a = (byte *)pnt;
	v = a[0];
	a[0] = a[3];
	a[3] = v;

	v = a[1];
	a[1] = a[2];
	a[2] = v;
}


byte get_checksumme (IRDATA *ir)
{
	int i = 2;
	byte cs = 0;
	while (i < ir->len) cs += ((byte *)ir)[i++];
	return (cs);
}


int GetByteorder ()
{
	char arr[2];
	short *pnt;

	pnt = (short *)arr;
	*pnt = 1;

	return (arr[1]);
}


int WriteIRStringEx (DEVICEINFO *dev,byte pnt[],int len)
{
#ifdef WIN32
	if (dev->io.if_type == IF_USB) {
		WriteUSBStringEx (dev,pnt,len);
		return (0);
	}

	return (WriteSerialStringEx (dev,pnt,len));
#endif

#ifdef LINUX
	return WriteSerialStringEx (dev,pnt,len);
#endif
}




int	ReadIRStringEx_ITo (DEVICEINFO *dev,byte pnt[],int len,word timeout)
{

#ifdef WIN32
	if (dev->io.if_type == IF_USB) return (ReadUSBStringEx_ITo (dev,pnt,len,timeout));
	else return (ReadSerialStringEx_ITo (dev,pnt,len,timeout));
#endif

#ifdef LINUX
	return (ReadSerialStringEx (dev,pnt,len,timeout));
#endif
}



int	ReadIRStringEx (DEVICEINFO *dev,byte pnt[],int len,word timeout)
{

#ifdef WIN32
	if (dev->io.if_type == IF_USB) return (ReadUSBStringEx (dev,pnt,len,timeout));
	else return (ReadSerialStringEx (dev,pnt,len,timeout));
#endif

#ifdef LINUX
	return (ReadSerialStringEx (dev,pnt,len,timeout));
#endif
}

void ConvertLCDCharset (byte *pnt)
{
	int i,t;
	if (new_lcd_flag) t = 1;
	else t = 0;
	for (i=0;i < 200;i++) pnt[i] = DispConvTable[1][pnt[i]];
}


void InitConversionTables ()
{
	int i;
	int num;

	num = 0;
	memset (DispConvTable[num],' ',256);					// Initialize Table

	for (i=0;i <= 127;i++) DispConvTable[num][i] = i;		// Set lower 7 Bits

	DispConvTable[num][192] = 65;
	DispConvTable[num][193] = 65;
	DispConvTable[num][194] = 65;
	DispConvTable[num][195] = 65;
	DispConvTable[num][197] = 0x81;
	DispConvTable[num][198] = 0x90;
	DispConvTable[num][199] = 0x99;
	DispConvTable[num][200] = 69;
	DispConvTable[num][201] = 69;
	DispConvTable[num][202] = 69;
	DispConvTable[num][203] = 69;
	DispConvTable[num][204] = 73;
	DispConvTable[num][205] = 73;
	DispConvTable[num][206] = 73;
	DispConvTable[num][207] = 73;
	DispConvTable[num][208] = 68;
	DispConvTable[num][209] = 78;
	DispConvTable[num][210] = 79;
	DispConvTable[num][211] = 79;
	DispConvTable[num][212] = 79;
	DispConvTable[num][213] = 79;
	DispConvTable[num][216] = 0x88;
	DispConvTable[num][217] = 85;
	DispConvTable[num][218] = 85;
	DispConvTable[num][219] = 85;
	DispConvTable[num][221] = 89;
	DispConvTable[num][224] = 97;
	DispConvTable[num][225] = 0x83;
	DispConvTable[num][226] = 97;
	DispConvTable[num][227] = 97;
	DispConvTable[num][229] = 0x84;
	DispConvTable[num][230] = 0x91;
	DispConvTable[num][231] = 0x99;
	DispConvTable[num][232] = 101;
	DispConvTable[num][233] = 101;
	DispConvTable[num][234] = 101;
	DispConvTable[num][235] = 101;
	DispConvTable[num][236] = 105;
	DispConvTable[num][237] = 105;
	DispConvTable[num][238] = 105;
	DispConvTable[num][239] = 105;
	DispConvTable[num][241] = 0xee;
	DispConvTable[num][242] = 111;
	DispConvTable[num][243] = 111;
	DispConvTable[num][244] = 111;
	DispConvTable[num][245] = 111;
	DispConvTable[num][248] = 0x88;
	DispConvTable[num][249] = 117;
	DispConvTable[num][250] = 117;
	DispConvTable[num][251] = 117;
	DispConvTable[num][253] = 121;
	DispConvTable[num][255] = 121;



	DispConvTable[num][0xC4] = 0x80;						// Set Conversion (Upper 8 Bits)
	DispConvTable[num][0xC5] = 0x82;
	DispConvTable[num][0xE1] = 0x83;
	DispConvTable[num][0xE5] = 0x84;
	DispConvTable[num][0xD6] = 0x86;
	DispConvTable[num][0xF6] = 0x87;
	DispConvTable[num][0xD8] = 0x88;
	DispConvTable[num][0xF8] = 0x89;
	DispConvTable[num][0xDC] = 0x8A;
	DispConvTable[num][0xFC] = 0x8B;
	DispConvTable[num][0x5C] = 0x8C;
	DispConvTable[num][0xA5] = 0x5C;
	DispConvTable[num][0xA7] = 0x8F;
	DispConvTable[num][0xC6] = 0x90;
	DispConvTable[num][0xE6] = 0x91;
	DispConvTable[num][0xA3] = 0x92;
	DispConvTable[num][0xA6] = 0x98;
	DispConvTable[num][0xC7] = 0x99;
	DispConvTable[num][0xB0] = 0xDF;
	DispConvTable[num][0xE4] = 0xE1;
	DispConvTable[num][0xDF] = 0xE2;
	DispConvTable[num][0xB5] = 0xE4;
	DispConvTable[num][0xA4] = 0xEB;
	DispConvTable[num][0xA2] = 0xEC;
	DispConvTable[num][0xF1] = 0xEE;
	DispConvTable[num][0xF7] = 0xFD;
	DispConvTable[num][0x94] = 0x94;
	DispConvTable[num][0x95] = 0x95;
	DispConvTable[num][0x96] = 0x96;
	DispConvTable[num][0x97] = 0x97;
	DispConvTable[num][0x9b] = 0x9b;
	DispConvTable[num][0x9c] = 0x9c;
	DispConvTable[num][0x9d] = 0x9d;
	DispConvTable[num][0x9e] = 0x9e;
	DispConvTable[num][0x9f] = 0x9f;

	num = 1;
	memset (DispConvTable[num],' ',256);					// Initialize Table

	for (i=0;i <= 127;i++) DispConvTable[num][i] = i;		// Set lower 7 Bits
	DispConvTable[num][192] = 65;
	DispConvTable[num][193] = 65;
	DispConvTable[num][194] = 65;
	DispConvTable[num][195] = 65;
	DispConvTable[num][197] = 0x81;
	DispConvTable[num][198] = 0x90;
	DispConvTable[num][199] = 0x99;
	DispConvTable[num][200] = 69;
	DispConvTable[num][201] = 69;
	DispConvTable[num][202] = 69;
	DispConvTable[num][203] = 69;
	DispConvTable[num][204] = 73;
	DispConvTable[num][205] = 73;
	DispConvTable[num][206] = 73;
	DispConvTable[num][207] = 73;
	DispConvTable[num][208] = 68;
	DispConvTable[num][209] = 78;
	DispConvTable[num][210] = 79;
	DispConvTable[num][211] = 79;
	DispConvTable[num][212] = 79;
	DispConvTable[num][213] = 79;
	DispConvTable[num][216] = 0x88;
	DispConvTable[num][217] = 85;
	DispConvTable[num][218] = 85;
	DispConvTable[num][219] = 85;
	DispConvTable[num][221] = 89;
	DispConvTable[num][224] = 97;
	DispConvTable[num][225] = 0x83;
	DispConvTable[num][226] = 97;
	DispConvTable[num][227] = 97;
	DispConvTable[num][229] = 0x84;
	DispConvTable[num][230] = 0x91;
	DispConvTable[num][231] = 0x99;
	DispConvTable[num][232] = 101;
	DispConvTable[num][233] = 101;
	DispConvTable[num][234] = 101;
	DispConvTable[num][235] = 101;
	DispConvTable[num][236] = 105;
	DispConvTable[num][237] = 105;
	DispConvTable[num][238] = 105;
	DispConvTable[num][239] = 105;
	DispConvTable[num][241] = 0xee;
	DispConvTable[num][242] = 111;
	DispConvTable[num][243] = 111;
	DispConvTable[num][244] = 111;
	DispConvTable[num][245] = 111;
	DispConvTable[num][248] = 0x88;
	DispConvTable[num][249] = 117;
	DispConvTable[num][250] = 117;
	DispConvTable[num][251] = 117;
	DispConvTable[num][253] = 121;
	DispConvTable[num][255] = 255;



	DispConvTable[num][0xC4] = 0x80;						// Set Conversion (Upper 8 Bits)
	DispConvTable[num][0xC5] = 0x82;
	DispConvTable[num][0xE1] = 0x83;
	DispConvTable[num][0xE5] = 0x84;
	DispConvTable[num][0xD6] = 0x86;
	DispConvTable[num][0xF6] = 0x87;
	DispConvTable[num][0xD8] = 0x88;
	DispConvTable[num][0xF8] = 0x89;
	DispConvTable[num][0xDC] = 0x8A;
	DispConvTable[num][0xFC] = 0x8B;
	DispConvTable[num][0x5C] = 0x8C;
	DispConvTable[num][0xA5] = 0x5C;
	DispConvTable[num][0xA7] = 0x8F;
	DispConvTable[num][0xC6] = 0x90;
	DispConvTable[num][0xE6] = 0x91;
	DispConvTable[num][0xA3] = 0x92;
	DispConvTable[num][0xA6] = 0x98;
	DispConvTable[num][0xC7] = 0x99;
	DispConvTable[num][0xB0] = 0xDF;
	DispConvTable[num][0xE4] = 0xE1;
	DispConvTable[num][0xDF] = 0xE2;
	DispConvTable[num][0xB5] = 0xE4;
	DispConvTable[num][0xA4] = 0xEB;
	DispConvTable[num][0xA2] = 0xEC;
	DispConvTable[num][0xF1] = 0xEE;
	DispConvTable[num][0xF7] = 0xFD;
	DispConvTable[num][0x94] = 0x94;
	DispConvTable[num][0x95] = 0x95;
	DispConvTable[num][0x96] = 0x96;
	DispConvTable[num][0x97] = 0x97;
	DispConvTable[num][0x9b] = 0x9b;
	DispConvTable[num][0x9c] = 0x9c;
	DispConvTable[num][0x9d] = 0x9d;
	DispConvTable[num][0x9e] = 0x9e;
	DispConvTable[num][0x9f] = 0x9f;
}

void SetLCDProcCharsV (byte dat[])
{
	int i;

	dat[0] = 4;
	
	i = 1;
	dat[i] = (i / 9) + 1;
	dat[i+1] = 16;
	dat[i+2] = 16;
	dat[i+3] = 16;
	dat[i+4] = 16;
	dat[i+5] = 16;
	dat[i+6] = 16;
	dat[i+7] = 16;
	dat[i+8] = 16;

	i = 10;
	dat[i] = (i / 9) + 1;
	dat[i+1] = 24;
	dat[i+2] = 24;
	dat[i+3] = 24;
	dat[i+4] = 24;
	dat[i+5] = 24;
	dat[i+6] = 24;
	dat[i+7] = 24;
	dat[i+8] = 24;

	i = 19;
	dat[i] = (i / 9) + 1;
	dat[i+1] = 28;
	dat[i+2] = 28;
	dat[i+3] = 28;
	dat[i+4] = 28;
	dat[i+5] = 28;
	dat[i+6] = 28;
	dat[i+7] = 28;
	dat[i+8] = 28;

	i = 28;
	dat[i] = (i / 9) + 1;
	dat[i+1] = 30;
	dat[i+2] = 30;
	dat[i+3] = 30;
	dat[i+4] = 30;
	dat[i+5] = 30;
	dat[i+6] = 30;
	dat[i+7] = 30;
	dat[i+8] = 30;

}

void SetSpecialChars (byte dat[])
{
	int i;

	dat[0] = 6;
	
	i = 1;
	dat[i] = (i / 9) + 1; // Play
	dat[i+1] = 16;
	dat[i+2] = 24;
	dat[i+3] = 28;
	dat[i+4] = 30;
	dat[i+5] = 28;
	dat[i+6] = 24;
	dat[i+7] = 16;
	dat[i+8] = 0;

	i = 10;
	dat[i] = (i / 9) + 1; // Pause
	dat[i+1] = 27;
	dat[i+2] = 27;
	dat[i+3] = 27;
	dat[i+4] = 27;
	dat[i+5] = 27;
	dat[i+6] = 27;
	dat[i+7] = 27;
	dat[i+8] = 0;

	i = 19;
	dat[i] = (i / 9) + 1; // Stop
	dat[i+1] = 0;
	dat[i+2] = 31;
	dat[i+3] = 31;
	dat[i+4] = 31;
	dat[i+5] = 31;
	dat[i+6] = 31;
	dat[i+7] = 0;
	dat[i+8] = 0;

	i = 28; 
	dat[i] = (i / 9) + 1; // Next
	dat[i+1] = 17;
	dat[i+2] = 25;
	dat[i+3] = 29;
	dat[i+4] = 31;
	dat[i+5] = 29;
	dat[i+6] = 25;
	dat[i+7] = 17;
	dat[i+8] = 0;

	i = 37;
	dat[i] = (i / 9) + 1; // Prev
	dat[i+1] = 17;
	dat[i+2] = 19;
	dat[i+3] = 23;
	dat[i+4] = 31;
	dat[i+5] = 23;
	dat[i+6] = 19;
	dat[i+7] = 17;
	dat[i+8] = 0;

	i = 46;
	dat[i] = (i / 9) + 1; // Rew
	dat[i+1] = 1;
	dat[i+2] = 3;
	dat[i+3] = 7;
	dat[i+4] = 15;
	dat[i+5] = 7;
	dat[i+6] = 3;
	dat[i+7] = 1;
	dat[i+8] = 0;
}


int SetTransceiverModus (byte mode,word send_mask,byte addr,char *hotcode,int hotlen,byte extended_mode,byte extended_mode2,byte usb_mode)
{
	MODE_BUFFER md;
	byte res;
	md.sbus_command = HOST_SETMODE;
	md.sbus_address = 0;
	if (addr <= 15)  md.sbus_address |= addr;
	if (addr == 'L') md.sbus_address |= ADRESS_LOCAL;
	if (addr == '*') md.sbus_address |= ADRESS_ALL;

	md.hotcode_len = hotlen;
	memcpy (md.hotcode,hotcode,hotlen);

	md.hotcode[hotlen] = extended_mode;
	md.hotcode[hotlen+1] = extended_mode2;

	md.sbus_len = sizeof (MODE_BUFFER) + md.hotcode_len - 98;


	md.mode = mode;
	md.target_mask = send_mask;

	res = WriteTransceiver ((IRDATA *)&md,usb_mode);
	if (res) return (res);

	return (0);
}


int	GetTransceiverVersion (char version [],unsigned int *cap,unsigned int *serno,char mac_adr[],byte usbmode)
{
	int res,i;
	IRDATA ir;
	char data[8];
	GETVERSION_LAN ver;
	struct sockaddr_in send_adr;

#ifdef LINUX
	fd_set events;
	int maxfd,wait;
	struct timeval tv;
#endif

	ir.command = HOST_VERSION;
	ir.len = 8;

	ir.address = VERSION_MAGIC_1;
	ir.target_mask = VERSION_MAGIC_2;

	ir.checksumme = get_checksumme (&ir);

	*cap = 0;
	*serno = 0;
	version[0] = 0;
	memset (mac_adr,0,6);

	if (usbmode == 2) {

		IRTransLanSend (NULL,&ir);

#ifdef WIN32
		res = WaitForSingleObject (IrtLanEvent,2000);
		if (res == WAIT_TIMEOUT)  return (ERR_READVERSION);
		WSAResetEvent (IrtLanEvent);
#endif

#ifdef LINUX
		FD_ZERO (&events);

		FD_SET (irtlan_socket,&events);
		maxfd = irtlan_socket + 1;

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		wait = select (maxfd,&events,NULL,NULL,&tv);
		if (!wait) return (ERR_READVERSION);
#endif


		i = sizeof (send_adr);
		res = recvfrom(irtlan_socket,(byte *)&ver,sizeof (GETVERSION_LAN),MSG_NOSIGNAL,(struct sockaddr *)&send_adr,&i);
		if (res <= 0 || res != ver.len + 1 || ver.netcommand != RESULT_GETVERSION) return (ERR_READVERSION);

		version[8] = 0;
		memcpy (version,ver.ir_version,8);
//		memcpy (dev->lan_version,ver.lan_version,8);
		*cap = ver.ir_capabilities;
		*serno = ver.ir_serno;
		memcpy (mac_adr,ver.mac_adr,6);
	}

	else {
		res = WriteTransceiver (&ir,usbmode);

		version[8] = 0;
		res = ReadIRString (version,8,500,usbmode);
		if (res != 8) return (ERR_READVERSION);

		if (strcmp (version+1,"2.24.01") >= 0) {
			res = ReadIRString (data,8,500,usbmode);
			if (res != 8) return (ERR_READVERSION);

			if (strcmp (version+1,"4.04.01") >= 0) {
				memcpy (cap,data,4);
				swap_int (cap);
				memcpy (serno,data + 4,4);
				swap_int ((int32_t *)serno);
			}
			else {
				memcpy (cap,data + 1,2);
				swap_int (cap);
				memcpy (serno,data + 3,4);
				swap_int ((int32_t *)serno);
			}
			if (*serno == 2802 || *serno == 0x11223344) *serno = 0;

		}
	}
	return (0);
}


int WriteTransceiverL (IRDATA *src,byte usb_mode);

int	GetTransceiverVersionL (char version [],unsigned int *cap,unsigned int *cap2,unsigned int *cap3,unsigned int *cap4,unsigned int *serno,char mac_adr[],byte usbmode)
{
	int res,i;
	IRDATA ir;
	char data[100];
	GETVERSION_LAN_EX ver;
	struct sockaddr_in send_adr;

#ifdef LINUX
	fd_set events;
	int maxfd,wait;
	struct timeval tv;
#endif

	ir.command = HOST_VERSION;
	ir.len = 8;

	ir.address = VERSION_MAGIC_1;
	ir.target_mask = VERSION_MAGIC_2;

	ir.checksumme = get_checksumme (&ir);

	*cap = 0;
	*serno = 0;
	version[0] = 0;
	memset (mac_adr,0,6);

	if (usbmode == 2) {

		IRTransLanSend (NULL,&ir);

#ifdef WIN32
		res = WaitForSingleObject (IrtLanEvent,2000);
		if (res == WAIT_TIMEOUT)  return (ERR_READVERSION);
		WSAResetEvent (IrtLanEvent);
#endif

#ifdef LINUX
		FD_ZERO (&events);

		FD_SET (irtlan_socket,&events);
		maxfd = irtlan_socket + 1;

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		wait = select (maxfd,&events,NULL,NULL,&tv);
		if (!wait) return (ERR_READVERSION);
#endif


		i = sizeof (send_adr);
		res = recvfrom(irtlan_socket,(byte *)&ver,sizeof (GETVERSION_LAN_EX),MSG_NOSIGNAL,(struct sockaddr *)&send_adr,&i);
		if (res <= 0 || res != ver.len + 1 || ver.netcommand != RESULT_GETVERSION) return (ERR_READVERSION);

		version[8] = 0;
		memcpy (version,ver.ir_version,8);
		*cap = ver.ir_capabilities;
		swap_int ((int32_t *)cap);
		*serno = ver.ir_serno;
		swap_int ((int32_t *)serno);
		memcpy (mac_adr,ver.mac_adr,6);

		if (res >= sizeof (GETVERSION_LAN_EX)) {
			*cap2 = ver. ir_capabilities2;
			*cap3 = ver.ir_capabilities3;
			*cap4 = ver.ir_capabilities4;
			swap_int ((int32_t *)cap2);
			swap_int ((int32_t *)cap3);
			swap_int ((int32_t *)cap4);
		}
		else {
			cap2 = 0;
			cap3 = 0;
			cap4 = 0;
		}

	}

	else {
		res = i = 0;
		while (res != 8 && i < 5) {
			res = WriteTransceiverL (&ir,usbmode);

			version[8] = 0;
			res = ReadIRString (version,8,500,usbmode);
			i++;
		}
		if (res != 8) return (ERR_READVERSION);

		if (strcmp (version+1,"5.10.01") >= 0) {
			res = ReadIRString (data,20,500,usbmode);
			if (res != 20) return (ERR_READVERSION);
			memcpy (cap,data,4);
			swap_int (cap);
			memcpy (cap2,data+4,4);
			swap_int (cap2);
			memcpy (cap3,data+8,4);
			swap_int (cap3);
			memcpy (cap4,data+12,4);
			swap_int (cap4);

			memcpy (serno,data + 16,4);
			swap_int ((int32_t *)serno);
		
			if (*serno == 2802 || *serno == 0x11223344) *serno = 0;
		}

		else if (strcmp (version+1,"2.24.01") >= 0) {
			res = ReadIRString (data,8,500,usbmode);
			if (res != 8) return (ERR_READVERSION);

			if (strcmp (version+1,"4.04.01") >= 0) {
				memcpy (cap,data,4);
				swap_int (cap);
				memcpy (serno,data + 4,4);
				swap_int ((int32_t *)serno);
			}
			else {
				memcpy (cap,data + 1,2);
				swap_int (cap);
				memcpy (serno,data + 3,4);
				swap_int ((int32_t *)serno);
			}
			if (*serno == 2802 || *serno == 0x11223344) *serno = 0;

		}
	}
	return (0);
}





int WriteTransceiver (IRDATA *src,byte usb_mode)
{
	byte res = 0;
	char st[100];
	int count = 0;
	IRDATA_LARGE send;

	swap_irdata (src,(IRDATA *)&send);

	send.checksumme = get_checksumme ((IRDATA *)&send);
	do {
		if (usb_mode == 1) FlushUSB ();
		else if (!usb_mode) FlushCom ();
		if (mode_flag & HEXDUMP) Hexdump_IO ((IRDATA *)&send);
		if (hexflag) Hexdump_File ((IRDATA *)&send);
		WriteIRString ((byte *)&send,send.len,usb_mode);
		res = ReadIRString (st,1,500,usb_mode);
		count++;
		if (res != 1 || *st != 'O') {
			if (res && *st == 'R') return (ERR_RESEND);
			if (usb_mode) FlushUSB ();
			else FlushCom ();
			msSleep (200);
			if (count > 2) msSleep (1000);
			res = 0;
		}
	} while (res == 0 && count < 5);

	if (count == 5) return (ERR_TIMEOUT);

	return (0);

}

int WriteTransceiverL (IRDATA *src,byte usb_mode)
{
	byte res = 0;
	char st[100];
	static byte seq;
	int count = 0;
	IRDATA_LARGE send;

	swap_irdata (src,(IRDATA *)&send);

	((byte *)&send)[send.len] = seq++;

	send.checksumme = get_checksumme ((IRDATA *)&send);

	do {
		if (usb_mode == 1) FlushUSB ();
		else if (!usb_mode) FlushCom ();
		if (mode_flag & HEXDUMP) Hexdump_IO ((IRDATA *)&send);
		if (hexflag) Hexdump_File ((IRDATA *)&send);
		if (usb_mode) WriteIRString ((byte *)&send,send.len+1,usb_mode);
		else WriteIRString ((byte *)&send,send.len,usb_mode);
		res = ReadIRString (st,1,500,usb_mode);
		count++;
		if (res != 1 || *st != 'O') {
			if (res && *st == 'R') return (ERR_RESEND);
			if (usb_mode) FlushUSB ();
			else FlushCom ();
			msSleep (200);
			if (count > 2) msSleep (1000);
			res = 0;
		}
	} while (res == 0 && count < 5);

	if (count == 5) return (ERR_TIMEOUT);

	return (0);

}

void WriteIRString (byte pnt[],int len,byte usb_mode)
{
#ifdef WIN32
	if (usb_mode) WriteUSBString (pnt,len);
	else WriteSerialString (pnt,len);
#endif

#ifdef LINUX
	WriteSerialString (pnt,len);
#endif
}

int	ReadIRString (byte pnt[],int len,word timeout,byte usb_mode)
{
#ifdef WIN32
	if (usb_mode) return (ReadUSBString (pnt,len,timeout));
	else return (ReadSerialString (pnt,len,timeout));
#endif

#ifdef LINUX
	return (ReadSerialString (pnt,len,timeout));
#endif
}


#ifdef LINUX
void FlushUSB (void)
{
	FlushCom ();
}
#endif
/*
void FlushIoEx (DEVICEINFO *dev)
{
#ifdef WIN32
	if (dev->io.if_type == IF_USB) FlushUSBEx (dev->io.usbport);
	else FlushComEx (dev->io.comport);
#endif

#ifdef LINUX
	FlushComEx (dev->io.comport);
#endif
}
*/

void SwapWordN (word *pnt)
{
	byte *a,v;

	if (byteorder) return;

	a = (byte *)pnt;
	v = a[0];
	a[0] = a[1];
	a[1] = v;
}


void SwapIntN (int32_t *pnt)
{
	byte *a,v;

	if (byteorder) return;

	a = (byte *)pnt;
	v = a[0];
	a[0] = a[3];
	a[3] = v;

	v = a[1];
	a[1] = a[2];
	a[2] = v;
}
