#ifndef __BASIC_H__
#define __BASIC_H__

/*
 * set of basic helper functions and other C primitives
 * - normally built-in functions
 * - built-in types or definitions
 * - zero-side effect helpers
 *
 */

#ifndef DEBUG_MODE
// Size of memory region, normally a builtin
// normally builtin
typedef unsigned size_t;
#endif

/*
 * A set of standard C defitions
 * - COMPILE_ASSERT and INLINE
 */
#include <cdefs.h>
/*
 * Built-in C null type
 */
#include <null.h>
/*
 * Built-in C boolean types
 */
#include <stdbool.h>
/*
 * Built-in C variadic defitions for variadic functions such as bwprintf
 */
#include <variadic.h>
/*
 * Helpful debugging macros and functions
 * - only applicable to local development
 */
#include <debug.h>

/*
 * Built-in C functions. These are required because GCC auto-injects them
 * when compiling to ARM, but doesn't include them as per
 * - http://cs107e.github.io/guides/gcc/
 * - https://gcc.gnu.org/onlinedocs/gcc/Standards.html (search memcpy)
 *
 * For now, let's avoid using them incase of bugs, but they're here if direly needed
 * typically you can restructure code to prevent using it, e.g.
 *
 * context_t stack_context = (context_t) {
 *  .used_descriptors = 0
 * };
 *
 * can instead be
 *
 * context_t stack_context;
 * stack_context.used_descriptors = 0;
 */

// void *memcpy(void *destination, const void *source, size_t num);
// void *memmove(void *destination, const void *source, size_t num);

// these memory copies are explicitly called, we use them for message passing
void *jmemcpy(void *destination, const void *source, size_t num);
void *jmemmove(void *destination, const void *source, size_t num);

/*
 * Utilty functions
 */

/**
 * Convert a hex char to an int
 */
int c2d( char ch );

/**
 * Convert a char array to an integer
 * @param  ch   ???
 * @param  src  character array to conver
 * @param  base of the number, at most 16
 * @param  nump ptr to store the result in
 * @return      ???
 */
char a2i( char ch, char **src, int base, int *nump );

/**
 * Convert an unsigned int to a char array
 * @param num  to convert
 * @param base of the number
 * @param bf   buffer to save the array into
 */
void ui2a( unsigned int num, unsigned int base, char *bf );

/**
 * Convert a decimal int to a char array
 * @param num  to convert
 * @param bf   buffer to save the array into
 */
void i2a( int num, char *bf );

/**
 * Convert a character to heximal
 * NOTE: Only considers the lower 4 bits (less than 16)
 */
char c2x( char ch );


// TODO: if possible these should be inline
/**
 * Checks if a character is numeric
 */
bool is_digit( char ch );

/**
 * Checks if a character is alpha
 */
bool is_alpha( char ch );

/**
 * Checks if a character is alphanumeric
 */
bool is_alphanumeric( char ch );


/**
 * String hashing function
 * Implementation is djb2, see http://www.cse.yorku.ca/~oz/hash.html
 * ~fast hashing function
 */
unsigned long hash(unsigned char *str);

#endif
