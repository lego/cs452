#ifndef __TASK_DESCRIPTOR_H__
#define __TASK_DESCRIPTOR_H__

/*
 * A descriptor of everything about a task that the kernel cares about, including:
 * - id and parent id (that spawned the task)
 * - current run state (enum)
 * - all state needed to begin or resume (stack pointer, entry point)
 */

typedef int TaskState;
#define STATE_ACTIVE (TaskState) 1;
#define STATE_READY (TaskState) 2;
#define STATE_ZOMBIE (TaskState) 3;

struct TaskDescriptor {
  int tid;
  int parent_tid;
  int priority;
  struct TaskDescriptor *next_ready_task;
  struct TaskDescriptor *next_send_task;
  TaskState state;
  void *current_stack_pointer;
  void (*entrypoint)();
};

typedef struct TaskDescriptor task_descriptor_t;

#endif
