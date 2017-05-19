#include <basic.h>
#include <bwio.h>
#include <kern/context.h>
#include <kern/context_switch.h>
#include <kern/scheduler.h>
#include <kern/task_descriptor.h>

void context_switch_init() {
}

int context_switch(syscall_t call_no, int arg1, void *arg2) {
  int ret_val = 0;

  switch (call_no) {
  case SYSCALL_MY_TID:
    log_syscall("syscall=MyTid ret=%d", active_task->tid);
    ret_val = active_task->tid;
    scheduler_requeue_task(active_task);
    break;
  case SYSCALL_MY_PARENT_TID:
    log_syscall("syscall=MyParentTid ret=%d", active_task->parent_tid);
    ret_val = active_task->parent_tid;
    scheduler_requeue_task(active_task);
    break;
  case SYSCALL_CREATE:
    log_syscall("syscall=Create");
    task_descriptor_t *new_task = td_create(ctx, active_task->tid, arg1, arg2);
    log_syscall("new task priority=%d tid=%d", arg1, new_task->tid);
    scheduler_requeue_task(new_task);
    scheduler_requeue_task(active_task);
    ret_val = new_task->tid;
    break;
  case SYSCALL_PASS:
    log_syscall("syscall=Pass");
    scheduler_requeue_task(active_task);
    break;
  case SYSCALL_EXIT:
    log_syscall("syscall=Exit");
    scheduler_exit_task();
    break;
  default:
    break;
  }

  // Reschedule the world
  scheduler_reschedule_the_world();

  return ret_val;
}
