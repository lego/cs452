/*
 * io_util.c - routines for ARM IO specific interfaces
 *
 */

#include <basic.h>
#include <io.h>
#include <ncurses.h>
#include <time.h>
#include <ts7200.h>

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

#define CLOCKS_PER_MILLISECOND (CLOCKS_PER_SEC / 1000)
#define CLOCKS_PER_MICROSECOND (CLOCKS_PER_MILLISECOND / 1000)

io_time_t io_get_time() {
  return clock();
}

unsigned int io_time_difference_ms(io_time_t current, io_time_t prev) {
  return (current - prev) / CLOCKS_PER_MILLISECOND;
}

unsigned int io_time_difference_us(io_time_t current, io_time_t prev) {
  return (current - prev) / CLOCKS_PER_MICROSECOND;
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
