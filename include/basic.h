#pragma once

/*
 * set of basic helper functions and other C primitives
 * - normally built-in functions
 * - built-in types or definitions
 * - zero-side effect helpers
 *
 */

/*
 * A set of standard C defitions
 * - COMPILE_ASSERT and INLINE
 */
#include <stddef.h>
/*
 * Built-in C boolean types
 */
#include <stdbool.h>
/*
 * Built-in C variadic defitions for variadic functions such as bwprintf
 */
#include <stdarg.h>
/*
 * Helpful debugging macros and functions
 * - only applicable to local development
 */
#include <debug.h>

/*
 * Assertions
 */
#include <kassert.h>
