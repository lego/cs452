#include <basic.h>
#include <bwio.h>
#include <cbuffer.h>
#include <kern/context.h>
#include <kern/task_descriptor.h>

#ifndef DEBUG_MODE
#include <kern/arm/linker.h>
#else
#include <stdlib.h>
#endif

task_descriptor_t *td_create(context_t *ctx, int parent_tid, int priority, void (*entrypoint)(), const char *func_name) {
  // TODO: Assert task priority is valid, i.e. in [1,5]
  int tid = ctx->used_descriptors++;
  if (tid >= MAX_TASKS) {
    bwprintf(COM2, "WARNING: MAXIMUM TASKS REACHED. HELP.");
    #ifndef DEBUG_MODE
    asm volatile("mov pc, #20");
    #else
    exit(1);
    #endif
  }
  task_descriptor_t *task = &ctx->descriptors[tid];
  task->priority = priority;
  task->tid = tid;
  task->has_started = false;
  task->parent_tid = parent_tid;
  task->entrypoint = entrypoint;
  task->state = STATE_READY;
  task->next_ready_task = NULL;
  task->execution_time = 0;
  task->func_name = func_name;
  #ifndef DEBUG_MODE
  task->stack_pointer = _TaskStackStart + (_TaskStackSize * tid);
  #endif

  cbuffer_init(&task->send_queue, task->send_queue_buf, 100);

  return task;
}
