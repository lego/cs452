/*
 * The all magical kernel core
 * this is the entrypoint to the kernel and the bulk of magic
 */

#include <alloc.h>
#include <basic.h>
#include <bwio.h>
#include <cbuffer.h>
#include <entry_task.h>
#include <io.h>
#include <kern/context.h>
#include <kern/context_switch.h>
#include <kern/kernel_request.h>
#include <kern/scheduler.h>
#include <kern/task_descriptor.h>
#include <kernel.h>

task_descriptor_t *active_task = NULL;
context_t *ctx = NULL;
void *kernel_stack_pointer = 0;


kernel_request_t *activate(task_descriptor_t *task) {
  scheduler_activate_task(task);
  kernel_request_t *kr = NULL;
  return kr;
}

void handle(kernel_request_t *request) {
  return;
}

int main() {
  /* initialize various kernel components */
  context_switch_init();
  io_init();
  scheduler_init();

  /* initialize core kernel global variables */
  // create shared kernel context memory
  context_t stack_context;
  stack_context.used_descriptors = 0;
  ctx = &stack_context;

  /* create first user task */
  task_descriptor_t *first_user_task = td_create(ctx, KERNEL_TID, PRIORITY_MEDIUM, entry_task);
  scheduler_requeue_task(first_user_task);

  log_debug("M   ready_queue_size=%d\n\r", scheduler_ready_queue_size());


  // start executing user tasks
  while (scheduler_any_task()) {
    task_descriptor_t *next_task = scheduler_next_task();
    log_debug("M   next task tid=%d\n\r", next_task->tid);
    kernel_request_t *request = activate(next_task);
    // TODO: i'm not sure what the best way to create the request abstraction is
    // currently all request handling logic lives in context_switch.c
    handle(request);
  }
  return 0;
}
