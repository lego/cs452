#pragma once

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

// FIXME: make this an enum
typedef int task_state_t;
#define STATE_ACTIVE (task_state_t) 1
#define STATE_READY (task_state_t) 2
#define STATE_ZOMBIE (task_state_t) 3
#define STATE_SEND_BLOCKED (task_state_t) 4
#define STATE_RECEIVE_BLOCKED (task_state_t) 5
#define STATE_REPLY_BLOCKED (task_state_t) 6
#define STATE_EVENT_BLOCKED (task_state_t) 7

#define KERNEL_TID -1

// NOTE: priorities can be found in kernel.h

struct TaskDescriptor {
  int tid;
  // For re-allocatable stacks
  int stack_id;
  int parent_tid;
  bool has_started;
  bool was_interrupted;
  int priority;
  kernel_request_t current_request;
  struct TaskDescriptor *next_ready_task;
  cbuffer_t send_queue;
  void *send_queue_buf[MAX_TASKS];
  // TID of target task of Send when REPLY_BLOCKED
  int reply_blocked_on;
  volatile task_state_t state;
  void *stack_pointer;
  void (*entrypoint)();


  bool is_recyclable;

  /* Diagnostics */
  io_time_t execution_time;
  io_time_t send_execution_time;
  io_time_t recv_execution_time;
  io_time_t repl_execution_time;
  char name[128];
};

typedef struct TaskDescriptor task_descriptor_t;

task_descriptor_t *td_create(context_t *ctx, int parent_tid, int priority, void (*entrypoint)(), const char *func_name, bool is_recyclable);

void td_free_stack(int tid);

#define _TaskStackSize 0x09000
extern char *TaskStack;
