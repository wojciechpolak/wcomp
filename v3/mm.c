/*
   V3: mm.c

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

#include <stdio.h>
#include <stdlib.h>

#include "tree.h"

NODE *memory_pool;
NODE *free_memory_pool;
NODE *tmp_memory_pool;

void
mpool_append (NODE **head, NODE *p)
{
  p->memory_link = *head;
  *head = p;
}

/* removes without freeing! */
void
mpool_remove (NODE **head, NODE *r)
{
  NODE *p, *prev = NULL;

  if (!*head)
    return;

  p = *head;
  while (p)
    {
      NODE *next = p->memory_link;
      if (p == r)
	{
	  if (!prev)
	    *head = next;
	  else
	    prev->memory_link = next;
	}
      else
	prev = p;
      p = next;
    }
}


/* Mark & Sweep */

void
mark_node (NODE *node)
{
  mpool_remove (&memory_pool, node);
  mpool_append (&tmp_memory_pool, node);
}

traverse_fp mark_fptab[] = {
  mark_node,  /* NODE_NOOP */
  mark_node,  /* NODE_UNOP */
  mark_node,  /* NODE_BINOP */
  mark_node,  /* NODE_CONST */
  mark_node,  /* NODE_VAR */
  mark_node,  /* NODE_CALL */
  mark_node,  /* NODE_ASGN */
  mark_node,  /* NODE_EXPR */
  mark_node,  /* NODE_RETURN */
  mark_node,  /* NODE_PRINT */
  mark_node,  /* NODE_JUMP */
  mark_node,  /* NODE_COMPOUND  */
  mark_node,  /* NODE_ITERATION */
  mark_node,  /* NODE_CONDITION */
  mark_node,  /* NODE_VAR_DECL */
  mark_node   /* NODE_FNC_DECL */
};

NODE *
mark_free (NODE *root)
{
  tmp_memory_pool = NULL;
  traverse (root, mark_fptab);
  return tmp_memory_pool;
}

void
sweep (NODE *new_pool)
{
  /* Append memory_pool to free_memory_pool */
  NODE *p, *next;
  for (p = memory_pool; p; p = next)
    {
      next = p->memory_link;
      mpool_append (&free_memory_pool, p);
    }

  memory_pool = new_pool;
}

