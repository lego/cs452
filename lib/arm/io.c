/*
 * io_util.c - routines for ARM specific interfaces
 *
 */

#include <bwio.h>
#include <io.h>
#include <ts7200.h>

void io_init() {
  bwsetfifo(COM2, OFF);
  bwsetspeed(COM2, 115200);

  int *uart1_flags = (int *)(UART1_BASE + UART_LCRH_OFFSET);
  *uart1_flags |= STP2_MASK;
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
