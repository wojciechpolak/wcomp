/*
   V5: symbol.h

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

#ifndef _SYMBOL_H
#define _SYMBOL_H

/* Symbol types */
enum symbol_type
{
  SYMBOL_VAR,    /* variable */
  SYMBOL_FNC     /* function */
};

enum qualifier_type
{
  QUA_GLOBAL,    /* GLOBAL */
  QUA_AUTO,      /* AUTO */
  QUA_PARAMETER  /* PARAMETER */
};

struct variable_struct
{
  int level;                        /* nesting level */
  enum qualifier_type qualifier;    /* qualifier */
  struct node_struct *entry_point;  /* Entry point to the variable */
  off_t rel_address;                /* relative address against TOS */
};

struct varlist_struct
{
  struct varlist_struct *next;
  struct variable_struct *var;
};

struct function_struct
{
  int nparam;                       /* Number of parameters */
  int nauto;                        /* Number of automatic variables */
  struct symlist_struct *param;     /* Parameter list */
  struct node_struct *entry_point;  /* Entry point to the function */
};

struct symbol_struct
{
  struct symbol_struct *next;
  char *name;                       /* name of symbol */
  enum symbol_type type;            /* type of symbol */
  size_t sourceline;                /* source code line number */
  size_t ref_count;                 /* Number of times this symbol
                                       is referenced in the code */

  union {
    struct variable_struct *var;    /* pointer to VAR struct */
    struct function_struct *fnc;    /* pointer to FNC struct */
  } v;
};

struct symlist_struct
{
  struct symlist_struct *next;
  struct symbol_struct *symbol;
};

typedef struct function_struct function_t;
typedef struct variable_struct variable_t;
typedef struct varlist_struct varlist_t;
typedef struct symbol_struct SYMBOL;
typedef struct symlist_struct SYMLIST;

extern SYMBOL *symbol_functions;
extern SYMBOL *symbol_variables;
extern SYMBOL *symbol_history;

extern int nesting_level; /* nesting level */

SYMBOL *putsym (SYMBOL **, const char *, enum symbol_type);
SYMBOL *getsym (SYMBOL *, const char *);
SYMLIST *make_symlist (SYMBOL *, SYMLIST *);
void delsym_level (SYMBOL **, int);
void free_all_symbols (SYMBOL **);
void print_all_symbols (SYMBOL *);

void compute_stack_and_data (void);

#endif /* not _SYMBOL_H */

