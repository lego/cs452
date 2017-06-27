#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <kern/task_descriptor.h>

/*
 * Internal context switch bits
 *
 * These defines are the different integers associated with software interrupts
 * to denote which syscall is being made
 */
// FIXME: make syscall_t an enum
typedef int syscall_t;
#define SYSCALL_MY_TID (syscall_t) 1
#define SYSCALL_MY_PARENT_TID (syscall_t) 2
#define SYSCALL_CREATE (syscall_t) 3
#define SYSCALL_PASS (syscall_t) 4
#define SYSCALL_EXIT (syscall_t) 5
#define SYSCALL_SEND (syscall_t) 6
#define SYSCALL_RECEIVE (syscall_t) 7
#define SYSCALL_REPLY (syscall_t) 8
#define SYSCALL_AWAIT (syscall_t) 9
#define SYSCALL_EXIT_KERNEL (syscall_t) 10

#define SYSCALL_HW_INT (syscall_t) 99

/*
 * A global struct for kernel state
 */

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
 * - kernel_interface.c for current task state to set in the syscall request
 * - scheduler.c for current task state in switching tasks
 */
extern volatile task_descriptor_t *active_task;
/*
 * ctx is the kernels state. this includes
 * - list of task descriptors, and it's memory
 * used by:
 * - context_switch.c for creating new task descriptors
 * - main.c for creating the initial task descriptor, and init kernel state (only tasks)
 */
extern context_t *ctx;

/*
 * should_exit is a boolean for whether the kernel should
 * exit on the next taks cycle
 */
extern bool should_exit;

#endif
