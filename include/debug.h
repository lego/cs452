/*
 * Debug tooling
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#if DEBUG_MODE
  #include <stdio.h>
void debugger();
  #define log_debug(format, ...) fprintf(stderr, format, __VA_ARGS__)
#else
  #define NOOP do {} while(0)
  #define debugger() NOOP
  #define log_debug(format, ...) NOOP
#endif

#endif
