#ifndef __CONTEXT_H__
#define __CONTEXT_H__

/*
 * A global struct for kernel state
 */

#include <kern/task_descriptor.h>

// Hardcoded maximum used in a number of places
#define MAX_TASKS 100

struct Context {
  task_descriptor_t descriptors[MAX_TASKS];
  char used_descriptors;
};

#ifndef __DEFINED_CONTEXT_T
#define __DEFINED_CONTEXT_T
typedef struct Context context_t;
#endif

/*
 * Global state accessed in a few places
 */

/*
 * active_task is the current running task, NULL if kernel is executing
 * used by:
 * - context_switch.c for current task state during syscalls
 * - context_switch.c for current task state during syscalls
 * - scheduler.c for current task state in switching tasks
 */
extern task_descriptor_t *active_task;
/*
 * ctx is the kernels state. this includes
 * - list of task descriptors, and it's memory
 * used by:
 * - context_switch.c for creating new task descriptors
 * - main.c for creating the initial task descriptor, and init kernel state (only tasks)
 */
extern context_t *ctx;

#endif
