/*
   V3: main.c

   Copyright (C) 2003 Wojciech Polak.

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
#include <string.h>
#include "symbol.h"
#include "tree.h"
#include "optimize.h"

extern int parse(void);

int verbose;
int errcnt;             /* general error counter */
int optimize_level = 1; /* optimization level */

int
main (int argc, char *argv[])
{
  int status;
  int param = 1;

  while (argc > 1 && param < argc) {
    if (!strcmp (argv[param], "-v"))
	verbose = 1;
    if (!strcmp (argv[param], "-O0"))
	optimize_level = 0;
    param++;
  }

  status = parse ();

  if (status == 0 && errcnt == 0)
    {
      if (verbose)
	{
	  printf ("=== The parse tree (%d nodes) ===\n\n",
		  get_last_node_id());
	  print_node (root);
	}
      if (optimize_level > 0)
	{
	  optimize_tree (root);
	  printf ("\n=== After optimization ===\n\n");
	  print_node (root);
	}
    }

  if (symbol_functions || symbol_variables || symbol_history)
    printf ("\n=== Symbol table ===\n");

  if (symbol_functions)
    {
      printf ("* Functions:\n");
      print_all_symbols (symbol_functions);
      free_all_symbols (&symbol_functions);
    }
  if (symbol_variables)
    {
      printf ("* Variables (present):\n");
      print_all_symbols (symbol_variables);
      free_all_symbols (&symbol_variables);
    }
  if (symbol_history)
    {
      printf ("* Variables (past):\n");
      print_all_symbols (symbol_history);
      free_all_symbols (&symbol_history);
    }

  if (errcnt)
    status = 1;
  printf ("\nCompilation: %s\n", status ? "Failed" : "Passed");
  return status;
}

