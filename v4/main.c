/*
   V3: main.c

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
#include <unistd.h>
#include "symbol.h"
#include "tree.h"
#include "optimize.h"

extern int parse (void);
extern void open_file (char *);

int verbose;
int errcnt;             /* general error counter */
int optimize_level = 2; /* optimization level */

int
main (int argc, char *argv[])
{
  int status;

  while ((status = getopt (argc, argv, "vO:")) != EOF)
  {
    switch (status) {
    case 'v':
      verbose++;
      break;

    case 'O':
      optimize_level = atoi (optarg);
      break;
    }
  }

  if (argc - optind > 0)
    {
      if (argc - optind > 1) {
	fprintf (stderr, "%s: too many arguments\n", argv[0]);
	return 1;
      }
      open_file (argv[optind]);
    }
	
  status = parse ();

  if (status == 0 && errcnt == 0)
    {
      if (verbose)
	{
	  printf ("=== The parse tree (%d nodes) ===\n\n",
		  get_last_node_id ());
	  print_node (root);
	}
      if (optimize_level > 0)
	{
	  optimize_tree (root);
	  printf ("\n=== After optimization ===\n\n");
	  print_node (root);
	}
    }

  if (verbose)
    {
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
    }

  if (errcnt)
    status = 1;
  printf ("\nCompilation: %s\n", status ? "Failed" : "Passed");
  return status;
}

