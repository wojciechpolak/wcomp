/*
   V3: mm.h

   Copyright (C) 2003, 2004 Wojciech Polak.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#ifndef _MM_H
#define _MM_H

extern NODE *memory_pool;
extern NODE *free_memory_pool;
extern NODE *tmp_memory_pool;

void mpool_append (NODE **, NODE *);
void mpool_remove (NODE **, NODE *);

NODE *mark_free (NODE *);
void mark_node (NODE *);
void sweep (NODE *);

#endif /* not _MM_H */

