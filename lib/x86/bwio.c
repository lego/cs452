/*
 * bwio.c - busy-wait I/O routines for diagnosis
 *
 * Specific to the TS-7200 ARM evaluation board
 *
 */

#include <basic.h>
#include <assert.h>
#include <bwio.h>
#include <ncurses.h>
#include <stdio.h>
#include <ts7200.h>

/*
 * The UARTs are initialized by RedBoot to the following state
 *   115,200 bps
 *   8 bits
 *   no parity
 *   fifos enabled
 */
int bwsetfifo( int channel, int state ) {
  assert(channel == COM1 || channel == COM2);
  assert(state == true || state == false);
  // FIXME: debug output for this
  return 0;
}

int bwsetspeed( int channel, int speed ) {
  assert((channel == COM1 && speed == 2400) || (channel == COM2 && speed == 115200));
  // FIXME: debug output for this
  return 0;
}

int bwputc( int channel, char c ) {
  if (channel == COM1) {
    putc(c, stderr);
    return 0;
  } else if (channel == COM2) {
    putc(c, stdout);
    return 0;
  } else {
    return -1;
  }
}

int bwgetc( int channel ) {
  char c;
  if (channel == COM1) {
    return -1;
  } else if (channel == COM2) {
    while ((c = getch()) == ERR) ;
    return c;
  } else {
    return -1;
  }
}
