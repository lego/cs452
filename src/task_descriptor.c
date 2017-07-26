#include <stddef.h>
#include <kassert.h>
#include <bwio.h>
#include <cbuffer.h>
#include <jstring.h>
#include <kern/context.h>
#include <kern/task_descriptor.h>

#ifndef DEBUG_MODE
#else
#include <stdlib.h>
#endif

char *TaskStack;
int next_free_stack(task_descriptor_t * task) {
  if (cbuffer_size(&ctx->freed_stacks) == 0) {
    return ctx->used_stacks++;
  } else {
    return (int) cbuffer_pop(&ctx->freed_stacks, NULL);
  }
}

task_descriptor_t *td_create(context_t *ctx, int parent_tid, int priority, void (*entrypoint)(), const char *func_name, bool is_recyclable) {
  int tid;
  for (tid = 0; tid < MAX_TASKS; tid++) {
    if (ctx->descriptors[tid].state == STATE_ZOMBIE && ctx->descriptors[tid].is_recyclable)
      break;
  }
  KASSERT(tid < MAX_TASKS, "Warning: maximum tasks reached tid=%d", tid);
  if (tid >= ctx->used_descriptors)
    ctx->used_descriptors = tid + 1;
  task_descriptor_t *task = &ctx->descriptors[tid];
  task->priority = priority;
  task->tid = tid;
  task->stack_id = next_free_stack(task);
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
  task->is_recyclable = is_recyclable;
  jstrncpy(task->name, func_name, 128);
  #ifndef DEBUG_MODE
  KASSERT(task->stack_id < MAX_TASK_STACKS, "Maximum amount of task stacks allocated stack_id=%d used_stacks=%d", task->stack_id, ctx->used_stacks);
  task->stack_pointer = TaskStack + (_TaskStackSize * task->stack_id) + _TaskStackSize * 1/* Offset, because the stack grows down */;
  #endif

  cbuffer_init(&task->send_queue, task->send_queue_buf, 100);

  return task;
}

void td_free_stack(int tid) {
  ctx->descriptors[tid].stack_pointer = (void *) 0xDEADBEEF;
  cbuffer_add(&ctx->freed_stacks, (void *) ctx->descriptors[tid].stack_id);
}
