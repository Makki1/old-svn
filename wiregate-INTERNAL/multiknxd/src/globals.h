/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * globals.h
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

#include "../config.h"
#include <sys/time.h> 
#include <time.h>
#include <syslog.h>

/* DEBUG and LOG-macros, DBG isn't compiled in for release to save space */
//FIXME: Add loglevels
#ifdef DEBUG
#define DBG(...) { \
        struct timeval tv; \
        struct tm* ptm; \
        gettimeofday (&tv, NULL); \
        ptm = localtime (&tv.tv_sec); \
        char _tmstr[24]; \
        strftime(_tmstr,sizeof(_tmstr),"%Y-%m-%d %H:%M:%S",ptm); \
        fprintf(stderr, "%s.%03ld (%s, %s(), %d) - ", _tmstr, tv.tv_usec / 1000, __FILE__, __FUNCTION__, __LINE__); \
        char _bf[1024] = {0}; snprintf(_bf, sizeof(_bf)-1, __VA_ARGS__); \
        fprintf(stderr, "%s\n", _bf); }
#else
#define DBG(...)
#endif

#define LOG(...) { char _bf[1024] = {0}; snprintf(_bf, sizeof(_bf)-1, __VA_ARGS__); fprintf(stderr, "%s\n", _bf); syslog(LOG_INFO, "%s", _bf); }

#define MAX_MODULES 10

/* global variables that are accessed by all modules */
typedef struct _globals globals;

/* config-files */
dictionary * ow_ini;
struct json_object *jconf;
