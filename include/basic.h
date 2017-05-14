#ifndef __BASIC_H__
#define __BASIC_H__

#if DEBUG_MODE
  #include <assert.h>
  #include <ncurses.h>
#else
typedef unsigned size_t;              /* Size of a memory region */
#endif

#include <cdefs.h>
#include <stdbool.h>
#include <null.h>
#include <variadic.h>
#include <debug.h>

/*
 * Built-in functions
 */

void * memcpy(void *destination, const void *source, size_t num);
void * memmove(void *destination, const void *source, size_t num);

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

#endif
