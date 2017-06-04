/*
 * io_util.c - routines for ARM specific interfaces
 *
 */

#include <basic.h>
#include <bwio.h>
#include <io.h>
#include <ts7200.h>

#define MAX_TIME 0xFFFFFFFF

void ts7200_timer3_init() {
    // set timer3 to start at maximum value
    int *timer3_load = (int *)(TIMER3_BASE + LDR_OFFSET);
    *timer3_load = 0xFFFFFFFF;

    // set timer3 frequency to 508khz and enable it
    int *timer3_flags = (int *)(TIMER3_BASE + CRTL_OFFSET);
    *timer3_flags = *timer3_flags | CLKSEL_MASK | MODE_MASK | ENABLE_MASK;
}


void ts7200_timer2_init() {
    // set timer2 to start to 10ms in 508khz clock cycles
    int *timer2_load = (int *)(TIMER2_BASE + LDR_OFFSET);
    // this constant is 10000/0.5084689 in hex
    // 10,000 is the amount of ms in us, and 0.5084689 is the amount of us in a clock cycle
    // so the end result is the number of clock cycles in 10ms, 19666.886
    // NOTE: this will have a skew of a few us every tick
    *timer2_load = 0x4CD2;

    // set timer2 frequency to 508khz and enable it
    int *timer2_flags = (int *)(TIMER2_BASE + CRTL_OFFSET);
    *timer2_flags = *timer2_flags | CLKSEL_MASK | MODE_MASK | ENABLE_MASK;
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
  ts7200_timer2_init();
  ts7200_timer3_init();
}

void io_enable_caches() {
  asm volatile (
    "mov r0, #0\n\t"
    // invalidates I-Cache and D-Cache, see arm-920t (ARM 920T technical reference), pg. 312
    "mcr p15, 0, r0, c7, c7, 0\n\t"

    // pull value from coprocessor
    "mrc p15, 0, r0, c1, c0, 0\n\t"
    // enable I-cache (bit 12) ep93xx-user-guide section 2.2.3.3.1, page 43
    "orr r0, r0, #4096\n\t"
    // enable D-cache (bit 3) ep93xx-user-guide section 2.2.3.3.2, page 43
    "orr r0, r0, #4\n\t"
    // store new value into coprocessor
    "mcr p15, 0, r0, c1, c0, 0\n\t"
    // save r0 before stomping on it
    : : : /* Clobber list */ "r0");
}

void io_disable_caches() {
  asm volatile (
    // pull value from coprocessor
    "mrc p15, 0, r0, c1, c0, 0\n\t"
    // disable I-cache (bit 12) ep93xx-user-guide section 2.2.3.3.1, page 43
    "bic r0, r0, #4096\n\t"
    // disable D-cache (bit 3) ep93xx-user-guide section 2.2.3.3.2, page 43
    "bic r0, r0, #4\n\t"
    // store new value into coprocessor
    "mcr p15, 0, r0, c1, c0, 0\n\t"
    : : : /* Clobber list */ "r0");
}

io_time_t io_get_time() {
  int *data = (int *)(TIMER3_BASE + VAL_OFFSET);
  // Taking the compliment helps achieve current - prev
  return MAX_TIME - *data;
}

#define CLOCKS_PER_MILLISECOND 508

unsigned int io_time_difference_ms(io_time_t current, io_time_t prev) {
  // FIXME: This overflow check may not be correct
  if (prev > current) prev = MAX_TIME - prev + current;
  return (current - prev) / CLOCKS_PER_MILLISECOND;
}

unsigned int io_time_difference_us(io_time_t current, io_time_t prev) {
  // FIXME: This overflow check may not be correct
  if (prev > current) prev = MAX_TIME - prev + current;
  // This constant was acquired from Wolfram Alpha for (1/0.5084689), as there are
  // approximately 5.8us per clock tick. if you use the literal (1/0.5084689)
  // that value is integer rounded to 0 :(
  // NOTE: the actual clock speed is 508.4689khz, see ep93xx-user-guide pg. 134 (section 5.1.5.2.2)
  return (current - prev) * 1.96668862146731098008157431064;
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
  int status = can_get(channel);
  if (status != 0) return status;
  c = *data;
  return c;
}
