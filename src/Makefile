#
# Makefile for busy-wait IO tests
#
XCC     = gcc
AS	= as
LD      = ld
CFLAGS  = -c -fPIC -Wall -I. -I../bwio/include -mcpu=arm920t -msoft-float
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs: always generate a complete stack frame

LDFLAGS = -init main -Map main.map -N  -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L../bwio/lib

all:  main.s main.elf

main.s: main.c
	../armcheck; $(XCC) -S $(CFLAGS) main.c

main.o: main.s
	../armcheck; $(AS) $(ASFLAGS) -o main.o main.s

main.elf: main.o
	../armcheck; $(LD) $(LDFLAGS) -o $@ main.o -lbwio -lgcc

clean:
	-rm -f main.elf *.s *.o main.map
