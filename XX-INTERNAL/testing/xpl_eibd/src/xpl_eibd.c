/***************************************************************************
 *   Copyright (C) 2009 by Michael Markstaller   *
 *   devel@wiregate.de   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <stdarg.h>
#include <sys/file.h>

#include <xPL.h>
#include <eibclient.h>
#include "eib_common.h"

#define APP_VERSION "0.1"
#define APP_NAME "xpl-eibd"
#define PIDFILE "/var/run/" APP_NAME ".pid"

/* max numer of daemon-restart per day */
#define MAX_APP_RESTARTS 10000

static Bool daemonMode = FALSE;
static Bool debugMode = FALSE;
static Bool superviseMode = TRUE;

/* static buffer to create log messages */
#define LOG_BUFF_MAX 512
static char logMessageBuffer[LOG_BUFF_MAX];

/* Used to track app */
static pid_t appPid = 0;

static xPL_ServicePtr myService = NULL;
static xPL_MessagePtr eibdMessage = NULL;

/** path to pid file */
FILE *pidf;
char *pidfile = PIDFILE;

/* eib defintions */
static char eibUrl[255] = "local:/tmp/eib";
static int len;
static EIBConnection *con;
static eibaddr_t dest;
static eibaddr_t src;
static uchar buf[200];
int tcount = 0;
float tps = 0;
int last_telgr;

/* Private wrapper for messages */
static void writeMessage(int msgType, String theFormat, va_list theParms) {
  time_t rightNow;

  /* Write a time stamp */
  if (!daemonMode) {
    time(&rightNow);
    strftime(logMessageBuffer, 40, "%Y-%m-%d %H:%M:%S ", localtime(&rightNow));

    switch(msgType) {
    case LOG_ERR:
      strcat(logMessageBuffer, "ERROR");
      break;
    case LOG_WARNING:
      strcat(logMessageBuffer, "WARNING");
      break;
    case LOG_INFO:
      strcat(logMessageBuffer, "INFO");
      break;
    case LOG_DEBUG:
      strcat(logMessageBuffer, "DEBUG");
      break;
    }

    strcat(logMessageBuffer, ": ");
  } else
    logMessageBuffer[0] = '\0';

  /* Convert formatted message */
  vsprintf(&logMessageBuffer[strlen(logMessageBuffer)], theFormat, theParms);

  /* Write to the console or system log file */
  if (daemonMode)
    syslog(msgType, logMessageBuffer);
  else {
    strcat(logMessageBuffer, "\n");
    fprintf(stderr, logMessageBuffer);
  }
}

/* Write an information message out */
void writeInfo(String theFormat, ...) {
  va_list theParms;
  va_start(theParms, theFormat);
  writeMessage(LOG_INFO, theFormat, theParms);
  va_end(theParms);
}

/* Write an error message out */
static void writeError(String theFormat, ...) {
  va_list theParms;
  va_start(theParms, theFormat);
  writeMessage(LOG_ERR, theFormat, theParms);
  va_end(theParms);
}

/* Write a debug message out */
static void writeDebug(String theFormat, ...) {
  va_list theParms;

  if (debugMode) {
    va_start(theParms, theFormat);
    writeMessage(LOG_DEBUG, theFormat, theParms);
    va_end(theParms);
  }
}


/* parseCmdLine will handles command line switches.  Valid switches are: */
/*  -i x - set interface to use */
/*  -debug - set debugging mode */
/*  -xpldebug - set debugging and enable xPL debugging */
/*  -daemon - disconnect from the console */
/*  -u <url> - set eibd url (ie.e local:/tmp/eib or ip:127.0.0.1 */

static Bool parseCmdLine( int *argc, char *argv[]) {
  int swptr;
  int newcnt = 0;

  /* Handle each item of the command line.  If it starts with a '-', then */
  /* process it as a switch.  If not, then copy it to a new position in   */
  /* the argv list and up the new parm counter.                           */
  for(swptr = 0; swptr < *argc; swptr++) {
    /* If it doesn't begin with a '-', it's not a switch. */
    if (argv[swptr][0] != '-') {
      if (swptr != newcnt) argv[++newcnt] = argv[swptr];
    }
    else {
       /* Check for debug mode */
      if (!strcmp(argv[swptr],"-debug")) {
	debugMode = TRUE;
	daemonMode = FALSE;
	writeDebug("Debuging mode enabled");
	continue;
      }

      /* Check for debug mode */
      if (!strcmp(argv[swptr],"-xpldebug")) {
	debugMode = TRUE;
	daemonMode = FALSE;
	xPL_setDebugging(TRUE);
	xPL_Debug("xPL Debug mode enabled");
	continue;
      }

      /* Check for daemon mode */
      if (!strcmp(argv[swptr],"-daemon")) {
	daemonMode = TRUE;
	continue;
      }
      /* Check for daemon mode */
      if (!strcmp(argv[swptr],"-nosup")) {
	superviseMode = FALSE;
	continue;
      }

      /* Check for eibd url */
      if (!strcmp(argv[swptr],"-u")) {
	strcpy (eibUrl, argv[swptr+1]);
	continue;
      }

      /* Check for pidfile */
      if (!strcmp(argv[swptr],"-p")) {
	pidfile = argv[swptr+1];
	continue;
      }

	  /* Anything left is unknown */
      writeError("Unknown switch `%s'", argv[swptr] );
      return FALSE;
    }
  }

  /* Set in place the new argument count and exit */
  *argc = newcnt + 1;
  return TRUE;
}

/* Print command usage info out */
void printUsage(String ourName) {
  fprintf(stderr, "%s - xpl-eibd gateway v%s\n", ourName,APP_VERSION);
  fprintf(stderr, "Copyright (c) 2009, Michael Markstaller\n\n");
  fprintf(stderr, "Usage: %s [-debug] [-xpldebug] [-d] [-ip x] [-i x]\n", ourName);
  fprintf(stderr, "  -debug -- enable hub debug messages\n");
  fprintf(stderr, "  -xpldebug -- enable hub and xPLLib debugging messagaes\n");
  fprintf(stderr, "  -daemon   -- detach and run as daemon\n");
  fprintf(stderr, "  -nosup    -- Do not run supervisor-daemon (default=yes)\n");
  fprintf(stderr, "  -p <filename> -- pidfile-nae (else /var/run/%s.pid\n", APP_NAME);
  fprintf(stderr, "  -interface x   -- Use interface named x (i.e. eth0) as network interface\n");
  fprintf(stderr, "  -ip x  -- Bind to specified IP address for xPL\n");
  fprintf(stderr, "  -u <url> - set eibd url (ie.e local:/tmp/eib or ip:127.0.0.1\n");
}

/* Shutdown gracefully if user hits ^C or received TERM signal */
static void appShutdownHandler(int onSignal) {
  xPL_shutdown();
  writeInfo("%s received signal %d - shutting down", APP_NAME, WTERMSIG(onSignal));
  exit(0);
}


static void checkEibData() {
    char addr_from[10], addr_to[10], hexdata[50], op[9], databuf[4];
    int i;
    if (!EIB_Poll_Complete(con))
        return;
    len = EIBGetGroup_Src (con, sizeof (buf), buf, &src, &dest);
    if (len == -1) {
        writeError("Read from eibd failed");
        exit(1);
    }
      if (len < 2) {
          writeError("Invalid packet from eibd");
          exit(1);
      }
      sprintf (addr_from, "%d.%d.%d", (src >> 12) & 0x0f, (src >> 8) & 0x0f, (src) & 0xff);
      sprintf (addr_to, "%d/%d/%d", (dest >> 11) & 0x1f, (dest >> 8) & 0x07, (dest) & 0xff);
      if (buf[0] & 0x3 || (buf[1] & 0xC0) == 0xC0)
        {
          writeDebug("Unknown APDU from %s to %s: %s",addr_from,addr_to,hexdata);
        }
      else
        {
          switch (buf[1] & 0xC0)
            {
            case 0x00:
              printf ("Read");
              break;
            case 0x40:
              printf ("Response");
              break;
            case 0x80:
              printf ("Write");
              break;
            }
          printf (" from ");
          printIndividual (src);
          printf (" to ");
          printGroup (dest);
          if (buf[1] & 0xC0)
            {
              printf (": ");
              if (len == 2) { /* 1 Byte of Data */
                    sprintf (hexdata, "%02X (2Byte)", buf[1] & 0x3F);
                    printf ("%02X", buf[1] & 0x3F);
              } else {
                    printHex (len - 2, buf + 2);
                    for (i = 0; i < len-2; i++) {
                        sprintf (databuf, "%02X ", buf[i+2]);
                        strcat(hexdata,databuf);
                    }

              }

            }
          printf ("\n");
          writeDebug("APDU from %s to %s: %s",addr_from,addr_to,hexdata);
          fflush (stdout);
        }
}

/* This is the app code itself.  */
static void runApp() {
  /* Start xPL up */
  if (!xPL_initialize(xPL_getParsedConnectionType())) {
    writeError("Unable to start xPL");
    exit(1);
  }

  /* Initialze the service */

  /* Create a configurable service and ser our applications version */
  myService = xPL_createConfigurableService("wiregate", "eibd", "eibd.xpl");
  xPL_setServiceVersion(myService, APP_VERSION);
    /** Enable the service */
  xPL_setServiceEnabled(myService, TRUE);

  xPL_Debug("xPLLib started");

  /* initialize eibd-connection */
  con = EIBSocketURL (eibUrl);
  if (!con || (EIBOpen_GroupSocket (con, 0) == -1)) {
    writeError("opening eibd connection to %s failed", eibUrl);
	exit(0);
  }
  writeDebug("%s connected to %s", APP_NAME, eibUrl);


  /* Install signal traps for proper shutdown */
  signal(SIGTERM, appShutdownHandler);
  signal(SIGINT, appShutdownHandler);

  writeInfo("%s child started successfully", APP_NAME);
  /* Start xPL App HERE */
  for (;;) {
      xPL_processMessages(100);
	  sleep(1);
	  checkEibData();
	  writeInfo("hallo");
  }

  /* end */
  exit(0);
}

/* Shutdown gracefully if user hits ^C or received TERM signal */
static void supervisorShutdownHandler(int onSignal) {
  int childStatus;

  writeInfo("Received termination signal -- starting shutdown");

  /* Stop the child and wait for it */
  if ((appPid != 0) && !kill(appPid, SIGTERM)) {
    waitpid(appPid, &childStatus, 0);
  }

  /* Close pidfile */
  if (pidf) {
    fclose(pidf);
    (void) unlink(pidfile);
  }

  /* Exit */
  exit(0);
}


/* Start the hub and monitor it.  If it stops for any reason, restart it */
static void superviseApp() {
  int appRestartCount = 0;
  int childStatus;

  /* Begin app supervision */

  /* Install signal traps for proper shutdown */
  signal(SIGTERM, supervisorShutdownHandler);
  signal(SIGINT, supervisorShutdownHandler);

  /* To supervise the app, we repeatedly fork ourselves each time we */
  /* determine the app is not running.  We maintain a circuit breaker*/
  /* to prevent an endless spawning if the app just can't run        */
  for(;;) {
    /* See if we can still do this */
    if (appRestartCount > MAX_APP_RESTARTS) {
      writeError("%s has died %d times -- something may be wrong -- terminating supervisor", APP_NAME, MAX_APP_RESTARTS);
      exit(1);
    }

    /* Up the restart count */
    appRestartCount++;

    /* Fork off the hub */
    switch (appPid = fork()) {
    case 0:            /* child */
      /* Close standard I/O and become our own process group */
      close(fileno(stdin));
      close(fileno(stdout));
      close(fileno(stderr));
      setpgrp();

      /* Run the app */
      runApp();

      /* If we come back, something bad likely happened */
      exit(1);

      break;
    default:           /* parent */
      writeDebug("Spawned %s process, PID=%d, Spawn count=%d", APP_NAME, appPid, appRestartCount);
      break;
    case -1:           /* error */
      writeError("Unable to spawn %s supervisor, %s (%d)", APP_NAME, strerror(errno), errno);
      exit(1);
    }


    /* Now we just wait for something bad to happen to our hub */
    waitpid(appPid, &childStatus, 0);
    if (WIFEXITED(childStatus)) {
      writeInfo("%s exited normally with status %d -- restarting...", APP_NAME, WEXITSTATUS(childStatus));
      continue;
    }
    if (WIFSIGNALED(childStatus)) {
      writeInfo("%s died from by receiving unexpected signal %d -- restarting...", APP_NAME, WTERMSIG(childStatus));
      continue;
    }
    writeInfo("%s died from unknown causes -- restarting...", APP_NAME);
    continue;
    /* sleep a while to avoid too quick restarts */
    sleep(1);
  }
}


int main(int argc, String argv[]) {
    int fd;
  /* Check for xPL command parameters */
  xPL_parseCommonArgs(&argc, argv, TRUE);

  /* Parse Hub command arguments */
  if (!parseCmdLine(&argc, argv)) {
    printUsage(argv[0]);
    exit(1);
  }

  /* Now we detach (daemonize ourself) */
  if (daemonMode) {
    /* Fork ourselves */
    switch (fork()) {
    case 0:            /* child */
      /* Close standard I/O and become our own process group */
      close(fileno(stdin));
      close(fileno(stdout));
      close(fileno(stderr));
      setpgrp();

      if (strlen(pidfile) > 0 && superviseMode)
      {
        /* create pid lockfile in /var/run */
        if((fd=open(pidfile,O_RDWR|O_CREAT,0644))==-1 ||
           (pidf=fdopen(fd,"r+"))==NULL)
        {
            writeError("%s: can't open or create %s\n",
                APP_NAME,pidfile);
            exit(-1);
        }
        if(flock(fd,LOCK_EX|LOCK_NB)==-1)
        {
            pid_t otherpid;

            if(fscanf(pidf,"%d\n",&otherpid)>0)
            {
                writeError("%s: there seems to already be "
                    "a process with pid %d\n",
                    APP_NAME,otherpid);
                writeError("%s: otherwise delete stale "
                    "lockfile %s\n",APP_NAME,pidfile);
            }
            else
            {
                writeError("%s: invalid pidfile %s encountered\n",
                    APP_NAME,pidfile);
            }
            exit(1);
        }
        (void) fcntl(fd,F_SETFD,FD_CLOEXEC);
        rewind(pidf);
        (void) fprintf(pidf,"%d\n",getpid());
        (void) ftruncate(fileno(pidf),ftell(pidf));
      }
      /* Start he app and keep it running */
	  superviseApp();

      break;
    default:           /* parent */
      exit(0);
      break;
    case -1:           /* error */
      writeError("Unable to spawn %s supervisor, %s (%d)", APP_NAME, strerror(errno), errno);
      exit(1);
    }
  } else {
    /* When running non-detached, just do the app work */
    runApp();
  }


  return 0;
}
