##
## V4: Makefile
##
## Copyright (C) 2003, 2004 Wojciech Polak.
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

OUT = v4
CC = gcc
CFLAGS = -Wall -O2 -g

FLEX = flex
BISON = bison
BFLAGS = -d -v -t

all: v4

v4: lex.yy.c gram.tab.c mm.o symbol.o tree.o optimize.o main.o
	$(CC) $(CFLAGS) -o $(OUT) main.o mm.o symbol.o tree.o \
	optimize.o lex.yy.c gram.tab.c

lex.yy.c: lex.l
	$(FLEX) lex.l

gram.tab.c: gram.y
	$(BISON) $(BFLAGS) gram.y

mm.o: mm.c mm.h
	$(CC) $(CFLAGS) -c mm.c

symbol.o: symbol.c symbol.h
	$(CC) $(CFLAGS) -c symbol.c

tree.o: tree.c tree.h
	$(CC) $(CFLAGS) -c tree.c

optimize.o: optimize.c optimize.h
	$(CC) $(CFLAGS) -c optimize.c

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f $(OUT) core *.o lex.yy.c
	rm -f gram.tab.* gram.output

