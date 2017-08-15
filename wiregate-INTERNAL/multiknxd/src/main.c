/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Michael Markstaller 2012 <devel@wiregate.de>
 * 
 * multiknxd is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * multiknxd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * The idea is to have a simple, flexible construction which carries out various
 * different specific modules (tasks) - in it's own threads
 * modules should be included or left out at compile-time
 * List of modules:
 * rrd??
 * knxlistener (other modules may subscribe?)
 * owreader (owfs/1-Wire)
 * libmodbus
 * dmx/ola
 * lua-scripts
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <getopt.h>
#include <argp.h>
#include <pthread.h>
#include <fcntl.h>
#include <json/json.h> /* libjson0 / json-c */
#include "iniparser/iniparser.h"
#include "owhandler.h"
#include "knxhandler.h"
#include "globals.h"
#include "daemon.h"
#include "utils.h"



const char *argp_program_version = PACKAGE_NAME " " PACKAGE_VERSION;
static char doc[] = PACKAGE_NAME " " PACKAGE_VERSION " -- a flexible daemon";

char *eibd_url = "local:/tmp/eib";
int pidFilehandle;
char *pidfilename = "/var/run/" PACKAGE_NAME ".pid";

int module_threads[MAX_MODULES];
pthread_t threads[MAX_MODULES];
int num_threads = 0;

/* This structure is used by main to communicate with parse_opt. */
struct arguments
{
    int verbose;              /* The -v flag */
    int daemon;              /* The -d flag */
    char *pidfile;           /* Argument for -o */
    char *eibd_url;           /* Argument for -u */
};
/*
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC}.
*/
static struct argp_option options[] = 
{
    {"verbose",     'v', 0, 0, "Produce verbose output"},
    {"daemon",      'd', 0, 0, "Daemonize"},
    {"pidfile",     'p', "PIDFILE", 0, "PID-File"},
    {"eibd-url",    'u', "EIBD_URL", 0,
   "URL to access eibd (local:/tmp/eib or ip:1.2.3.4)"},
  {0}
};
/*
   PARSER. Field 2 in ARGP.
   Order of parameters: KEY, ARG, STATE.
*/
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = state->input;
    switch (key)
    {
    case 'v':
      arguments->verbose = 1;
      break;
    case 'd':
      arguments->daemon = 1;
      break;
    case 'p':
      arguments->pidfile = arg;
      pidfilename = arg;
      break;
    case 'u':
      arguments->eibd_url = arg;
      eibd_url = arg;
      break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= 1)
      {
        argp_usage(state);
      }
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}
static struct argp argp = {options, parse_opt, 0, doc};

void daemonShutdown() {
	//FIXME: clean exit pthread_exit(NULL); pthread_cancel(..);
    LOG( PACKAGE_NAME " " PACKAGE_VERSION " exiting.. Built:" __DATE__ " " __TIME__);
    iniparser_freedict(ow_ini);
	close(pidFilehandle);
	unlink(pidfilename);
	exit(EXIT_SUCCESS);
}

/**
 * handle signals
 * 
 */

void signal_handler(int sig) {
    switch(sig) {
        case SIGHUP:
            LOG("Received SIGHUP signal. dunno.");
            break;
        case SIGTERM:
            LOG("Received SIGTERM signal.");
			daemonShutdown();
			break;
        case SIGINT:
            LOG("Received SIGINT signal.");
			daemonShutdown();
			break;
        default:
            LOG("Unhandled signal (%d) %s", sig, strsignal(sig));
            break;
    }
}

/**
 * starts a thread with named module
 * 
 */
void startModule(void (*moduleName)(void)) {
    //FIXME: detach threads?
    //FIXME: compiler says »void * (*)(void *)« erwartet, aber Argument hat Typ »void (*)(void)«, void is void, dunno
    pthread_create(&threads[num_threads], NULL, moduleName, NULL); /* id, thread attributes, subroutine, arguments */
    num_threads++;
}


int  parse_ini_file(char * ini_name);

int main(int argc, char **argv)
{
    struct arguments arguments;
    /* Set argument defaults */
    arguments.pidfile = NULL;
    arguments.eibd_url = "local:/tmp/eib";
    arguments.daemon = 0;
    arguments.verbose = 0;
    argp_parse (&argp, argc, argv, 0, 0, &arguments);
    int i;

    //FIXME: parse ini-file(s) - change to JSON
    ow_ini = iniparser_load("owsensors.conf");
    if (ow_ini==NULL) {
        LOG("Failed to load ow_ini");
        exit(1);
    }
    //DEBUG: dump ini
    //iniparser_dump(ow_ini, stdout);
    LOG("INI: Sections %d", iniparser_getnsec(ow_ini));
    LOG("INI: Global cycle: %g", iniparser_getdouble(ow_ini, ":cycle", -1.0));
    LOG("INI: sens cycle: %g", iniparser_getdouble(ow_ini, "26.15cebc000000:cycle", -1.0));

    /* Load config */
    //FIXME: define conf-dir at least at compile-time
    char* file = file_read("modules.conf");
    if (file) {
        jconf = json_tokener_parse(file);
        //json_parse(jobj);
        free(file);   /* release allocated memory */
    } else {
        //FIXME: use common error-handler/macro to DIE
        LOG("failed to load modules.conf!");
        exit(1);
    }
    //DEBUG: dump json-config
    
	//FIXME: clean shutdown in sub-thread with signals?
	signal(SIGHUP, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    /* ignore SIGPIPE (sent by OS if transmitting to closed TCP sockets) */
    signal(SIGPIPE, SIG_IGN);

    LOG( PACKAGE_NAME " " PACKAGE_VERSION " starting.. Built:" __DATE__ " " __TIME__);
    if (arguments.verbose)
        DBG("Debugging enabled & active");
    if (arguments.daemon)
        daemonize ();
    startModule(ow_ini_handler_fast);
    startModule(ow_ini_handler_slow);
    startModule(knx_handler);    
    //FIXME: enum & start modules from modules.conf

    while (1) {
        DBG("looping");
        usleep(10*1000*1000);
    }
    return (0);
}
