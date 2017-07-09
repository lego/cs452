/*
 * The all magical kernel core
 * this is the entrypoint to the kernel and the bulk of magic
 */

#include <basic.h>
#include <alloc.h>
#include <bwio.h>
#include <jstring.h>
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
#include <priorities.h>
#include <entries.h>

volatile task_descriptor_t *active_task;
context_t *ctx;
bool should_exit;
char logs[LOG_SIZE];
volatile int log_length;
unsigned int main_fp;

static inline kernel_request_t *activate(task_descriptor_t *task) {
  return scheduler_activate_task(task);
}

static inline void handle(kernel_request_t *request) {
  syscall_handle(request);
}

io_time_t execution_time_start;

// TODO: we should track the timing for all tasks, not just the idle task
static inline void task_pre_activate(task_descriptor_t *task) {
  execution_time_start = io_get_time();
}

static inline void task_post_activate(task_descriptor_t *task) {
  io_time_t execution_time_end = io_get_time();
  task->execution_time += (execution_time_end - execution_time_start);
}

int main() {
  #ifndef DEBUG_MODE
  // saves FP to be able to clean exit to redboot
  asm volatile("mov %0, fp @ save fp" : "=r" (main_fp));
  #endif

  should_exit = false;
  active_task = NULL;
  ctx = NULL;
  log_length = 0;

  allocator_init();

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
  io_enable_caches();

  /* create first user task */
  task_descriptor_t *first_user_task = td_create(ctx, KERNEL_TID, PRIORITY_ENTRY_TASK, ENTRY_FUNC, "First user task");
  scheduler_requeue_task(first_user_task);

  log_kmain("ready_queue_size=%d", scheduler_ready_queue_size());

  #ifndef DEBUG_MODE
  // FIXME: super awesome hacky "let's make the trains go"
  // this should happen in user code
  bwputc(COM1, 0x60);
  #endif

  // start executing user tasks
  while (!should_exit) {
    task_descriptor_t *next_task = scheduler_next_task();
    log_kmain("next task tid=%d", next_task->tid);
    task_pre_activate(next_task);
    kernel_request_t *request = activate(next_task);
    task_post_activate(next_task);
    if (next_task->state != STATE_ZOMBIE) {
      handle(request);
    }
  }

  #ifndef DEBUG_MODE
  cleanup();
  #endif

  // io_disable_caches();

  return 0;
}
