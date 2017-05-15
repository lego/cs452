#include <ts7200.h>
#include <clock.h>

#define CLOCKS_PER_MILLISECOND (CLOCKS_PER_SEC / 1000)

void clock_init() {

}

ktime_t clock_get_ticks() {
  return clock();
}

unsigned int clock_ms_difference(ktime_t current, ktime_t prev) {
  return (current - prev) / CLOCKS_PER_MILLISECOND;
}
