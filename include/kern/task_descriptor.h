#ifndef __TASK_DESCRIPTOR_H__
#define __TASK_DESCRIPTOR_H__

#include <cbuffer.h>
#include <kern/kernel_request.h>
#include <kernel.h>

// Forward declared struct, because this is circular
struct Context;
// this is done to not typedef declare twice, as this isn't supported by the
// old compiler
#ifndef __DEFINED_CONTEXT_T
#define __DEFINED_CONTEXT_T
typedef struct Context context_t;
#endif

/*
 * A descriptor of everything about a task that the kernel cares about, including:
 * - id and parent id (that spawned the task)
 * - current run state (enum)
 * - all state needed to begin or resume (stack pointer, entry point)
 */

typedef int task_state_t;
#define STATE_ACTIVE (task_state_t) 1
#define STATE_READY (task_state_t) 2
#define STATE_ZOMBIE (task_state_t) 3
#define STATE_SEND_BLOCKED (task_state_t) 4
#define STATE_RECEIVE_BLOCKED (task_state_t) 5

#define KERNEL_TID -1

/*
 * This defines where our task stack is
 * Empiracally (see main.map), the code is loaded into RAM starting
 * at 0x00218000 and isn't that long, so a guestimate was we will never
 * pass 0x00250000 and that allocating space will not bump into anything else
 * NOTE: stacks grown down, so the lower bound is lower than the define
 *
 * FIXME: put asserts where we use the stack, to do sanity checks
 */
#define TASK_STACK_START (char *) 0x00260000
#define TASK_STACK_SIZE  0x00010000

// NOTE: priorities can be found in kernel.h

struct TaskDescriptor {
  int tid;
  int parent_tid;
  bool has_started;
  task_priority_t priority;
  kernel_request_t current_request;
  struct TaskDescriptor *next_ready_task;
  cbuffer_t send_queue;
  void *send_queue_buf[MAX_TASKS];
  volatile task_state_t state;
  void *stack_pointer;
  void (*entrypoint)();
};

typedef struct TaskDescriptor task_descriptor_t;

task_descriptor_t *td_create(context_t *ctx, int parent_tid, task_priority_t priority, void (*entrypoint)());

#endif
