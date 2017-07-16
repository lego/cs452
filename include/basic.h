#pragma once

/*
 * set of basic helper functions and other C primitives
 * - normally built-in functions
 * - built-in types or definitions
 * - zero-side effect helpers
 *
 */

/*
 * Helpful debugging macros and functions
 * - only applicable to local development
 */
#include <debug.h>

/*
 * Assertions
 */
#include <kassert.h>

// 2^32-1
#define INT_MAX 2147483647

#define ABS(a) ((a >= 0) ? (a) : -(a))

#define MAX(a, ...) _MAX(a, MAX(## __VA_ARGS__))
#define _MAX(a,b) ((a > b) ? (a) : (b))
