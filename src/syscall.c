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
  case SYSCALL_REPLY:
    syscall_reply(task, arg);
    break;
  default:
    log_syscall("WARNING: syscall not handled. syscall_no=%d", task->tid, arg->syscall);
    break;
  }
}


void syscall_create(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=Create", task->tid);
  syscall_create_arg_t *create_arg = arg->arguments;
  task_descriptor_t *new_task = td_create(ctx, task->tid, create_arg->priority, create_arg->entrypoint);
  log_syscall("new task priority=%d tid=%d", task->tid, create_arg->priority, new_task->tid);
  scheduler_requeue_task(new_task);
  scheduler_requeue_task(task);
  ((syscall_pid_ret_t *) arg->ret_val)->tid = new_task->tid;
}

void syscall_my_tid(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=MyTid", task->tid);
  scheduler_requeue_task(task);

  ((syscall_pid_ret_t *) arg->ret_val)->tid = task->tid;
}

void syscall_my_parent_tid(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=MyParentTid ret=%d", task->tid, task->parent_tid);
  scheduler_requeue_task(task);

  ((syscall_pid_ret_t *) arg->ret_val)->tid = task->parent_tid;
}

void syscall_pass(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=Pass", task->tid);
  scheduler_requeue_task(task);
}

void syscall_exit(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=Exit", task->tid);
  // don't reschedule task
  task->state = STATE_ZOMBIE;
  // do some cleanup, like remove the thread (x86)
  scheduler_exit_task(task);
}

void copy_msg(task_descriptor_t *src_task, task_descriptor_t *dest_task) {
  syscall_message_t *src_msg = src_task->current_request.arguments;
  syscall_message_t *dest_msg = dest_task->current_request.ret_val;

  // short circuit conditions
  if (src_msg->msglen == 0) {
    dest_msg->status = 0;
    return;
  } else if (dest_msg->msglen == 0) {
    dest_msg->status = -1;
    return;
  }

  int min_msglen = src_msg->msglen < dest_msg->msglen ? src_msg->msglen : dest_msg->msglen;

  jmemcpy((void *) dest_msg->msg, (void *) src_msg->msg, min_msglen);
  dest_msg->tid = src_task->tid;

  if (src_msg->msglen > min_msglen) {
    dest_msg->status = -1;
  } else {
    dest_msg->status = min_msglen;
  }
}


bool is_valid_task(int tid) {
  return ctx->used_descriptors > tid && ctx->descriptors[tid].state != STATE_ZOMBIE;
}

void syscall_send(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=Send", task->tid);
  task->state = STATE_RECEIVE_BLOCKED;
  syscall_message_t *msg = arg->arguments;

  // check if the target task is valid
  if (!is_valid_task(msg->tid)) {
    msg->status = -2;
    scheduler_requeue_task(task);
    return;
  }

  task_descriptor_t *target_task = &ctx->descriptors[msg->tid];

  if (target_task->state == STATE_SEND_BLOCKED) {
    // if receiver is blocked, copy the message to them and queue them
    copy_msg(task, target_task);

    // start the receiving task
    target_task->state = STATE_READY;
    scheduler_requeue_task(target_task);

    // set this task to reply blocked
    task->state = STATE_REPLY_BLOCKED;
  } else {
    // if receiver is not blocked, add to their send queue
    int status = cbuffer_add(&target_task->send_queue, task);
    // FIXME: handle bad status of cbuffer, likely panic
  }
}

void syscall_receive(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=Receive", task->tid);
  task->state = STATE_SEND_BLOCKED;
  syscall_message_t *msg = arg->ret_val;

  // if senders are blocked, get the message and continue
  if (!cbuffer_empty(&task->send_queue)) {
    int status;
    task_descriptor_t *sending_task = (task_descriptor_t *) cbuffer_pop(&task->send_queue, &status);
    // FIXME: handle bad status of cbuffer, likely panic
    copy_msg(sending_task, task);

    task->state = STATE_READY;
    scheduler_requeue_task(task);

    // reply block the sending task
    sending_task->state = STATE_REPLY_BLOCKED;
  }
}

void syscall_reply(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("syscall=Reply", task->tid);
  syscall_message_t *msg = arg->arguments;

  // check if the target task is valid
  if (!is_valid_task(msg->tid)) {
    msg->status = -2;
    scheduler_requeue_task(task);
    return;
  }

  task_descriptor_t *sending_task = (task_descriptor_t *) &ctx->descriptors[msg->tid];
  if (sending_task->state == STATE_REPLY_BLOCKED) {

    copy_msg(task, sending_task);
    task->state = STATE_READY;
    sending_task->state = STATE_READY;

    // copy destination status over
    syscall_message_t *reply_msg = sending_task->current_request.ret_val;
    msg->status = reply_msg->status;

    scheduler_requeue_task(sending_task);
    scheduler_requeue_task(task);
  } else {
    // if the target task isn't reply blocked, return -3
    msg->status = -3;
    scheduler_requeue_task(task);
    return;
  }
}
