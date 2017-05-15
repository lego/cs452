#ifndef __CLOCK_H__
#define __CLOCK_H__

#ifdef DEBUG_MODE
// If we're compiling locally, we use built-in clock types
#include <time.h>
typedef clock_t ktime_t;
#else
// Otherwise, expect unsigned integers
typedef unsigned int ktime_t;
#endif

/**
 * Initializes the clock hardware if required
 * On the ARM device, this will configure some memory values
 */
void clock_init();

/**
 * Gets the clock ticks
 * @return a clock value
 * NOTE: this value may under/overflow with enough time
 */
ktime_t clock_get_ticks();

/**
 * Gets the ms difference between two clock values
 */
unsigned int clock_ms_difference(ktime_t current, ktime_t prev);

#endif
