#ifndef __IO_UTIL_H__
#define __IO_UTIL_H__

#include <basic.h>

/**
 * Specifics for ARM interfaces
 * This is stubbed out on x86
 */

#ifdef DEBUG_MODE
#include <time.h>
typedef clock_t time_tt;
#else
typedef unsigned int time_tt;
#endif

time_tt get_ticks();

void ts7200_init();
void ts7200_timer3_init();
void ts7200_train_controller_init();
time_tt ts7200_timer3_get_value();
unsigned int ts7200_timer_ms_difference(time_tt current, time_tt prev);

// FIXME: add documentation

#ifndef DEBUG_MODE
// This is provided by ncurses
#define OK 0
#else
#include <ncurses.h>
#endif

int can_put(int channel);

int ts_putc(int channel, char c);

// int ts_putw(int channel, char *s);


int can_get(int channel);

int get(int channel);

#endif
