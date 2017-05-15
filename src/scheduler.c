#include <basic.h>
#include <heap.h>
#include <kern/context.h>
#include <kern/scheduler.h>

void scheduler_requeue_task(task_descriptor_t *task) {
  heap_push(schedule_heap, task->priority, task);
}

bool scheduler_any_task() {
  return heap_size(schedule_heap) != 0;
}


task_descriptor_t *scheduler_next_task() {
  return heap_pop(schedule_heap);
}
