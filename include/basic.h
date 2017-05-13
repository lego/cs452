#ifndef __BASIC_H__
#define __BASIC_H__

/*
 * Types
 */

#if DEBUG_MODE
  #include <assert.h>
  #include <ncurses.h>
#else
// These are provided by ncurses

// Bool
typedef char bool;
  #define true (bool) 1
  #define false (bool) 0

// Null
  #define NULL ( (void *) 0)
#endif


/*
 * Util functions
 */

int a2d( char ch );

char a2i( char ch, char **src, int base, int *nump );

// uint to array
void ui2a( unsigned int num, unsigned int base, char *bf );

// int to array
void i2a( int num, char *bf );

// char to hex
// NOTE: considers only the lower 16 bits
char c2x( char ch );

bool is_digit( char ch );
bool is_alpha( char ch );
bool is_alphanumeric( char ch );
unsigned int jatoui( char *str, int *status );

/*
 * Debug tooling
 */

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
