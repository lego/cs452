#ifndef __IO_H__
#define __IO_H__

#include <ts7200.h>

/*
 * IO utilities for interfacing with devices
 * these are also points where things get stubbed on x86
 */

/**
 * Initalizes IO for the devices
 */
void io_init();

#ifdef DEBUG_MODE
  #include <time.h>
  typedef clock_t io_time_t;
#else
  typedef unsigned int io_time_t;
#endif

void io_enable_caches();
void io_disable_caches();

/**
 * Gets the time value. Only use this for relative time and calculating
 * actual time using io_time_difference
 */
io_time_t io_get_time();

/**
 * Calculates the millisecond difference between two timing values
 */
unsigned int io_time_difference_ms(io_time_t current, io_time_t previous);

/**
 * Calculates the microsecond difference between two timing values
 */
unsigned int io_time_difference_us(io_time_t current, io_time_t previous);

/**
 * Checks if the channel is ready to put a char
 * @return         status
 *                 0 => OK
 *                 -1 => buffer full
 *                 -2 => ERROR: invalid channel
 */
int io_can_put(int channel);

/**
 * Checks if the channel has a char to get
 * @return         status
 *                 0 => OK
 *                 -1 => buffer empty
 *                 -2 => ERROR: invalid channel
 */
int io_can_get(int channel);

/**
 * Puts a char into a channel
 * @return         status
 *                 0 => OK
 *                 -1 => buffer full
 *                 -1 => ERROR: invalid channel
 */
int io_putc(int channel, char c);

/**
 * Gets a char from a channel
 * NOTE: this has ~undefined behaviour if io_can_get is false
 * @return         character, if something happened result is
 *                 -1 => buffer empty
 *                 -2 => ERROR: invalid channel
 */
int io_getc(int channel);

#endif
