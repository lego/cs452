#include <kern/interrupts.h>

typedef int (*interrupt_handler)(int);

void interrupts_arch_init() {
  // FIXME: fix the correct setting of interrupts
}


void interrupts_enable_irq(await_event_t event_type) {
  if (event_type == EVENT_TIMER) {
    // TODO: set VIC2 13th bit
  }
}

void interrupts_disable_irq(await_event_t event_type) {
  if (event_type == EVENT_TIMER) {
    // TODO: unset VIC2 13th bit
  }
}
