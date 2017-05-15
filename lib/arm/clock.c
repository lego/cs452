#include <clock.h>
#include <ts7200.h>

void clock_init() {
  // set timer3 to start at maximum value
  int *timer3_load = (int *)(TIMER3_BASE + LDR_OFFSET);
  *timer3_load = 0xFFFFFFFF;

  // set timer3 frequency to 508khz and enable it
  int *timer3_flags = (int *)(TIMER3_BASE + CRTL_OFFSET);
  *timer3_flags = *timer3_flags | CLKSEL_MASK | ENABLE_MASK;
}

ktime_t clock_get_ticks() {
  int *data = (int *)(TIMER3_BASE + VAL_OFFSET);
  // Taking the compliment helps achieve current - prev
  // FIXME: we can tell the timer to go in reverse
  // FIXME: we also need to ensure we handle underflow
  return (ktime_t) (0xFFFFFFFF - *data);
}

unsigned int clock_ms_difference(ktime_t current, ktime_t prev) {
  if (prev > current) prev = 0xFFFFFFFF - prev + current;
  return (current - prev) / 508;
}
