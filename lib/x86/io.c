/*
 * io_util.c - routines for ARM IO specific interfaces
 *
 */

#include <basic.h>
#include <ncurses.h>
#include <ts7200.h>
#include <io.h>
#include <time.h>

void io_init() {
  // initialize ncurses
  initscr();
  // ??
  cbreak();
  // don't echo back typed characters, we do this
  noecho();
  // don't delay drawing to the screen
  nodelay(stdscr, TRUE);
  // ???
  endwin();
  // remove the buffer to prevent delaying writes
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
}

bool ts7200_uart1_get_cts() {
  return true;
}

int can_put(int channel) {
  if (channel == COM1) {
    return 0;
  } else if (channel == COM2) {
    return 0;
  } else {
    return -2;
  }
}

int ts_putc(int channel, char c) {
  if (channel == COM1) {
    putc(c, stderr);
    return 0;
  } else if (channel == COM2) {
    putchar(c);
    return 0;
  } else {
    return -2;
  }
  return 0;
}

int can_get(int channel) {
  if (channel == COM1) {
    return -1;
  } else if (channel == COM2) {
    int ch = getch();
    if (ch == ERR) {
      return -1;
    } else {
      ungetch(ch);
      return 0;
    }
  } else {
    return -2;
  }
}

int get(int channel) {
  if (channel == COM1) {
    return -1;
  } else if (channel == COM2) {
    return getch();
  } else {
    return -2;
  }
}
