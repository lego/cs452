#
# Makefile for the simulating test
#
CC    = gcc
DEBUG  = -g
CFLAGS = -Wall -msoft-float --std=c89 -Wno-comment -DDEBUG_MODE $(DEBUG)

KERNEL_INCLUDES=-I../src
USER_INCLUDES=-I../userland
INCLUDES=-I../include

KERNEL_SRCS=$(wildcard ../src/*.c)
KERNEL_FILES=$(KERNEL_SRCS:../src/%.c=%.o)

USERLAND_SRCS=$(wildcard ../userland/*.c)
USERLAND_FILES=$(USERLAND_SRCS:../userland/%.c=%.o)

LIB_SRCS=$(wildcard ../lib/*.c)
LIB_FILES=$(LIB_SRCS:../lib/%.c=%.o)

SIM_SRCS=$(wildcard *.c)
SIM_FILES=$(SIM_SRCS:.c=.o)

all: main.a

%.o: ../userland/%.c
	$(CC) -c $(CFLAGS) $(USER_INCLUDES) $(INCLUDES) $< -o $@

%.o: ../src/%.c
	$(CC) -c $(CFLAGS) $(KERNEL_INCLUDES) $(INCLUDES) $< -o $@

%.o: ../lib/%.c
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $(INCLUDES) -Wno-varargs $< -o $@

main.a: $(SIM_FILES) $(USERLAND_FILES) $(KERNEL_FILES) basic.o
	$(CC) $(CFLAGS) $(KERNEL_INCLUDES) $(INCLUDES) $^ -lncurses -o main.a

clean:
	-rm -f *.s *.o *.a
