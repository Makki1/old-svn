/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * knxhandler.c
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

#include "knxhandler.h"
#include "iniparser/iniparser.h"
#include "globals.h"

/**
 * handler(thread) for knx/eibd
 * - the old ini-variant -
 */

void *knx_handler(void) {
    while (1) {
        //FIXME: listen on groupsocket, do something useful
        DBG("hello");
        usleep(30*1000*1000);
    }
}