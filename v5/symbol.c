/*
   V5: symbol.c

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
#include <string.h>
#include "symbol.h"

static void copy_to_history (SYMBOL *);
static void free_symbol (SYMBOL *s);

SYMBOL *symbol_functions;
SYMBOL *symbol_variables;
SYMBOL *symbol_history;

int nesting_level;
extern size_t input_line_num;

SYMBOL *
putsym (SYMBOL **s, const char *name, enum symbol_type type)
{
  SYMBOL *new;

  new = (SYMBOL *)malloc (sizeof(SYMBOL));
  if (!new)
    exit (EXIT_FAILURE);

  new->name = strdup (name);
  new->type = type;
  new->sourceline = input_line_num;
  new->ref_count = 0;

  if (type == SYMBOL_VAR)
    {
      variable_t *var;
      var = (variable_t *) malloc (sizeof(variable_t));
      if (!var)
	exit (EXIT_FAILURE);
      memset (var, 0, sizeof(variable_t));
      new->v.var = var;
      new->v.var->level = nesting_level;
    }
  else if (type == SYMBOL_FNC)
    {
      function_t *fnc;
      fnc = (function_t *) malloc (sizeof(function_t));
      if (!fnc)
	exit (EXIT_FAILURE);
      memset (fnc, 0, sizeof(function_t));
      new->v.fnc = fnc;
    }

  if (*s)
    new->next = *s;
  else
    new->next = NULL;
  *s = new;
  return new;
}

SYMBOL *
getsym (SYMBOL *s, const char *name)
{
  SYMBOL *ptr;

  if (s == NULL)
    return NULL;

  for (ptr = s; ptr; ptr = ptr->next)
    {
      if (strcmp (ptr->name, name) == 0)
	return ptr;
    }
  return NULL;
}

SYMLIST *
make_symlist (SYMBOL *s, SYMLIST *next)
{
  SYMLIST *x;

  x = (SYMLIST *)malloc (sizeof(SYMLIST));
  if (x == NULL)
    exit (EXIT_FAILURE);

  x->symbol = s;
  x->next   = next;
  return x;
}

static void
copy_to_history (SYMBOL *s)
{
  if (!s)
    return;
  if (symbol_history)
    s->next = symbol_history;
  else
    s->next = NULL;
  symbol_history = s;
}

static void
free_symbol (SYMBOL *s)
{
  if (!s)
    return;

  free (s->name);
  if (s->type == SYMBOL_VAR && s->v.var)
    free (s->v.var);
  else if (s->type == SYMBOL_FNC && s->v.fnc)
    free (s->v.fnc);
  free (s);
}

void
delsym_level (SYMBOL **s, int level)
{
  SYMBOL *p, *prev = NULL;

  if (!*s)
    return;

  p = *s;
  while (p)
    {
      SYMBOL *next = p->next;
      if (p->type == SYMBOL_VAR && (p->v.var->qualifier == QUA_AUTO ||
				    p->v.var->qualifier == QUA_PARAMETER))
	{
	  if (p->v.var->level < level)
	    break;
	  if (!prev)
	    *s = next;
	  else
	    prev->next = next;

	  /* NOTE: Do not free the symbol, since it may be referenced
	     to by the code */

	  copy_to_history (p); 
	}
      else
	prev = p;
      p = next;
    }
}

void
free_all_symbols (SYMBOL **s)
{
  SYMBOL *ptr, *next;

  for (ptr = *s; ptr; ptr = next)
    {
      next = ptr->next;
      free_symbol (ptr);
    }
  *s = NULL;
}

void
print_all_symbols (SYMBOL *s)
{
  SYMBOL *ptr;

  for (ptr = s; ptr; ptr = ptr->next)
    {
      printf ("Name: %s", ptr->name);

      if (ptr->type == SYMBOL_VAR)
	{
	  printf (", Nlevel: %d", ptr->v.var->level);
	  printf (", Qualifier: ");
	  switch (ptr->v.var->qualifier) {
	  case QUA_GLOBAL:
	    printf ("GLOBAL");
	    printf (", +%ld", ptr->v.var->rel_address);
	    break;
	  case QUA_AUTO:
	    printf ("AUTO");
	    printf (", -%ld", ptr->v.var->rel_address);
	    break;
	  case QUA_PARAMETER:
	    printf ("PARAMETER");
	    printf (", +%ld", ptr->v.var->rel_address);
	    break;
	  default:
	    printf ("UNKNOWN");
	  }
	}
      else if (ptr->type == SYMBOL_FNC)
      {
	SYMLIST *p = ptr->v.fnc->param;

	printf (", Nparam: %d", ptr->v.fnc->nparam);
	printf (", ");

	for (; p; p = p->next)
	  printf ("%s ", p->symbol->name);

	printf (", Nauto: %d", ptr->v.fnc->nauto);
      }

      fputc ('\n', stdout);
    }
}

void
compute_stack_and_data (void)
{
  off_t rel = 1;
  SYMBOL *s = symbol_functions;

  /* function parameters */

  for (; s && s->type == SYMBOL_FNC; s = s->next)
    {
      SYMLIST *p;
      int nparam = s->v.fnc->nparam;
      for (p = s->v.fnc->param; p; p = p->next)
	p->symbol->v.var->rel_address = nparam--;
    }

  /* global variables */

  s = symbol_variables;

  for (; s && s->type == SYMBOL_VAR; s = s->next)
    {
      s->v.var->rel_address = rel++;
    }
}

