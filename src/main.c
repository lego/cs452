/*
 * iotest.c
 */

#define TASKDESCRIPTORINLINE

#include <basic.h>
#include <kern/context.h>
#include <heap.h>
#include <bwio.h>
#include <cbuffer.h>
#include <io.h>
#include <entry_task.h>
#include <kernel.h>
#include <alloc.h>
#include <kern/task_descriptor.h>
#include <kern/kernel_request.h>
#include <kern/scheduler.h>

task_descriptor_t *active_task = NULL;
heap_t *schedule_heap = NULL;
context_t *ctx = NULL;

task_descriptor_t *schedule() {
  task_descriptor_t *next_task = heap_pop(schedule_heap);
  return next_task;
}

kernel_request_t *activate(task_descriptor_t *task) {
  scheduler_activate_task(task);
  kernel_request_t *kr = NULL;
  return kr;
}

void handle(kernel_request_t *request) {
  return;
}

int main() {
  // initialize kernel logic
  io_init();
  scheduler_init();
  context_t stack_context = (context_t) {
    .used_descriptors = 0
  };
  ctx = &stack_context;
  void *heap_buf[MAX_TASKS];
  heap_t stack_heap = heap_create((heapnode_t *)heap_buf, MAX_TASKS);
  schedule_heap = &stack_heap;

  // create first user task
  int tid = ctx->used_descriptors++;
  int user_task_priority = 1;
  ctx->descriptors[tid] = (task_descriptor_t) {
    .priority = user_task_priority,
    .tid = tid,
    .parent_tid = -1,
    .entrypoint = entry_task
  };
  heap_push(schedule_heap, user_task_priority, &ctx->descriptors[tid]);

  // start executing user tasks
  while (heap_size(schedule_heap) != 0) {
    task_descriptor_t *next_task = schedule();
    log_debug("M   next task tid=%d\n\r", next_task->tid);
    kernel_request_t *request = activate(next_task);
    handle(request);
  }
  return 0;
}
