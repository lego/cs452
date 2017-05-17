#include <basic.h>
#include <cbuffer.h>
#include <kern/context.h>
#include <kern/task_descriptor.h>

task_descriptor_t *td_create(context_t *ctx, int parent_tid, task_priority_t priority, void (*entrypoint)()) {
  // TODO: Assert task priority is valid, i.e. in [1,5]
  int tid = ctx->used_descriptors++;
  task_descriptor_t *task = &ctx->descriptors[tid];
  task->priority = priority;
  task->tid = tid;
  task->has_started = false;
  task->parent_tid = parent_tid;
  task->entrypoint = entrypoint;
  task->state = STATE_READY;
  task->next_ready_task = NULL;
  task->stack_pointer = TASK_STACK_START + (TASK_STACK_SIZE * tid);

  cbuffer_init(&task->send_queue, task->send_queue_buf, 100);

  return task;
}
