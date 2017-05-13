/*
 * io_util.c - routines for ARM IO specific interfaces
 *
 */

#include <ncurses.h>
#include <ts7200.h>
#include <io_util.h>
#include <time.h>

#define CLOCKS_PER_MILLISECOND (CLOCKS_PER_SEC / 1000)

void ts7200_init() {
  // Allow buffering stdin
  initscr();
  cbreak();
  noecho();
  nodelay(stdscr, TRUE);
  endwin();
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
}

void ts7200_train_controller_init() {

}

void ts7200_timer3_init() {

}

bool ts7200_uart1_get_cts() {
  return true;
}

time_tt ts7200_timer3_get_value() {
  return clock();
}

unsigned int ts7200_timer_ms_difference(time_tt current, time_tt prev) {
  return (current - prev) / CLOCKS_PER_MILLISECOND;
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
