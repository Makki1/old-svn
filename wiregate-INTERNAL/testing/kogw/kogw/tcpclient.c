/*
    HS-KOGW4Web
    Copyright (C) 2010 Michael Markstaller <mm@elabnet.de>

    Derived from eibd:
    Copyright (C) 2005-2010 Martin Koegler <mkoegler@auto.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 * usage: THIS <host> <port>
 */

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define BUFFSIZE 32000
void Die(char *mess) { perror(mess); exit(1); }


/* This function accepts a socket FD and a ptr to a destination
 * buffer.  It will receive from the socket until the EOL byte
 * sequence in seen.  The EOL bytes are read from the socket, but
 * the destination buffer is terminated before these bytes.
 * Returns the size of the read line (without EOL bytes).
 */
int recv_line(int sockfd, unsigned char *dest_buffer) {
#define EOL "\0" // End-of-line byte sequence
#define EOL_SIZE 1
   unsigned char *ptr;
   int eol_matched = 0;

   ptr = dest_buffer;
   while(recv(sockfd, ptr, 1, 0) == 1) { // Read a single byte.
      if(*ptr == EOL[eol_matched]) { // Does this byte match terminator?
         eol_matched++;
         if(eol_matched == EOL_SIZE) { // If all bytes match terminator,
            *(ptr+1-EOL_SIZE) = '\0'; // terminate the string.
            return strlen(dest_buffer); // Return bytes received
         }
      } else {
         eol_matched = 0;
      }
      ptr++; // Increment the pointer to the next byter.
   }
   return 0; // Didn't find the end-of-line characters.
}



int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in koserver;
    char buffer[BUFFSIZE];
    unsigned int secretlen;
    int received = 0;
    char secret[255] = "\0";
    int n;

    if (argc < 3) {
      fprintf(stderr, "USAGE: <hs_ip> <port> <secret>\n");
      exit(1);
    }
    /* Create the TCP socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      Die("Failed to create socket");
    }
    /* Construct the server sockaddr_in structure */
    memset(&koserver, 0, sizeof(koserver));       /* Clear struct */
    koserver.sin_family = AF_INET;                  /* Internet/IP */
    koserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
    koserver.sin_port = htons(atoi(argv[2]));       /* server port */
    /* Establish connection */
    if (connect(sock,
                (struct sockaddr *) &koserver,
                sizeof(koserver)) < 0) {
      Die("Failed to connect with server");
    }
    /* Send the secret/init */
    //secretlen = strlen(secret);
    //if (argc = 4)
    //    secret = (char *) strdup(argv[3]);
    n = send(sock, secret, strlen(secret)+1, 0);

    /* Receive the word back from the server */
    fprintf(stdout, "Received: ");
    int bytes = 1;
    int processed = 0;
    char recvchar = 1;
    int msizeDebug = 0;
    int msgCountDebug = 0;

    int msgtype;
    int msgaddr; //FIXME: int32
    unsigned char *msg;

    while (1) {
        bytes = recv_line(sock,buffer);
        //printf("bytes: %d : %s\n",bytes,buffer);
        //printf("%d,",bytes);
        if (bytes == 0)
            printf("NULL!\n");
        msgtype=atoi(strtok(buffer,"|"));
        msgaddr=atoi(strtok(NULL,"|"));
        msg=strtok(NULL,"|");
        printf("mem:%d t:%d %d/%d/%d D:%s\n",msizeDebug,msgtype,(msgaddr >> 11) & 0xff, (msgaddr >> 8) & 0x07, (msgaddr) & 0xff,msg);
        if (msgtype == 2) { //init
            msizeDebug += sizeof(msgtype) + sizeof(msgaddr) + sizeof(msg);
        } else {
            msgCountDebug++;
        }
        if (msgCountDebug > 500)
            exit(0);
        //straddress.sprintf("%d/%d/%d", (address >> 11) & 0xff, (address >> 8) & 0x07, (address) & 0xff);
    }
    /*
    while (bytes > -1) {
        int msglen = 0;
        while (bytes = recv(sock, recvchar, 1, 0) > 0 && recvchar != NULL) {
            msglen += 1;
        }
        printf("msglen: %d\n",msglen);
      */
        //int bytes = 0;
      //if ((bytes = recv(sock, buffer, BUFFSIZE-1, 0)) < 1) {
        //Die("Failed to receive bytes from server");
      //}
      //received += bytes;
      //printf("bytes: %d",bytes);
      //fprintf(stdout, buffer);
      //buffer[bytes] = '\0';
      //while (bytes > processed) {
        //*msg = strtok(NULL,"\0");
        //fprintf(stdout, buffer);
        //processed += strlen(msg);
        //fprintf(stdout, msg);



}
