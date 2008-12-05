/*
   V1: main.c

   Copyright (C) 2003 Wojciech Polak.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>

extern int parse (void);

int errcnt; /* general error counter */

int
main (void)
{
  int status = parse ();
  if (errcnt)
    status = 1;
  printf ("Syntax analysis: %s\n", status ? "Failed" : "Passed");
  return status;
}

