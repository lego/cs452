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
#include <kern/interrupts.h>
#include <kern/syscall.h>
#include <kernel.h>

#if defined(USE_K1)
#include <k1_entry.h>
#elif defined(USE_K2)
#include <k2_entry.h>
#elif defined(USE_K3)
#include <k3_entry.h>
#elif defined(USE_BENCHMARK)
#include <benchmark_entry.h>
#else
#error Bad PROJECT value provided to Makefile. Expected "K1", "K2" or "BENCHMARK"
#endif

#define ENTRY_TASK_PRIORITY 1

task_descriptor_t *active_task;

context_t *ctx;

int idle_task_tid;


static inline kernel_request_t *activate(task_descriptor_t *task) {
  return scheduler_activate_task(task);
}

static inline void handle(kernel_request_t *request) {
  syscall_handle(request);
}

io_time_t idle_time_total;
io_time_t idle_time_start;
io_time_t time_since_idle_print;

static inline void idle_task_pre_activate(task_descriptor_t *task) {
  if (task->priority == 31) {
    idle_time_start = io_get_time();
  }
}

static inline void idle_task_post_activate(task_descriptor_t *task) {
  if (task->priority == 31) {
    io_time_t idle_time_end = io_get_time();
    idle_time_total += (idle_time_end - idle_time_start);

    // Only print idle time every 1s
    if (io_time_difference_us(idle_time_end, time_since_idle_print) >= 1000000) {
      unsigned int idle_time_us = io_time_difference_us(idle_time_total, 0);
      bwprintf(COM2, "Idle task ran for %dus\n\r", idle_time_us);
      idle_time_total = 0;
      time_since_idle_print = idle_time_end;
    }
  }
}

int main() {
  active_task = NULL;
  ctx = NULL;
  idle_task_tid = 0;
  time_since_idle_print = io_get_time();

  /* initialize various kernel components */
  context_switch_init();
  io_init();
  scheduler_init();
  interrupts_init();

  /* initialize core kernel global variables */
  // create shared kernel context memory
  context_t stack_context;
  stack_context.used_descriptors = 0;
  ctx = &stack_context;

  // enable caches here, because these are after initialization
  // io_enable_caches();

  /* create first user task */
  task_descriptor_t *first_user_task = td_create(ctx, KERNEL_TID, ENTRY_TASK_PRIORITY, ENTRY_FUNC);
  scheduler_requeue_task(first_user_task);

  log_kmain("ready_queue_size=%d", scheduler_ready_queue_size());

  // start executing user tasks
  while (scheduler_any_task()) {
    task_descriptor_t *next_task = scheduler_next_task();
    log_kmain("next task tid=%d", next_task->tid);
    idle_task_pre_activate(next_task);
    kernel_request_t *request = activate(next_task);
    idle_task_post_activate(next_task);
    if (next_task->state != STATE_ZOMBIE) {
      handle(request);
    }
  }

  // Clear out any interrupt bits
  // This is needed because for some reason if we run our program more than
  //   once, on the second time we immediately hit the interrupt handler
  //   before starting a user task, even though we should start with
  //   interrupts disabled
  // TODO: figure out why ^
  interrupts_clear_all();

  // io_disable_caches();

  return 0;
}
