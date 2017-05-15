#
# Makefile for the kernel, and library
#

# Disable builtin rules, including %.o: %.c
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

ifndef LOCAL
ARCH=arm
CC     = ./armcheck; gcc
AS     = ./armcheck; as
AR     = ./armcheck; ar
LD     = ./armcheck; ld
CFLAGS = -fPIC -Wall -mcpu=arm920t -msoft-float
# -Wall: report all warnings
# -fPIC: emit position-independent code
# -mcpu=arm920t: generate code for the 920t architecture
# -msoft-float: use software for floating point
ASFLAGS  = -mcpu=arm920t -mapcs-32
# -mcpu=arm920t: use assembly code for the 920t architecture
# -mapcs-32: always create a complete stack frame
LDFLAGS = -init main -Map main.map -N  -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L.
else
ARCH=x86
CC     = gcc
CFLAGS = -Wall -msoft-float --std=gnu89 -Wno-comment -DDEBUG_MODE -g -Wno-varargs
# -Wall: report all warnings
# -msoft-float: use software for floating point
# --std=gnu89: use GNU89, default for the school ARM GCC
# -Wno-comment: disable one line comment warnings, they weren't a thing in GNU89
# -DDEBUG_MODE: define DEBUG_MODE for debug purposes
# -g: add symbol information for using GDB
# -Wno-varargs: disable a varargs warning of casting an int to char, no big deal
endif
ARFLAGS = rcs

# Libraries for linker
# WARNING: Fucking scary as hell. if you put -lgcc before anything, nothing works
# so be careful with how or when you add them in the list
LIBRARIES= -lcbuffer -larray -lbwio -lbasic -lio_util -lalloc -lgcc

# List of includes for headers that will be linked up in the end
INCLUDES = -I./include
USERLAND_INCLUDES = -I./userland


# Various separate components src files, objs, and bins
LIB_SRCS := $(wildcard lib/*.c) $(wildcard lib/$(ARCH)/*.c)
LIB_BINS := $(LIB_SRCS:.c=.a)
LIB_BINS := $(patsubst lib/$(ARCH)/%.a,lib%.a,$(LIB_BINS))
LIB_BINS := $(patsubst lib/%.a,lib%.a,$(LIB_BINS))

KERNEL_SRCS := $(wildcard src/*.c) $(wildcard src/$(ARCH)/*.c)
KERNEL_OBJS := $(patsubst src/$(ARCH)/%.c,%.o,$(KERNEL_SRCS))
KERNEL_OBJS := $(patsubst src/%.c,%.o,$(KERNEL_OBJS))

TEST_SRCS := $(wildcard test/*.c)
TEST_OBJS := $(patsubst test/%.c,%.o,$(TEST_SRCS))
TEST_BINS := $(TEST_OBJS:.o=.a)

USERLAND_SRCS := $(wildcard userland/*.c)
USERLAND_OBJS := $(patsubst userland/%.c,%.o,$(USERLAND_SRCS))

ifdef LOCAL
all: main.a $(TEST_BINS)
else
all: main.elf install
endif

install: main.elf
	./upload.sh $<

# ARM binary
main.elf: $(LIB_BINS) $(KERNEL_OBJS) $(USERLAND_OBJS)
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) $(USERLAND_OBJS) -o $@ $(LIBRARIES)

# Local simulator
main.a: $(LIB_BINS) $(KERNEL_OBJS) $(USERLAND_OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(USERLAND_INCLUDES) $^ -lncurses -lpthread -o $@

%.s: src/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -S -c $< -o $@

%.s: src/$(ARCH)/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -S -c $< -o $@

%.s: lib/%.c
	$(CC) $(INCLUDES) $(CFLAGS) -S -c $< -o $@

%.s: userland/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -S -c $< -o $@

%.a: test/%.c $(LIB_BINS)
	$(CC) $(INCLUDES) $(CFLAGS) $< $(LIB_BINS) -o $@

%.s: lib/$(ARCH)/%.c
	$(CC) $(INCLUDES) $(CFLAGS) -S -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

%.a: %_test.s
	$(CC) $(CFLAGS) $< -o $@

lib%.a: %.o
	$(AR) $(ARFLAGS) $@ $<

clean:
	rm -rf *.o *.s *.elf *.a *.a.dSYM/ *.map

.PHONY: clean

ifndef LOCAL
.PRECIOUS: %.s %.map
endif
