#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <kern/task_descriptor.h>
#include <heap.h>

#define MAX_TASKS 100

typedef struct {
  task_descriptor descriptors[MAX_TASKS];
  char used_descriptors;
} context;

extern task_descriptor *active_task;
extern context *ctx;
extern heap_t *schedule_heap;


#endif
