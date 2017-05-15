#include <basic.h>
#include <bwio.h>
#include <kern/context_switch.h>
#include <kern/context.h>
#include <kern/task_descriptor.h>
#include <kern/scheduler.h>
#include <heap.h>

int context_switch(syscall_t call_no, int arg1, void *arg2) {
  int ret_val = 0;

  switch (call_no) {
  case SYSCALL_MY_TID:
    log_debug("syscall=MyTid ret=%d\n\r", active_task->tid);
    ret_val = active_task->tid;
    heap_push(schedule_heap, active_task->priority, active_task);
    break;
  case SYSCALL_MY_PARENT_TID:
    log_debug("syscall=MyParentTid ret=%d\n\r", active_task->parent_tid);
    ret_val = active_task->parent_tid;
    heap_push(schedule_heap, active_task->priority, active_task);
    break;
  case SYSCALL_CREATE:
    log_debug("syscall=Create\n\r");
    unsigned int tid = ctx->used_descriptors++;
    ctx->descriptors[tid] = (task_descriptor_t) {
      .priority = arg1,
      .tid = tid,
      .parent_tid = active_task->tid,
      .entrypoint = arg2
    };
    log_debug("new task priority=%d tid=%d\n\r", arg1, tid);
    heap_push(schedule_heap, arg1, &ctx->descriptors[tid]);
    heap_push(schedule_heap, active_task->priority, active_task);
    ret_val = tid;
    break;
  case SYSCALL_PASS:
    log_debug("syscall=Pass\n\r");
    heap_push(schedule_heap, active_task->priority, active_task);
    break;
  case SYSCALL_EXIT:
    log_debug("syscall=Exit\n\r");
    scheduler_exit_task();
    break;
  default:
    break;
  }

  // Reschedule the world
  scheduler_reschedule_the_world();

  return ret_val;
}
