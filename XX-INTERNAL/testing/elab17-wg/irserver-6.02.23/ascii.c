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
#include <sys/stat.h> 
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
#include <time.h>

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

extern int protocol_version;
int AnalyzeUDPString (char *st,int *netcom,char *remote,char *command,char *ccf,int *netmask,int *bus,int *led,int *port);

void	CloseIRSocket (int client);
int		FlashHTML (byte *command,byte result[]);
int		GetHTMLFileList (byte *command,byte result[]);
int		IRTransLanFlash (DEVICEINFO *dev,IRDATA_LAN_FLASH *ird,int len,uint32_t ip);
int		SendASCII (byte *command,byte result[]);
int		GetRemotelist (byte *data,byte result[]);
int		GetCommandlist (byte *data,byte result[]);

void	SwapWordN (word *pnt);
void	SwapIntN (int32_t *pnt);

void DoExecuteASCIICommand (byte command[],SOCKET sockfd,int client)
{
	int len,errcnt,sz,res;
	char result[65536];

	memset (result,0,sizeof (result));
	if (!strncmp (command,"GET_HTML_FILELIST",17)) {
		len = GetHTMLFileList (command+17,result);
	}
	else if (!strncmp (command,"FLASH_HTML",10)) {
		len = FlashHTML (command+10,result);
	}
	else if (!strncmp (command,"snd",3)) {
		len = SendASCII (command,result);
	}
	else if (!strncmp (command,"getremotes",10)) {
		len = GetRemotelist (command,result);
	}
	else if (!strncmp (command,"getcommands",11)) {
		len = GetCommandlist (command,result);
	}

	else {
		sprintf (result,"Illegal ASCII Command: %s\n",command);
		log_print (result,LOG_ERROR);
		return;
	}

	sprintf (result+2,"%05d",len);
	result[7] = ' ';
	log_print (result,LOG_INFO);

	errcnt = sz = 0;
	while (sz < len && errcnt < 20) {
		res = send (sockfd,result + sz,len - sz,MSG_NOSIGNAL);
		if (res > 0) sz += res;
		if (res == -1) {
			msSleep (100);
			errcnt++;
		}
	}

	if (res <= 0 && len) {
		CloseIRSocket (client);
		sprintf (result,"IP Connection lost\n");
		log_print (result,LOG_ERROR);
	}
} 

int GetCommandlist (byte *data,byte result[])
{
	int i,j,res;
	COMMANDBUFFER cb;

	i = 12;

	while (data[i] && data[i] != ',') i++;
	if (data[i]) data[i++] = 0;
	
	res = GetCommandDatabase (&cb,data+12,atoi (data+i));
	if (res) {
		sprintf (result,"**00000 RESULT ERROR: Remote %s not found\n",data+12);
		log_print (result+15, LOG_ERROR);
		return ((int)strlen (result));
	}

	sprintf (result,"**00000 COMMANDLIST %d,%d,%d",cb.offset,cb.count_total,cb.count_buffer);

	for (i=0;i < cb.count_buffer;i++) {
		strcat (result,",");
		j = 19;
		while (j && cb.commands[i][j] == ' ') j--;
		cb.commands[i][j+1] = 0;
		strcat (result,cb.commands[i]);
	}
	strcat (result,"\n");

	return ((int)strlen (result));

}

int GetRemotelist (byte *data,byte result[])
{
	int i,j;
	REMOTEBUFFER rb;
	GetRemoteDatabase (&rb,atoi (data + 11));

	sprintf (result,"**00000 REMOTELIST %d,%d,%d",rb.offset,rb.count_total,rb.count_buffer);

	for (i=0;i < rb.count_buffer;i++) {
		strcat (result,",");
		j = 79;
		while (j && rb.remotes[i].name[j] == ' ') j--;
		rb.remotes[i].name[j+1] = 0;
		strcat (result,rb.remotes[i].name);
	}
	strcat (result,"\n");

	return ((int)strlen (result));
}


int SendASCII (byte *data,byte result[])
{
	int res;
	int cmd_num,adr;
	char err[256],txt[256];
	char remote[80],command[20],ccf[2048];
	int netcom,netmask,bus,led,port;

	res = AnalyzeUDPString (data,&netcom,remote,command,ccf,&netmask,&bus,&led,&port);
	if (res) {
		log_print ("Illegal IRTrans ASCII Command\n", LOG_ERROR);
		strcpy (result,"**00000 RESULT FORMAT ERROR\n");
		return ((int)strlen (result));
	}

	sprintf (txt,"IRTrans ASCII Command: %d %s,%s,%d,%d\n", netcom,remote,command,bus,led);
	log_print (txt,LOG_DEBUG);
	
	adr = 0;

	if (netmask) adr |= 0x10000 | netmask;
	adr |= (led & 3) << 17;
	if (bus == 255) adr |= 0x40000000;
	else adr |= bus << 19;
	protocol_version = 210;

	if (netcom == 1) {
		res = DBFindRemoteCommand (remote,command,&cmd_num,NULL);
		if (res) {
			GetError (res, txt);
			switch(res) {
				case ERR_REMOTENOTFOUND:
					sprintf (err, txt, remote);
					break;
				case ERR_COMMANDNOTFOUND:
					sprintf (err, txt, command);
					break;
				default:
					sprintf (err, txt);
					break;
			}
			sprintf (result,"**00000 RESULT ERROR: %s",err);
			log_print (err, LOG_ERROR);
			return ((int)strlen (result));
		}
		SendIR (cmd_num,adr,COMMAND_SEND);
		strcpy (result,"**00000 RESULT OK\n");
	}
	return ((int)strlen (result));
}

// Bus übergeben !!!

int FlashHTML (byte *command,byte result[])
{
	int i,p,pos;
	struct stat fst;
	word flashpage,adr;
	word cnt = 0;
	unsigned int mem[65536];
	char fname[256];
	HTTP_DIRECTORY *dir;
	IRDATA_LAN_FLASH ird;
	FILE *fp;
	
	while (*command == ' ') command++;

	p = 0;
	cnt = 0;

	memset (mem,0,sizeof (mem));
	dir = (HTTP_DIRECTORY *)mem;
	memset (&ird,0,sizeof (IRDATA_LAN_FLASH));

	while (command[p]) {
		i = p;
	
		while (command[i] != ';') i++;
		command[i] = 0;

#ifdef WIN32
		sprintf (fname,"..\\html\\%s",command+p);
		fp = fopen (fname,"rb");
#else
		sprintf (fname,"../html/%s",command+p);
		fp = fopen (fname,"r");
#endif
		if (fp) {
			strncpy (dir->dir[cnt].name,command+p,22);
			dir->dir[cnt].name[22] = 0;

#ifdef WIN32
			fstat (_fileno(fp),&fst);
#else
			fstat (fileno(fp),&fst);
#endif

			dir->dir[cnt].timestamp = (unsigned int)(fst.st_mtime + ((unsigned int)70 * 365 * 24 * 3600) + ((unsigned int)17 * 24 * 3600)); // Umrechnung auf NTP Format
			SwapIntN (&dir->dir[cnt].timestamp);

			if (fst.st_size > 0xffff) dir->dir[cnt].len = 0xffff;
			else dir->dir[cnt].len = (word)fst.st_size;

			if (!strcmp (dir->dir[cnt].name + strlen (dir->dir[cnt].name) - 4,".txt")) dir->dir[cnt].filetype = CONTENT_PLAIN | EXTERNAL_FILE;
			else if (!strcmp (dir->dir[cnt].name + strlen (dir->dir[cnt].name) - 4,".htm")) dir->dir[cnt].filetype = CONTENT_HTML | EXTERNAL_FILE;
			else if (!strcmp (dir->dir[cnt].name + strlen (dir->dir[cnt].name) - 5,".html")) dir->dir[cnt].filetype = CONTENT_HTML | EXTERNAL_FILE;
			else if (!strcmp (dir->dir[cnt].name + strlen (dir->dir[cnt].name) - 4,".jpg")) dir->dir[cnt].filetype = CONTENT_JPEG | EXTERNAL_FILE;
			else if (!strcmp (dir->dir[cnt].name + strlen (dir->dir[cnt].name) - 5,".jpeg")) dir->dir[cnt].filetype = CONTENT_JPEG | EXTERNAL_FILE;
			else if (!strcmp (dir->dir[cnt].name + strlen (dir->dir[cnt].name) - 4,".gif")) dir->dir[cnt].filetype = CONTENT_GIF | EXTERNAL_FILE;
			else dir->dir[cnt].filetype = EXTERNAL_FILE;
			cnt++;
			fclose (fp);
		}
		
		p = i + 1;
	}

	dir->count = (word)cnt;
	dir->magic = F_MAGIC;
	pos = (cnt * sizeof (HTTP_DIRENTRY)) / 4 + 1;

	for (cnt=0;cnt < dir->count && pos < 32768;cnt++) {
#ifdef WIN32
		sprintf (fname,"..\\html\\%s",dir->dir[cnt].name);
		fp = fopen (fname,"rb");
#else
		sprintf (fname,"../html/%s",dir->dir[cnt].name);
		fp = fopen (fname,"r");
#endif
		if (!fp) continue;
		
		fread (&mem[pos],1,dir->dir[cnt].len,fp);
		fclose (fp);

		dir->dir[cnt].adr = pos;
		pos += (dir->dir[cnt].len + 3) / 4;

		SwapWordN (&dir->dir[cnt].adr);
		SwapWordN (&dir->dir[cnt].len);
	}
	SwapWordN (&dir->count);
	SwapWordN (&dir->magic);

	sprintf (fname,"HTML Size: %d\n",pos * 4);
	log_print (fname,LOG_INFO);

	if (pos >= 32768) {
		sprintf (fname,"HTML Size %d (Max. is 128K)\n",pos * 4);
		log_print (fname,LOG_ERROR);
		sprintf (result,"**00000 RESULT_HTML_FLASH E%s\n",fname);
		return ((int)strlen (result));
	}
	
	flashpage = 128;

	p = adr = 0;
	while (adr < pos && p != 'E') {
		i = 0;
		do {
			ird.adr = adr;
			ird.len = flashpage;
		
			SwapWordN (&ird.adr);
			SwapWordN (&ird.len);

			memcpy (ird.data,mem + adr,flashpage);
			ird.netcommand = COMMAND_FLASH_HTML;
#ifdef WIN32
			p = IRTransLanFlash (IRDevices,&ird,flashpage,IRDevices[0].io.IPAddr[0].sin_addr.S_un.S_addr);
#else
			p = IRTransLanFlash (IRDevices,&ird,flashpage,IRDevices[0].io.IPAddr[0].sin_addr.s_addr);
#endif
			if (!p) p = 'E';
			i++;
		} while (p == 'E' && i < 5);
		adr += flashpage / 4;
	}
	
	if (p == 'E') {
		strcpy (result,"**00000 RESULT_HTML_FLASH E\n");
	}
	else {
		sprintf (result,"**00000 RESULT_HTML_FLASH O%d\n",pos * 4);
	}
	return ((int)strlen (result));
}



int GetHTMLFileList (byte *command,byte result[])
{
	word cnt = 0,len;
	char fname[256];
	FILE *fp;
	struct stat fst;
    char st[256];
	struct tm *atime;

#ifdef WIN32
    struct _finddata_t c_file;
#ifdef _M_X64
    intptr_t hFile;
#else
    int hFile;
#endif
#endif

#ifdef LINUX
    int fd,pos,lend;
    long off;
	struct dirent *di;
    char mem[2048];
#endif

	while (*command == ' ') command++;

	memcpy (result,"**00000 RESULT_HTML_FILELIST ",29);
	len = 29;
	
#ifdef WIN32
	if((hFile = _findfirst( "..\\html\\*.*", &c_file )) != -1L) {
		do if (c_file.attrib != _A_SUBDIR) {
			sprintf (fname,"..\\html\\%s",c_file.name);
			fp = fopen (fname,"r");
			if (fp) {
				fstat (_fileno(fp),&fst);
				strncpy (result+len,c_file.name,22);
				strcat (result,";");

				atime = localtime (&fst.st_mtime);
				sprintf (st,"%d;%02d.%02d.%04d %02d:%02d;",fst.st_size,atime->tm_mday,atime->tm_mon+1,atime->tm_year + 1900,atime->tm_hour,atime->tm_min);
				strcat (result,st);

				len = (word)strlen (result);
				cnt++;
				fclose (fp);
			}
		} while( _findnext( hFile, &c_file ) == 0);
		_findclose( hFile );
	}
#endif

#ifdef LINUX
        fd = open ("../html",0);
        do {
			lend = getdirentries (fd,mem,2048,&off);
			pos = 0;
			while (pos < lend) {
				di = (struct dirent *)&mem[pos];

				sprintf (fname,"../html/%s",di->d_name);
				fp = fopen (fname,"r");
				if (fp && !fstat (fileno(fp),&fst) && S_ISREG (fst.st_mode)) {
					strncpy (result+len,di->d_name,22);
					strcat (result,";");

					atime = localtime (&fst.st_mtime);
					sprintf (st,"%d;%02d.%02d.%04d %02d:%02d;",fst.st_size,atime->tm_mday,atime->tm_mon+1,atime->tm_year + 1900,atime->tm_hour,atime->tm_min);
					strcat (result,st);
					
					len = strlen (result);
					cnt++;
					fclose (fp);
				}
			
				pos += di -> d_reclen;
			}
        } while (lend);

        close (fd);
#endif

	strcat (result,"\n");
	len++;
	return (len);
}

