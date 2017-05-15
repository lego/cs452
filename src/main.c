/*
 * The all magical kernel core
 * this is the entrypoint to the kernel and the bulk of magic
 */

#include <alloc.h>
#include <basic.h>
#include <bwio.h>
#include <cbuffer.h>
#include <entry_task.h>
#include <heap.h>
#include <io.h>
#include <kern/context.h>
#include <kern/kernel_request.h>
#include <kern/scheduler.h>
#include <kern/task_descriptor.h>
#include <kernel.h>

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
  /* initialize various kernel components */
  io_init();
  scheduler_init();

  /* initialize core kernel global variables */
  // create shared kernel context memory
  context_t stack_context;
  stack_context.used_descriptors = 0;
  ctx = &stack_context;
  // create the scheduler data structure and memory
  void *heap_buf[MAX_TASKS];
  heap_t stack_heap = heap_create((heapnode_t *)heap_buf, MAX_TASKS);
  schedule_heap = &stack_heap;

  /* create first user task */
  // int tid = ctx->used_descriptors++;
  // int user_task_priority = 1;
  // ctx->descriptors[tid].priority = user_task_priority;
  // ctx->descriptors[tid].tid = tid;
  // ctx->descriptors[tid].parent_tid = -1;
  // ctx->descriptors[tid].entrypoint = entry_task;
  task_descriptor_t *first_user_task = td_create(ctx, KERNEL_TID, PRIORITY_MEDIUM, entry_task);
  heap_push(schedule_heap, first_user_task->priority, first_user_task);

  log_debug("M   starting heap size=%d\n\r", heap_size(schedule_heap));

  // Task descriptor memory space test
  // char* p = NULL;
  // log_debug("M   stack ptr=%x\n\r", (unsigned int) &p);
  //
  // #define TASK_STACK_START (char *) 0x00250000
  // #define TASK_STACK_SIZE  0x00010000
  //
  // char *task_stacks = TASK_STACK_START;
  // log_debug("M   creating task stack ptr=%x\n\r", (unsigned int) task_stacks);
  // int i, j;
  // p = task_stacks;
  // for (i = 0; i < MAX_TASKS; i++) {
  //   log_debug("M   writing to task stack tid=%d ptr=%x\n\r", i, (unsigned int) task_stacks + i*0x00010000);
  //   p++;
  //   for (j = 0; j < TASK_STACK_SIZE; j++) {
  //     p++;
  //     *p = 0xDE;
  //   }
  // }
  //
  // log_debug("M   done writing to task stacks\n\r");

  // start executing user tasks
  while (heap_size(schedule_heap) != 0) {
    log_debug("M   heap size=%d\n\r", heap_size(schedule_heap));
    task_descriptor_t *next_task = schedule();
    log_debug("M   next task tid=%d\n\r", next_task->tid);
    kernel_request_t *request = activate(next_task);
    // TODO: i'm not sure what the best way to create the request abstraction is
    // currently all request handling logic lives in context_switch.c
    handle(request);
  }
  return 0;
}
