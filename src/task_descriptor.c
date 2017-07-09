#include <stddef.h>
#include <kassert.h>
#include <bwio.h>
#include <cbuffer.h>
#include <kern/context.h>
#include <kern/task_descriptor.h>

#ifndef DEBUG_MODE
#else
#include <stdlib.h>
#endif

char TaskStack[_TaskStackSize * (MAX_TASKS + 2)];

task_descriptor_t *td_create(context_t *ctx, int parent_tid, int priority, void (*entrypoint)(), const char *func_name) {
  // TODO: Assert task priority is valid, i.e. in [1,5]
  int tid = ctx->used_descriptors++;
  KASSERT(tid < MAX_TASKS, "Warning: maximum tasks reached.");
  task_descriptor_t *task = &ctx->descriptors[tid];
  task->priority = priority;
  task->tid = tid;
  task->has_started = false;
  task->parent_tid = parent_tid;
  task->entrypoint = entrypoint;
  task->state = STATE_READY;
  task->next_ready_task = NULL;
  task->execution_time = 0;
  task->send_execution_time = 0;
  task->recv_execution_time = 0;
  task->repl_execution_time = 0;
  task->was_interrupted = false;
  task->func_name = func_name;
  #ifndef DEBUG_MODE
  task->stack_pointer = TaskStack + (_TaskStackSize * tid) + _TaskStackSize * 1/* Offset, because the stack grows down */;
  #endif

  cbuffer_init(&task->send_queue, task->send_queue_buf, 100);

  return task;
}
