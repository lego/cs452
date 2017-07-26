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
#include <kern/globals.h>
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

volatile int program_ended = -1;
int next_starting_task = -1;
int last_started_task = -1;

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
  char taskStack[_TaskStackSize * (MAX_TASK_STACKS + 2)];
  TaskStack = taskStack;
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
  stack_context.used_stacks = 0;
  for (int i = 0; i < MAX_TASKS; i++) {
    stack_context.descriptors[i].state = STATE_ZOMBIE;
  }
  cbuffer_init(&stack_context.freed_stacks, stack_context.freed_stacks_buffer, MAX_TASK_STACKS);
  ctx = &stack_context;

  // enable caches here, because these are after initialization
  io_enable_caches();

  /* create first user task */
  task_descriptor_t *first_user_task = td_create(ctx, KERNEL_TID, PRIORITY_ENTRY_TASK, ENTRY_FUNC, "First user task", false);
  scheduler_requeue_task(first_user_task);

  log_kmain("ready_queue_size=%d", scheduler_ready_queue_size());

  #ifndef DEBUG_MODE
  // FIXME: super awesome hacky "let's make the trains go"
  // this should happen in user code
  bwputc(COM1, 0x60);
  #endif

  // bwputstr(COM2, SAVE_TERMINAL);

  // start executing user tasks
  while (!should_exit) {
    task_descriptor_t *next_task = scheduler_next_task();
    // Condition hit if task was rescheduled and then parent or self
    // was destroyed
    if (next_task->state == STATE_ZOMBIE) continue;
    KASSERT(next_task->state == STATE_READY, "Task had non-ready tid=%d state=%d", next_task->tid, next_task->state);
    log_kmain("next task tid=%d", next_task->tid);
    next_starting_task = next_task->tid;
    task_pre_activate(next_task);
    kernel_request_t *request = activate(next_task);
    task_post_activate(next_task);
    last_started_task = next_task->tid;
    if (next_task->state != STATE_ZOMBIE) {
      handle(request);
    }
  }

  #ifndef DEBUG_MODE
  cleanup(false);
  #endif

  bwputc(COM1, 0x61);
  bwputc(COM1, 0x61);
  bwputc(COM1, 0x61);
  bwputc(COM1, 0x61);

  // io_disable_caches();

  return 0;
}
