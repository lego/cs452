#
# Makefile for CBuffer testing
#
# Disable builtin rules, including %.o: %.c
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

CC     = gcc
DEBUG  = -g
CFLAGS = -Wall -msoft-float -Wno-comment -DDEBUG_MODE --std=c89 $(DEBUG)

INCLUDES= -I../include

LIB_SRCS=$(wildcard ../lib/*.c)
LIB_FILES=$(LIB_SRCS:../lib/%.c=%.o)

TEST_SRCS=$(wildcard *.c)
TEST_FILES=$(TEST_SRCS:.c=.a)

all: clean $(TEST_FILES)

clean:
	-rm -f *.s *.o *.a

%.o: ../lib/%.c
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

%.a: %.c $(LIB_FILES)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

.PHONY: clean
