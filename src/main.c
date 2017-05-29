/*
 * The all magical kernel core
 * this is the entrypoint to the kernel and the bulk of magic
 */

#include <alloc.h>
#include <basic.h>
#include <bwio.h>
#include <cbuffer.h>
#include <io.h>
#include <kern/context.h>
#include <kern/context_switch.h>
#include <kern/kernel_request.h>
#include <kern/scheduler.h>
#include <kern/task_descriptor.h>
#include <kern/syscall.h>
#include <kernel.h>

#if defined(USE_K1)
#include <k1_entry.h>
#elif defined(USE_K2)
#include <k2_entry.h>
#elif defined(USE_BENCHMARK)
#include <benchmark_entry.h>
#else
#error Bad PROJECT value provided to Makefile. Expected "K1", "K2" or "BENCHMARK"
#endif

#define ENTRY_TASK_PRIORITY 1

task_descriptor_t *active_task;

context_t *ctx;

static inline kernel_request_t *activate(task_descriptor_t *task) {
  return scheduler_activate_task(task);
}

static inline void handle(kernel_request_t *request) {
  syscall_handle(request);
}

int main() {
  active_task = NULL;
  ctx = NULL;

  /* initialize various kernel components */
  context_switch_init();
  io_init();
  scheduler_init();

  /* initialize core kernel global variables */
  // create shared kernel context memory
  context_t stack_context;
  stack_context.used_descriptors = 0;
  ctx = &stack_context;

  // enable caches here, because these are after initialization
  io_enable_caches();

  /* create first user task */
  task_descriptor_t *first_user_task = td_create(ctx, KERNEL_TID, ENTRY_TASK_PRIORITY, ENTRY_FUNC);
  scheduler_requeue_task(first_user_task);

  log_kmain("ready_queue_size=%d", scheduler_ready_queue_size());

  // start executing user tasks
  while (scheduler_any_task()) {
    task_descriptor_t *next_task = scheduler_next_task();
    log_kmain("next task tid=%d", next_task->tid);
    kernel_request_t *request = activate(next_task);
    if (next_task->state != STATE_ZOMBIE) {
      handle(request);
    }
  }

  io_disable_caches();

  return 0;
}
