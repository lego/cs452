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
    log_syscall("WARNING: syscall not handled. syscall_no=%d", arg->syscall);
    break;
  }
}


void syscall_create(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=Create");
  syscall_create_arg_t *create_arg = arg->arguments;
  task_descriptor_t *new_task = td_create(ctx, task->tid, create_arg->priority, create_arg->entrypoint);
  log_syscall("new task priority=%d tid=%d", create_arg->priority, new_task->tid);
  scheduler_requeue_task(new_task);
  scheduler_requeue_task(task);
  ((syscall_pid_ret_t *) arg->ret_val)->tid = new_task->tid;
}

void syscall_my_tid(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=MyTid ret=%d", task->tid);
  scheduler_requeue_task(task);

  ((syscall_pid_ret_t *) arg->ret_val)->tid = task->tid;
}

void syscall_my_parent_tid(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=MyParentTid ret=%d", task->parent_tid);
  scheduler_requeue_task(task);

  ((syscall_pid_ret_t *) arg->ret_val)->tid = task->parent_tid;
}

void syscall_pass(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=Pass");
  scheduler_requeue_task(task);
}

void syscall_exit(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=Exit");
  scheduler_exit_task();
}

void copy_msg(task_descriptor_t *src_task, task_descriptor_t *dest_task) {
  syscall_message_t *src_msg = src_task->current_request.arguments;
  syscall_message_t *dest_msg = dest_task->current_request.ret_val;

  jmemcpy((void *) dest_msg->msg, (void *) src_msg->msg, src_msg->msglen);
  dest_msg->tid = src_task->tid;

  // FIXME: do size checks, and status setting
}

void syscall_send(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=Send");
  task->state = STATE_RECEIVE_BLOCKED;
  syscall_message_t *msg = arg->arguments;
  task_descriptor_t *target_task = &ctx->descriptors[msg->tid];

  // if receiver is not blocked, add to their send queue
  if (target_task->state == STATE_SEND_BLOCKED) {
    copy_msg(task, target_task);

    target_task->state = STATE_READY;
    scheduler_requeue_task(target_task);
    // FIXME: reply block task
    task->state = STATE_READY;
    scheduler_requeue_task(task);
  } else {
    int status = cbuffer_add(&target_task->send_queue, task);
    // FIXME: handle bad status of cbuffer
  }
}

void syscall_receive(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=Receive");
  task->state = STATE_SEND_BLOCKED;
  syscall_message_t *msg = arg->ret_val;

  // if senders are blocked, get the message and continue
  if (!cbuffer_empty(&task->send_queue)) {
    int status;
    task_descriptor_t *sending_task = (task_descriptor_t *) cbuffer_pop(&task->send_queue, &status);
    // FIXME: handle bad status of cbuffer
    copy_msg(sending_task, task);

    task->state = STATE_READY;
    scheduler_requeue_task(task);
    // FIXME: reply block sending_task
    sending_task->state = STATE_READY;
    scheduler_requeue_task(sending_task);
  }
}
