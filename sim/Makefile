#
# Makefile for the simulating test
#
XCC    = gcc
DEBUG  = -g
CFLAGS = -Wall -msoft-float -I../include -I../src --std=c89 -Wno-comment -DDEBUG_MODE $(DEBUG)

all: clean main.a

main.a:
	$(XCC) $(CFLAGS) ../src/*.c bwio_stub.c io_util_stub.c ../lib/basic.c -lncurses -o main.a

clean:
	-rm -f *.s *.o *.a
