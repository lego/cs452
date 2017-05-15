#include <basic.h>
#include <kern/context.h>
#include <kern/scheduler.h>

task_descriptor_t *ready_queues[4];
task_descriptor_t *ready_queues_end[4];

void scheduler_init() {
  scheduler_arch_init();
  int i;
  for (i = 0; i < 4; i++) {
    ready_queues[i] = NULL;
    ready_queues_end[i] = NULL;
  }
}

void scheduler_requeue_task(task_descriptor_t *task) {
  if (ready_queues[task->priority] == NULL) {
    ready_queues[task->priority] = task;
    ready_queues_end[task->priority] = task;
  } else {
    ready_queues_end[task->priority]->next_ready_task = task;
    ready_queues_end[task->priority] = task;
  }
}

bool scheduler_any_task() {
  return ready_queues[0] != NULL ||
         ready_queues[1] != NULL ||
         ready_queues[2] != NULL ||
         ready_queues[3] != NULL;
}

int scheduler_ready_queue_size() {
  int count = 0;
  int i;
  task_descriptor_t *current;

  for (i = 0; i < 4; i++) {
    current = ready_queues[i];
    while (current != NULL) {
      count++;
      current = current->next_ready_task;
    }
  }
  return count;
}

task_descriptor_t *scheduler_next_task() {
  // get the first queue with a task
  task_descriptor_t *next_task = NULL;
  if (ready_queues[0] != NULL)
    next_task = ready_queues[0];
  else if (ready_queues[1] != NULL)
    next_task = ready_queues[1];
  else if (ready_queues[2] != NULL)
    next_task = ready_queues[2];
  else if (ready_queues[3] != NULL)
    next_task = ready_queues[3];

  // If this is null, we're screwed, other logic should
  // check scheduler_any_task first
  assert(next_task != NULL);

  // replace the next task in the queue
  ready_queues[next_task->priority] = next_task->next_ready_task;
  // if this is the last task on that queue, replace the end
  if (ready_queues_end[next_task->priority] == next_task) {
    ready_queues_end[next_task->priority] = NULL;
  }
  // set this tasks next task to NULL, as it's now dequeued
  next_task->next_ready_task = NULL;
  return next_task;
}
