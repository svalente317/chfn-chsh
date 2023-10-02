#
#    Makefile for chfn and chsh
#    (c) 1994 by Salvatore Valente <svalente@mit.edu>
#    v1.0 Feb 4, 1994
#
#    This program is free software. You can redistribute it and
#    modify it under the terms of the GNU General Public License.
#    There is no warranty.
#

PROG1	= chfn
OBJS1	= chfn.o setpwnam.o
PROG2	= chsh
OBJS2	= chsh.o setpwnam.o
CC	= gcc -Wall
CFLAGS	= -O
LDFLAGS =
LIBS	=

BINDIR	= /usr/bin/
MANDIR	= /usr/man/man1/

all: $(PROG1) $(PROG2)

$(PROG1): $(OBJS1)
	$(CC) $(LDFLAGS) -o $@ $(OBJS1)

$(PROG2): $(OBJS2)
	$(CC) $(LDFLAGS) -o $@ $(OBJS2)

.c.o:
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm -f $(PROG1) $(PROG2) *.o *~

#
#  install setuid root.
#
install:
	install -o root -g bin -m 4711 $(PROG1) $(BINDIR)
	install -o root -g bin -m 4711 $(PROG2) $(BINDIR)
	install -o root -g man -m 0444 $(PROG1).1 $(MANDIR)
	install -o root -g man -m 0444 $(PROG2).1 $(MANDIR)
