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
*/

#define HSKO_UPDATE           0x0001
#define HSKO_INIT             0x0002

#include <time.h>
#include "types.h"

typedef struct
{
  /** Layer 4 data */
   CArray data;
  /** destination address */
  grpaddr_t dst;
  /** receive time */
  time_t recvtime;
} GroupCacheEntry;


class GroupCache
{
  /** debug output */
  //Trace *t;
  /** output queue */
  Array < GroupCacheEntry * >cache;
  bool enable;
  //pth_mutex_t mutex;
  //pth_cond_t cond;
  uint16_t pos;
  grpaddr_t updates[0x100];

  GroupCacheEntry *find (grpaddr_t dst);
  void add (GroupCacheEntry * entry);

public:
    GroupCache (int t);
    virtual ~ GroupCache ();

  //void Get_L_Data (L_Data_PDU * l);

  //bool Start ();
  //void Clear ();
  //void Stop ();

  GroupCacheEntry Read (grpaddr_t addr, unsigned timeout, uint16_t age);
    Array < grpaddr_t > LastUpdates (uint16_t start, uint8_t timeout,
	//			     uint16_t & end, pth_event_t stop);
                     uint16_t & end);
  void remove (grpaddr_t addr);
};

