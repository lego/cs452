#
# Makefile for busy-wait IO tests
#
## Disable builtin rules, including %.o: %.c
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

CC     = gcc
AS     = as
LD     = ld
CFLAGS = -c -fPIC -Wall -I. -I../include -mcpu=arm920t -msoft-float
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fpic: emit position-independent code
# -Wall: report all warnings

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs: always generate a complete stack frame

LDFLAGS = -init main -Map main.map -N  -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L../lib
LIBRARIES= -lbwio -lbasic -lio_util -lgcc
# List of libraries to link
# WARNING: Fucking scary as hell if you put -lgcc before anything, nothing works

all: main.elf

%.s: %.c
	$(CC) -S -c $(CFLAGS) $(INCLUDES) $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

%.elf: %.o
	$(LD) $(LDFLAGS) -o $@ $< $(LIBRARIES)

clean:
	-rm -f *.elf *.s *.o *.map

.PHONY: clean


.PRECIOUS: %.s %.map
