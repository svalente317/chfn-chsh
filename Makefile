#
#    Makefile for chfn and chsh
#    (c) 1994 by salvatore valente <svalente@athena.mit.edu>
#    v1.0 Feb 4, 1994
#
#    this program is free software.  you can redistribute it and
#    modify it under the terms of the gnu general public license.
#    there is no warranty.
#
#    $Author: svalente $
#    $Revision: 1.3 $
#    $Date: 1994/10/16 06:53:53 $
#

PROG1	= chfn
OBJS1	= chfn.o setpwnam.o
PROG2	= chsh
OBJS2	= chsh.o setpwnam.o
CC	= gcc -Wall
CFLAGS	= -O2
LDFLAGS = -s
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
#  install setuid.  whee.
#
install:
	install -o root -g bin -m 4711 $(PROG1) $(BINDIR)
	install -o root -g bin -m 4711 $(PROG2) $(BINDIR)
	install -o root -g man -m 0444 $(PROG1).1 $(MANDIR)
	install -o root -g man -m 0444 $(PROG2).1 $(MANDIR)
