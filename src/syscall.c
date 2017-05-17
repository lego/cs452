#include <basic.h>
#include <kern/syscall.h>
#include <kern/kernel_request.h>
#include <kern/scheduler.h>

void syscall_handle(kernel_request_t *arg) {
  task_descriptor_t *task = &ctx->descriptors[arg->tid];

  task->state = STATE_READY;

  switch (arg->syscall) {
  case SYSCALL_MY_TID:
    syscall_my_tid(task, arg);
    break;
  case SYSCALL_MY_PARENT_TID:
    syscall_my_parent_tid(task, arg);
    break;
  case SYSCALL_CREATE:
    syscall_create(task, arg);
    break;
  case SYSCALL_PASS:
    syscall_pass(task, arg);
    break;
  case SYSCALL_EXIT:
    syscall_exit(task, arg);
    break;
  case SYSCALL_SEND:
    syscall_send(task, arg);
    break;
  case SYSCALL_RECEIVE:
    syscall_receive(task, arg);
    break;
  default:
    log_debug("WARNING: syscall not handled. syscall_no=%d\n\r", arg->syscall);
    break;
  }
}


void syscall_create(task_descriptor_t *task, kernel_request_t *arg) {
  log_debug("syscall=Create\n\r");
  syscall_create_arg_t *create_arg = arg->arguments;
  task_descriptor_t *new_task = td_create(ctx, task->tid, create_arg->priority, create_arg->entrypoint);
  log_debug("new task priority=%d tid=%d\n\r", create_arg->priority, new_task->tid);
  scheduler_requeue_task(new_task);
  scheduler_requeue_task(task);
  ((syscall_pid_ret_t *) arg->ret_val)->tid = new_task->tid;
}

void syscall_my_tid(task_descriptor_t *task, kernel_request_t *arg) {
  log_debug("syscall=MyTid ret=%d\n\r", task->tid);
  scheduler_requeue_task(task);

  ((syscall_pid_ret_t *) arg->ret_val)->tid = task->tid;
}

void syscall_my_parent_tid(task_descriptor_t *task, kernel_request_t *arg) {
  log_debug("syscall=MyParentTid ret=%d\n\r", task->parent_tid);
  scheduler_requeue_task(task);

  ((syscall_pid_ret_t *) arg->ret_val)->tid = task->parent_tid;
}

void syscall_pass(task_descriptor_t *task, kernel_request_t *arg) {
  log_debug("syscall=Pass\n\r");
  scheduler_requeue_task(task);
}

void syscall_exit(task_descriptor_t *task, kernel_request_t *arg) {
  log_debug("syscall=Exit\n\r");
  task->state = STATE_ZOMBIE;
  scheduler_exit_task();
}

void syscall_send(task_descriptor_t *task, kernel_request_t *arg) {
  log_debug("syscall=Send\n\r");
  // syscall_message_t *msg = arg->arguments;
  // task->state
  // // don't reschedule task, it's blocked
  // ((syscall_pid_ret_t *) arg->ret_val)->tid = new_task->tid;
}

void syscall_receive(task_descriptor_t *task, kernel_request_t *arg) {
  log_debug("syscall=Receive\n\r");
  // syscall_message_t *msg = arg->arguments;
  // task->state
  // // don't reschedule task, it's blocked
  // ((syscall_pid_ret_t *) arg->ret_val)->tid = new_task->tid;
}
