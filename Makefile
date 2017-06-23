#
# Makefile for the kernel, and library
#

# Disable builtin rules, including %.o: %.c
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

ifndef PROJECT
PROJECT=K4
endif

ifndef LOCAL
# Set of compiler settings for compiling ARM on the student environment
ARCH   = arm
CC     = ./armcheck; gcc
AS     = ./armcheck; as
AR     = ./armcheck; ar
LD     = ./armcheck; ld
CFLAGS = -fPIC -Wall -mcpu=arm920t -msoft-float --std=gnu99 -O2 -DUSE_$(PROJECT) -finline-functions -finline-functions-called-once -Winline -Werror -Wno-unused-variable
# -Wall: report all warnings
# -fPIC: emit position-independent code
# -mcpu=arm920t: generate code for the 920t architecture
# -msoft-float: use software for floating point
# --std=gnu99: use C99, woo! (default is gnu89)
# -O2: lots of optimizing
ASFLAGS  = -mcpu=arm920t -mapcs-32
# -mcpu=arm920t: use assembly code for the 920t architecture
# -mapcs-32: always create a complete stack frame
LDFLAGS = -init main -Map main.map -N  -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 -L.
# TODO: Document what these mean... heh
else
# Set of compiler settings for compiling on a local machine (likely x86, but nbd)
ARCH   = x86
CC     = gcc
CFLAGS = -Wall -msoft-float --std=gnu99 -Wno-comment -DDEBUG_MODE -g -Wno-varargs -Wno-typedef-redefinition -DUSE_$(PROJECT) -finline-functions -Wno-undefined-inline
# -Wall: report all warnings
# -msoft-float: use software for floating point
# --std=gnu99: use C99, same as possible on the school ARM GCC
# -Wno-comment: disable one line comment warnings, they weren't a thing in GNU89
# -DDEBUG_MODE: define DEBUG_MODE for debug purposes
# -g: add symbol information for using GDB
# -Wno-varargs: disable a varargs warning of casting an int to char, no big deal
# -O2: lots of optimizing
endif
ARFLAGS = rcs

# Libraries for linker
# WARNING: Fucking scary as hell. if you put -lgcc before anything, nothing works
# so be careful with the order when you add things
#
# When you add a file it will be in the form of -l<filename>
# NOTE: If you add an ARM specific file, you also need to add -larm<filename>
LIBRARIES= -ltrack -lcbuffer -ljstring -lmap -larmio -lbwio -larmbwio -lbasic -lheap -lalloc -lgcc

# List of includes for headers that will be linked up in the end
INCLUDES = -I./include -I./track
USERLAND_INCLUDES = -I./userland


# Various separate components src files, objs, and bins
LIB_SRCS := $(wildcard lib/*.c) $(wildcard lib/$(ARCH)/*.c) $(wildcard track/*.c)
LIB_BINS := $(LIB_SRCS:.c=.a)
LIB_BINS := $(patsubst lib/$(ARCH)/%.a,lib$(ARCH)%.a,$(LIB_BINS))
LIB_BINS := $(patsubst lib/%.a,lib%.a,$(LIB_BINS))

KERNEL_SRCS := $(wildcard src/*.c) $(wildcard src/$(ARCH)/*.c)
KERNEL_OBJS := $(patsubst src/$(ARCH)/%.c,$(ARCH)%.o,$(KERNEL_SRCS))
KERNEL_OBJS := $(patsubst src/%.c,%.o,$(KERNEL_OBJS))

TEST_SRCS := $(wildcard test/*.c)
TEST_OBJS := $(patsubst test/%.c,%.o,$(TEST_SRCS))
TEST_BINS := $(TEST_OBJS:.o=.a)

USERLAND_SRCS := $(wildcard userland/*.c)
USERLAND_OBJS := $(patsubst userland/%.c,%.o,$(USERLAND_SRCS))

ifdef LOCAL
# If were compiling locally, we care about main.a and test binaries
all: main.a $(TEST_BINS)
else
# Compiling on the student environment, we want to compile the ELF and install it
all: main.elf install
endif

# Depend on main.elf and use the script to copy the file over
install: main.elf
	./upload.sh $<

# ARM binary
main.elf: $(LIB_BINS) $(KERNEL_OBJS) $(USERLAND_OBJS)
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) $(USERLAND_OBJS) -o $@ $(LIBRARIES)

small_main.elf: $(KERNEL_SRCS) $(LIB_SRCS) $(USERLAND_SRCS)
	$(CC) $(CFLAGS) -nostdlib -nostartfiles -ffreestanding -Wl,-Map,main.map -Wl,-N -T orex.ld -Wl,-L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2 $(INCLUDES) $(USERLAND_INCLUDES) $^ -o $@ -Wl,-lgcc


# Local simulation binary
# NOTE: it just explicitly lists a bunch of folders, this is because the
# process of .c => .s => .o drops debugging symbols :(
main.a:
	$(CC) $(CFLAGS) $(INCLUDES) $(USERLAND_INCLUDES) src/*.c src/x86/*.c lib/*.c lib/x86/*.c userland/*.c -lncurses -lpthread -o $@

# ASM files from various locations
%.s: src/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -S -c $< -o $@

%.s: lib/%.c
	$(CC) $(INCLUDES) $(CFLAGS) -S -c $< -o $@

%.s: userland/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -S -c $< -o $@

# architecture specific ASM gets prefixed
# this is done to have both a src/bwio.c and src/arm/bwio.c
$(ARCH)%.s: src/$(ARCH)/%.c
	$(CC) $(INCLUDES) $(USERLAND_INCLUDES) $(CFLAGS) -S -c $< -o $@

$(ARCH)%.s: lib/$(ARCH)/%.c
	$(CC) $(INCLUDES) $(CFLAGS) -S -c $< -o $@

# all object files from ASM files
%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

# create binaries for each test file, depending on all lib code
%.a: test/%.c $(LIB_SRCS)
	$(CC) $(INCLUDES) $(CFLAGS) $< $(LIB_SRCS) -lncurses -lpthread -lcheck -o $@

# create library binaries from object files, for ARM bundling
lib%.a: %.o
	$(AR) $(ARFLAGS) $@ $<

# clean all files in the top-level, the only place we have temp files
clean:
	rm -rf *.o *.s *.elf *.a *.a.dSYM/ *.map

# always run clean (it doesn't produce files)
# also always run main.a because it implicitly depends on all C files
.PHONY: clean main.a

ifndef LOCAL
# if we're compiling ARM, keep the ASM and map files, they're useful
.PRECIOUS: %.s arm%.s %.map
endif
