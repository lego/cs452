/*
 * io_util.c - routines for ARM specific interfaces
 *
 */

#include <basic.h>
#include <bwio.h>
#include <io.h>
#include <ts7200.h>

void ts7200_timer3_init() {
    // set timer3 to start at maximum value
    int *timer3_load = (int *)(TIMER3_BASE + LDR_OFFSET);
    *timer3_load = 0xFFFFFFFF;

    // set timer3 frequency to 508khz and enable it
    int *timer3_flags = (int *)(TIMER3_BASE + CRTL_OFFSET);
    *timer3_flags = *timer3_flags | CLKSEL_MASK | ENABLE_MASK;
}

void ts7200_uart1_init() {
  // FIXME: we probably want to hard set the flags in case they were messed up
  int *uart1_flags = (int *)(UART1_BASE + UART_LCRH_OFFSET);
  *uart1_flags |= STP2_MASK;
}

void ts7200_uart2_init() {
  // FIXME: we probably want to hard set the flags in case they were messed up
  bwsetfifo(COM2, OFF);
  bwsetspeed(COM2, 115200);
}

void io_init() {
  ts7200_uart1_init();
  ts7200_uart2_init();
  ts7200_timer3_init();
}

io_time_t io_get_time() {
  int *data = (int *)(TIMER3_BASE + VAL_OFFSET);
  // Taking the compliment helps achieve current - prev
  return 0xFFFFFFFF - *data;
}


#define CLOCKS_PER_MILLISECOND 508

unsigned int io_time_difference_ms(io_time_t current, io_time_t prev) {
  // FIXME: This overflow check may not be correct
  if (prev > current) prev = 0xFFFFFFFF - prev + current;
  return (current - prev) / CLOCKS_PER_MILLISECOND;
}

unsigned int io_time_difference_us(io_time_t current, io_time_t prev) {
  // FIXME: This overflow check may not be correct
  if (prev > current) prev = 0xFFFFFFFF - prev + current;
  // This constant was acquired from Wolfram Alpha for (1/0.508), as there are
  // approximately 5.8us per clock tick
  return (current - prev) * 1.96850393700787401574803;
}

bool ts7200_uart1_get_cts() {
  int *uart1_flags = (int *)(UART1_BASE + UART_FLAG_OFFSET);
  return (*uart1_flags & CTS_MASK) ? true : false;
}

// Returns status:
// 0 => OK
// -1 => ERROR: UART buffer is full
// -2 => ERROR: bad channel
int can_put(int channel) {
  int *flags;
  switch( channel ) {
  case COM1:
    flags = (int *)( UART1_BASE + UART_FLAG_OFFSET );
    break;
  case COM2:
    flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
    break;
  default:
    return -2;
    break;
  }
  if (*flags & TXFF_MASK && !(*flags & CTS_MASK)) {
    return -1;
  }
  return 0;
}

// Returns status:
// 0 => OK
// -1 => ERROR: UART buffer is full
// -2 => ERROR: bad channel
int ts_putc(int channel, char c) {
  int *data;
  switch( channel ) {
  case COM1:
    data = (int *)( UART1_BASE + UART_DATA_OFFSET );
    break;
  case COM2:
    data = (int *)( UART2_BASE + UART_DATA_OFFSET );
    break;
  default:
    return -2;
    break;
  }
  int status = can_put(channel);
  if (status != 0) return status;
  *data = c;
  return 0;
}

// Returns status:
// 0 => OK
// -1 => ERROR: UART buffer is empty
// -2 => ERROR: bad channel
int can_get(int channel) {
  int *flags;
  switch( channel ) {
  case COM1:
    flags = (int *)( UART1_BASE + UART_FLAG_OFFSET );
    break;
  case COM2:
    flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
    break;
  default:
    return -2;
    break;
  }

  if (*flags & RXFE_MASK) {
    return -1;
  }

  return 0;
}

// Returns status:
// 0 => OK
// -1 => ERROR: UART buffer is empty
// -2 => ERROR: bad channel
int get(int channel) {
  int *data;
  unsigned char c;
  switch( channel ) {
  case COM1:
    data = (int *)( UART1_BASE + UART_DATA_OFFSET );
    break;
  case COM2:
    data = (int *)( UART2_BASE + UART_DATA_OFFSET );
    break;
  default:
    return -2;
    break;
  }
  int status = can_put(channel);
  if (status != 0) return status;
  c = *data;
  return c;
}
