# Make setting the right compiler PHONY so it's always run
.PHONY: armcc

#
# Makefile for busy-wait IO library
#
XCC     = gcc
AS	= as
AR	= ar
CFLAGS  = -c -fPIC -Wall -I. -I../include -mcpu=arm920t -msoft-float
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings
# -msoft-float: use software for floating point

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs-32: always create a complete stack frame

ARFLAGS = rcs

all: install

armcc:
	../../armcc

bwio.s: bwio.c armcc
	../../armcheck; $(XCC) -S $(CFLAGS) bwio.c

bwio.o: bwio.s armcc
	../../armcheck; $(AS) $(ASFLAGS) -o bwio.o bwio.s

bwio.a: bwio.o armcc
	../../armcheck; $(AR) $(ARFLAGS) $@ bwio.o

install: bwio.a
	cp bwio.a ../lib/libbwio.a

clean:
	-rm -f *.s *.a *.o
