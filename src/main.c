/*
 * iotest.c
 */

#define TASKDESCRIPTORINLINE

#include <basic.h>
#include <kern/context.h>
#include <heap.h>
#include <bwio.h>
#include <cbuffer.h>
#include <io_util.h>
#include <entry_task.h>
#include <kernel.h>
#include <alloc.h>
#include <kern/task_descriptor.h>
#include <kern/kernel_request.h>
#include <kern/scheduler.h>

task_descriptor *active_task = NULL;
heap_t *schedule_heap = NULL;
context *ctx = NULL;

task_descriptor *schedule() {
  task_descriptor *next_task = heap_pop(schedule_heap);
  return next_task;
}

KernelRequest *activate(task_descriptor *task) {
  scheduler_activate_task(task);
  KernelRequest *kr = NULL;
  return kr;
}

void handle(KernelRequest *request) {
  return;
}

int main() {
  // initialize kernel logic
  ts7200_init();
  scheduler_init();
  context stack_context = (context) {
    .used_descriptors = 0
  };
  ctx = &stack_context;
  void *heap_buf[MAX_TASKS];
  heap_t stack_heap = heap_create((node_t *)heap_buf, MAX_TASKS);
  schedule_heap = &stack_heap;

  // create first user task
  int tid = ctx->used_descriptors++;
  int user_task_priority = 1;
  ctx->descriptors[tid] = (task_descriptor) {
    .priority = user_task_priority,
    .tid = tid,
    .parent_tid = -1,
    .entrypoint = entry_task
  };
  heap_push(schedule_heap, user_task_priority, &ctx->descriptors[tid]);

  // start executing user tasks
  while (heap_size(schedule_heap) != 0) {
    task_descriptor *next_task = schedule();
    log_debug("M   next task tid=%d\n\r", next_task->tid);
    KernelRequest *request = activate(next_task);
    handle(request);
  }
  return 0;
}
