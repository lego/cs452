#include <basic.h>
#include <kern/interrupts.h>

typedef int (*interrupt_handler)(int);

void interrupts_arch_init() {
  log_interrupt("Init");
  log_interrupt("Clearing all active interrupts");
  interrupts_clear_all();

  // Enable hardware interrupts
  log_interrupt("Enabling TIMER2 interrupts");
  INTERRUPT_ENABLE(INTERRUPT_TIMER2);

  log_interrupt("Enabling UART2 interrupts");
  INTERRUPT_ENABLE(INTERRUPT_UART2);

  log_interrupt("Enabling UART1 interrupts");
  INTERRUPT_ENABLE(INTERRUPT_UART1);
}

void interrupts_enable_irq(await_event_t event_type) {
  if (event_type == EVENT_TIMER) {
    INTERRUPT_ENABLE(INTERRUPT_TIMER2);
  }
}

void interrupts_disable_irq(await_event_t event_type) {
  if (event_type == EVENT_TIMER) {
    INTERRUPT_DISABLE(INTERRUPT_TIMER2);
    INTERRUPT_CLEAR(INTERRUPT_TIMER2);
  }
}

void interrupts_clear_all() {
  VMEM(VIC1_BASE + VIC_CLEAR_OFFSET) = 0xFFFFFFFF;
  VMEM(VIC2_BASE + VIC_CLEAR_OFFSET) = 0xFFFFFFFF;
}
