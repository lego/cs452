#
# Makefile for library components
#
# Disable builtin rules, including %.o: %.c
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

CC      = gcc
AS	    = as
AR	    = ar
CFLAGS  = -fPIC -Wall -mcpu=arm920t -msoft-float
# -Wall: report all warnings
# -fPIC: emit position-independent code
# -I: include header files
# -mcpu=arm920t: generate code for the 920t architecture
# -msoft-float: use software for floating point

INCLUDES = -I../include
# List of includes that will be linked up in the end

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs-32: always create a complete stack frame

ARFLAGS = rcs

SRCS=$(wildcard *.c)
LIBS=$(addprefix lib,$(SRCS:.c=.a))

all: $(LIBS)

%.s: %.c
	$(CC) $(INCLUDES) $(CFLAGS) -S -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

lib%.a: %.o
	$(AR) $(ARFLAGS) $@ $<

clean:
	-rm -f *.s *.a *.o

.PHONY: clean

.PRECIOUS: %.s
