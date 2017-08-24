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
#include <time.h>
#include <sys/timeb.h>
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
#include <net/if.h>
#include <sys/ioctl.h>
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

#ifdef WIN32
#include "winio.h"
#include "winusbio.h"
BOOL WINAPI ShutdownHandler (DWORD type);
#endif

#ifdef WIN32
WSAEVENT IrtLanEvent;
#endif


SOCKET irtlan_outbound;
SOCKET irtlan_socket;


int rcv_status_timeout (int timeout,uint32_t ip);


int	IRTransLanFlash (DEVICEINFO *dev,IRDATA_LAN_FLASH *ird,int len,uint32_t ip)
{
	int res;
	byte bcast = 0;
	struct sockaddr_in target;

	
	if (bcast) {
		memset (&target,0,sizeof (struct sockaddr));
		target.sin_family = AF_INET;
		target.sin_addr.s_addr = INADDR_BROADCAST;
		target.sin_port = htons ((word)IRTRANS_PORT);
	}

	else if (dev) memcpy (&target,&dev->io.IPAddr[0],sizeof (struct sockaddr_in));


	if ((bcast || dev) && connect (irtlan_outbound,(struct sockaddr *)&target,sizeof (struct sockaddr_in)) < 0) return (ERR_BINDSOCKET);


	res = send (irtlan_outbound,(char *)ird,len + 5,0);

	msSleep (30);
	
	return (rcv_status_timeout (500,ip));
}



int	IRTransLanSend (DEVICEINFO *dev,IRDATA *ird)
{
	int res;
	char st[255];
	IRDATA_LAN_LARGE irdlan;
	struct sockaddr_in target;

	
	memset (&irdlan,0,sizeof (IRDATA_LAN_LARGE));

	if (ird->command == START_FLASH_MODE) {
		if (ird->len == 0) {
			irdlan.netcommand = COMMAND_FLASH_END;
			ird->len = 3;
		}
		else irdlan.netcommand = COMMAND_FLASH_START;
	}

	else if (ird->command == TRANSFER_FLASH) {
		irdlan.netcommand = COMMAND_FLASH_DATA;
	}

	else {
		irdlan.netcommand = COMMAND_LAN;
	}
	if (dev) {
//		irtlan_outbound = irtlan_socket;				// Änderung für WiFi
		memcpy (&target,&dev->io.IPAddr[0],sizeof (struct sockaddr_in));
		if (dev && connect (irtlan_outbound,(struct sockaddr *)&target,sizeof (struct sockaddr_in)) < 0) return (ERR_BINDSOCKET);

	}
	memcpy (&(irdlan.ir_data),ird,ird->len);


	res = send (irtlan_outbound,(char *)&irdlan,ird->len + sizeof (IRDATA_LAN) - sizeof (IRDATA),0);

	if (dev && dev->fw_capabilities2 & FN2_SEND_ACK && (ird->command == HOST_SEND || 
		((dev->lan_version[0] != 'L' || memcmp (dev->lan_version+1,"1.07.24",7) > 0) &&  (ird->command == HOST_RESEND || ird->command == HOST_SEND_LEDMASK || ird->command == HOST_RESEND_LEDMASK)))) {
#ifdef WIN32
		res = rcv_status_timeout (2000,target.sin_addr.S_un.S_addr);
#else
		res = rcv_status_timeout (2000,target.sin_addr.s_addr);
#endif
		if (res != COMMAND_SEND_ACK) {
			sprintf (st,"IRTRans LAN IRSend ACK: %d\n",res);
			log_print (st,LOG_ERROR);
		}

	}

	else msSleep (30);

	return (0);
}


int rcv_status_timeout (int timeout,uint32_t ip)
{
	byte data[1000];
	int stat,sz,val;
	int res = -1;
	struct sockaddr_in from;

#ifdef LINUX
	fd_set events;
	int maxfd,wait,flags;
	struct timeval tv;

	flags = fcntl (irtlan_socket,F_GETFL);
#endif

retry:
#ifdef WIN32
	stat = WaitForSingleObject (IrtLanEvent,timeout);
	if (stat == WAIT_TIMEOUT)  return (-1);
#endif

#ifdef LINUX
	FD_ZERO (&events);

	FD_SET (irtlan_socket,&events);
	maxfd = irtlan_socket + 1;

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	stat = select (maxfd,&events,NULL,NULL,&tv);
	if (stat == 0)  return (-1);

	fcntl (irtlan_socket,F_SETFL,flags | O_NONBLOCK);
#endif

	do { 
		sz = sizeof (from);
		val = recvfrom (irtlan_socket,data,1000,0,(struct sockaddr *)&from,&sz);
#ifdef WIN32
		if (val > 0 && (!ip || from.sin_addr.S_un.S_addr == ip)) res = data[0];
#else
		if (val > 0 && (!ip || from.sin_addr.s_addr == ip)) res = data[0];
#endif
	} while (val > 0);

#ifdef WIN32
	WSAResetEvent (IrtLanEvent);
#endif
	if (res == -1) goto retry;

#ifdef LINUX
	fcntl (irtlan_socket,F_SETFL,flags);
#endif

	return (res);
}


int rcv_status ()
{
	byte val,res = 0;

	val = recv (irtlan_socket,&res,1,0);

	return res;
}

void InitWinsock ()
{
#ifdef WIN32
	int err;
    WORD	wVersionRequired;
    WSADATA	wsaData;
    wVersionRequired = MAKEWORD(2,2);
    err = WSAStartup(wVersionRequired, &wsaData);
    if (err != 0) exit(1);

#endif
}

extern int irtrans_udp_port;

void CloseIRTransLANSocket (void)
{
	if (irtlan_outbound > 0) closesocket (irtlan_outbound);
	if (irtlan_socket > 0) closesocket (irtlan_socket);

}

int OpenIRTransLANSocket (void)
{
	int res;
	struct sockaddr_in serv_addr;

	irtlan_outbound = socket (PF_INET,SOCK_DGRAM,0);
	if (irtlan_outbound < 0) return (ERR_OPENSOCKET);

	res = 1;
	setsockopt (irtlan_outbound,SOL_SOCKET,SO_BROADCAST,(char *)&res,sizeof (int));

	memset (&serv_addr,0,sizeof (serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = 0;
	serv_addr.sin_port = htons (0);
	if (bind (irtlan_outbound,(struct sockaddr *)&serv_addr,sizeof (serv_addr)) < 0) {
		fprintf (stderr,"\n\nError binding send socket ... Abort [%d].\n",errno);
		exit (-1);
	}


	irtlan_socket = socket (PF_INET,SOCK_DGRAM,0);
	if (irtlan_socket < 0) return (ERR_OPENSOCKET);

	memset (&serv_addr,0,sizeof (serv_addr));
	serv_addr.sin_family = AF_INET;

	serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
	serv_addr.sin_port = htons ((short)irtrans_udp_port);

	if (bind (irtlan_socket,(struct sockaddr *)&serv_addr,sizeof (serv_addr)) < 0) {
		fprintf (stderr,"\n\nError binding socket ... Abort [%d].\n",errno);
		exit (-1);
	}
	return (0);
}

#ifdef LINUX

SOCKET irt_bcast[32];
int if_count;

int OpenIRTransBroadcastSockets (void)
{
	int res,i;
	unsigned int ips[32];

	struct sockaddr_in serv_addr;

	if_count = GetInterfaces (ips);

	for (i=0;i < if_count;i++) {

		irt_bcast[i] = socket (PF_INET,SOCK_DGRAM,0);
		if (irt_bcast[i] < 0) return (ERR_OPENSOCKET);

		res = 1;
		setsockopt (irt_bcast[i],SOL_SOCKET,SO_BROADCAST,(char *)&res,sizeof (int));

		memset (&serv_addr,0,sizeof (serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = ips[i];
		serv_addr.sin_port = htons (0);
		if (bind (irt_bcast[i],(struct sockaddr *)&serv_addr,sizeof (serv_addr)) < 0) {
			fprintf (stderr,"\n\nError binding send socket ... Abort [%d].\n",errno);
			exit (-1);
		}
		memset (&serv_addr,0,sizeof (serv_addr));
		serv_addr.sin_family = AF_INET;

		serv_addr.sin_addr.s_addr = htonl (INADDR_BROADCAST);
		serv_addr.sin_port = htons ((short)irtrans_udp_port);

		if (connect (irt_bcast[i],(struct sockaddr *)&serv_addr,sizeof (struct sockaddr_in)) < 0) return (ERR_BINDSOCKET);
	}

	return (0);
}



int GetInterfaces (unsigned int ips[])
{
	int i,j,cnt;
	FILE *fp;
	char *pnt,ln[256];
	struct sockaddr_in *sinp;
	struct ifreq ifr;
	int s; /* Socket */
	char local_ip_addr[16];

	fp = fopen ("/proc/net/dev","r");
	if (!fp) return (0);
	s = socket(AF_INET, SOCK_DGRAM, 0);

	cnt = 0;
	pnt = fgets (ln,sizeof (ln),fp);
	while (pnt) {
		i = 0;
		while (ln[i] == ' ') i++;
		if (!memcmp (ln+i,"eth",3) || !memcmp (ln+i,"wlan",4)) {
			j = i;
			while ((ln[j] >= '0' && ln[j] <= '9') || (ln[j] >= 'a' && ln[j] <= 'z') || (ln[j] >= 'A' && ln[j] <= 'Z')) j++;
			ln[j] = 0;
			memset (&ifr,0,sizeof (ifr));
			strcpy(ifr.ifr_name, ln+i);
			ioctl(s, SIOCGIFADDR, &ifr);
			sinp = (struct sockaddr_in*)&ifr.ifr_addr;
			ips[cnt++] = sinp->sin_addr.s_addr;
		}
		pnt = fgets (ln,sizeof (ln),fp);
	}

	close (s);
	fclose (fp);

	return (cnt);
}

#endif
