#include <basic.h>
#include <bwio.h>
#include <kern/context.h>
#include <kern/syscall.h>
#include <kern/context_switch.h>
#include <kern/scheduler.h>
#include <kern/task_descriptor.h>

void context_switch_init() {
}

void context_switch(kernel_request_t *arg) {
  ctx->descriptors[arg->tid].current_request = *arg;

  // Reschedule the world
  scheduler_reschedule_the_world();
}
