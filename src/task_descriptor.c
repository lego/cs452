#include <basic.h>
#include <kern/context.h>
#include <kern/task_descriptor.h>

task_descriptor_t *td_create(context_t *ctx, int parent_tid, task_priority_t priority, void (*entrypoint)()) {
  // TODO: Assert task priority is valid, i.e. in [1,5]
  int tid = ctx->used_descriptors++;
  ctx->descriptors[tid].priority = priority;
  ctx->descriptors[tid].tid = tid;
  ctx->descriptors[tid].parent_tid = parent_tid;
  ctx->descriptors[tid].entrypoint = entrypoint;
  ctx->descriptors[tid].state = STATE_READY;
  ctx->descriptors[tid].next_ready_task = NULL;
  ctx->descriptors[tid].stack_pointer = TASK_STACK_START + (TASK_STACK_SIZE * tid);
  return &ctx->descriptors[tid];
}
