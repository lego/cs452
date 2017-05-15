#include <basic.h>
#include <bwio.h>
#include <kern/context_switch.h>
#include <kern/context.h>

int context_switch(syscall_t call_no, int arg1, int arg2) {
  // TODO: Add software interupt, passing in arg1, arg2
  // and returning ret
}

void context_switch_in(syscall_t call_no, int arg1, void *arg2) {
  // TODO: Arrive here from the software interupt
  // Pull in args from registers
  int ret;

  switch (call_no) {
  case SYSCALL_MY_TID:
    log_debug("syscall=MyTid ret=%d\n\r", active_task->tid);
    return active_task->tid;
    break;
  case SYSCALL_MY_PARENT_TID:
    log_debug("syscall=MyParentTid ret=%d\n\r", active_task->parent_tid);
    return active_task->parent_tid;
    break;
  case SYSCALL_CREATE:
    log_debug("syscall=Create\n\r");
    break;
  case SYSCALL_PASS:
    log_debug("syscall=Pass\n\r");
    break;
  case SYSCALL_EXIT:
    log_debug("syscall=Exit\n\r");
    break;
  default:
    break;
  }

  // return ret properly into the user context
  ret = 0;

  return;
}
