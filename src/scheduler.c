#include <basic.h>
#include <kern/context.h>
#include <kern/scheduler.h>

task_descriptor_t *ready_queues[MAX_PRIORITY + 1];
task_descriptor_t *ready_queues_end[MAX_PRIORITY + 1];

unsigned long int priotities_ready;

void scheduler_init() {
  priotities_ready = 0;
  scheduler_arch_init();
  int i;
  for (i = 0; i < MAX_PRIORITY + 1; i++) {
    ready_queues[i] = NULL;
    ready_queues_end[i] = NULL;
  }
}

void scheduler_requeue_task(task_descriptor_t *task) {
  if (ready_queues[task->priority] == NULL) {
    ready_queues[task->priority] = task;
    ready_queues_end[task->priority] = task;
    priotities_ready |= 1 << task->priority;
  } else {
    ready_queues_end[task->priority]->next_ready_task = task;
    ready_queues_end[task->priority] = task;
  }
}

int scheduler_ready_queue_size() {
  int count = 0;
  int i;
  task_descriptor_t *current;

  for (i = 0; i < MAX_PRIORITY + 1; i++) {
    current = ready_queues[i];
    while (current != NULL) {
      count++;
      current = current->next_ready_task;
    }
  }
  return count;
}

task_descriptor_t *scheduler_next_task() {
  // get the lowest priority with a task
  int next_priority = ctz(priotities_ready);
  task_descriptor_t *next_task = ready_queues[next_priority];

  // If this is null, we're screwed, other logic should
  // check scheduler_any_task first
  assert(next_task != NULL);

  // replace the next task in the queue
  ready_queues[next_task->priority] = next_task->next_ready_task;
  // if this is the last task on that queue, replace the end
  if (ready_queues_end[next_task->priority] == next_task) {
    ready_queues_end[next_task->priority] = NULL;
    priotities_ready &= ~(0x1 << next_task->priority);
  }
  // set this tasks next task to NULL, as it's now dequeued
  next_task->next_ready_task = NULL;
  return next_task;
}
