#include <signal.h>
#include <unistd.h>
#include <kern/interrupt_handler.h>
#include <kern/scheduler.h>
#include <pthread.h>
#include <kern/context.h>

#define CLOCK 1

pthread_t clock_thread;

void trigger_interrupt(int interrupt) {
  // FIXME: only trigger when in task code AND not handling interrupts
  pthread_kill(scheduler_x86_get_thread(active_task->tid), SIGUSR1);
}

void *clock_interruptor() {
  while (true) {
    // sleep for 10us
    usleep(10);
    trigger_interrupt(CLOCK);
  }
}

void interrupt_clock_interruptor_init() {
  log_interrupt("Initialize clock interrupts");
  pthread_create(&clock_thread, NULL, &clock_interruptor, 0);
  // start thread watching / sleeping on clock
}

void interrupt_handler_init() {
  log_interrupt("Initialize interrupt handler");

  interrupt_clock_interruptor_init();
}

void handle_interrupt() {

}

void interrupt_interceptor() {
  scheduler_x86_deactivate_task_for_interrupt();
  handle_interrupt();
}

void interrupt_handler_enable_in_task() {
  signal(SIGUSR1, interrupt_interceptor);
}
