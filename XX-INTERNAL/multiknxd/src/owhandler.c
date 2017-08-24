/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * owhandler.c
 *
 * Copyright (C) 2012 - Michael Markstaller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * Some notes on this module:
 * - makes heavy use of iniparser to avoid building duplicate structs
 * - ow_ini may NOT change during runtime - if so the thread should be retstartet
 * - optimization is more on a low memory footprint and avoid to build leaks than on speed
 * 
 */

#include <search.h> /* binary tree/hash for states */
#include <ownetapi.h>
#include "owhandler.h"
#include "iniparser/iniparser.h"
#include "globals.h"

//FIXME: configurable, default to localhost
char *owserver_address = "172.17.2.203:4304";

typedef struct SensorState_t {
	char name[18]; //FIXME: do we need it?
    int present;
    float lastval;
    int timelast;
} SensorState_t;

//FIXME: savestate/loadstate?
//FIXME: mutex for state?
//FIXME: check to use owserver-cache (NOT reading uncached!) more efficient


void *handleDS1990(int state, int index) {
    //FIXME: enum bus.X/bus.X
}

/**
 * handler(thread) for the fast stuff on onewire
 * - the old ini-variant -
 */
void *ow_ini_handler_fast(void) {
    int i;
    OWNET_HANDLE owh = 0;
    int family;
    int scount = 0;
    long long sensorN;
    char *sensorId;
    /* determine required size of SensorState once */
    for (i=0; i<iniparser_getnsec(ow_ini); i++) {
        sscanf (iniparser_getsecname(ow_ini,i), "%x.%llx", &family, &sensorN);
        if (family != OW_FAM_DS1990 && family != OW_FAM_DS2406 && family != OW_FAM_DS2413)
            continue;
        scount++;
    }
    //FIXME: if scount is 0 we could exit this thread?
    SensorState_t SensorState[scount];
    DBG("SensorState size: %d/%d", sizeof(SensorState),sizeof(int));
    while (1) {
        if (!owh) {
            DBG("Trying to connect to %s", owserver_address);
            if ((owh = OWNET_init(owserver_address)) < 0) {
                LOG("OWNET_init(%s) failed", owserver_address);
                usleep(3*1000*1000);
            }
        } else {
            DBG("hello");
            scount=0;
            for (i=0; i<iniparser_getnsec(ow_ini); i++) {
                sscanf (iniparser_getsecname(ow_ini,i), "%x.%llx", &family, &sensorN);
                //DBG("Looking on %s (%d . %lld)",iniparser_getsecname(ow_ini,i),family,sensorN);
                /* Save the world, only look on iButtons&I/Os here */
                //FIXME: maybe better to check iButton direct and Fall-through, saves the switch-statement after present
                if (family != OW_FAM_DS1990 && family != OW_FAM_DS2406 && family != OW_FAM_DS2413)
                    continue;
                //FIXME: read uncached here ONCE per loop
                if (OWNET_present(owh,iniparser_getsecname(ow_ini,i))==0) {
                    switch(family) {
                        case OW_FAM_DS1990:
                            /* FIXME: check struct if present before, only handle if newly connected to avoid
                             * scanning the whole bus uncached for bus-id */
                            handleDS1990(1,i);
                            break;
                        case OW_FAM_DS2406:
                        case OW_FAM_DS2413:
                            //FIXME: query GPIOs
                            break;
                    }                        
                    DBG("hurray, %s (%lld) is present",iniparser_getsecname(ow_ini,i),sensorN);
                } else {
                    if (family != OW_FAM_DS1990)
                        continue;
                    //check disconnected iButton
                    DBG("%s (%lld) is NOT here",iniparser_getsecname(ow_ini,i),sensorN);
                }
                scount++;
            }
            //FIXME: check looptime, sleep at least 200-300ms but as owserver is slow anyway..
            usleep(1*1000*1000);
        }
    }
}

/**
 * handler(thread) for the slow stuff on onewire
 * - the old ini-variant -
 */
void *ow_ini_handler_slow(void) {
    while (1) {
        /* FIXME: Fill global struct with mapping of bus.X / device-address (usb/i2c) / names
         * and lock mutex until this is finished (at least on startup)
         */
        DBG("hello");
        //foreach sensor-family (temp/hum) we want do something
        usleep(5*1000*1000);
    }
}
