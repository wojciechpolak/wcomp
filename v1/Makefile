##
## V1: Makefile
##
## Copyright (C) 2003 Wojciech Polak.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

OUT = v1
CC = gcc
CFLAGS = -Wall -O2 -g

FLEX = flex
BISON = bison
BFLAGS = -d -v -t

all: v1

v1: main.c lex.yy.c gram.tab.c
	$(CC) $(CFLAGS) -o $(OUT) main.c lex.yy.c gram.tab.c

lex.yy.c: lex.l
	$(FLEX) lex.l

gram.tab.c: gram.y
	$(BISON) $(BFLAGS) gram.y

clean:
	rm -f $(OUT) core *.o lex.yy.c
	rm -f gram.tab.* gram.output

