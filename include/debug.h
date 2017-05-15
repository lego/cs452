#ifndef __DEBUG_H__
#define __DEBUG_H__

/*
 * Debug tooling
 */

#if DEBUG_MODE

/**
 * This function is made to only be a breakpoint in GDB
 * By doing 'break debugger', we can stop on any calls to this
 * while we can also stub this to otherwise be a NOP
 */
void debugger();

#include <stdarg.h>
#include <stdio.h>
/**
 * Prints with a formatted string to STDERR for debugging output
 */
#define log_debug(format, ...) fprintf(stderr, format, ## __VA_ARGS__)

#include <assert.h>

#else


/*
 * If we're compiling to ARM, we turn all debugging functionality to NOPs
 */
#define NOP do {} while(0)
#define debugger() NOP
#include <bwio.h>
#define log_debug(format, ...) bwprintf(COM2, format, ## __VA_ARGS__)
// #define log_debug(format, ...) NOP
#define assert(x) NOP

#endif

#endif
