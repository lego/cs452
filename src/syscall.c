#include <basic.h>
#include <util.h>

#include <alloc.h>
#include <bwio.h>
#include <kern/syscall.h>
#include <kern/kernel_request.h>
#include <kern/scheduler.h>
#include <kern/interrupts.h>

io_time_t *expected_ptr;
io_time_t beginning_recording_time;

void pre_time_recording(io_time_t *recording_time) {
  expected_ptr = recording_time;
  beginning_recording_time = io_get_time();
}

void post_time_recording(io_time_t *recording_time) {
  KASSERT(expected_ptr == recording_time, "Something happened. Recording times failed as the beginning is not the same as the end.");
  expected_ptr = NULL;
  io_time_t now = io_get_time();
  *recording_time = now - beginning_recording_time;
}

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
  case SYSCALL_EXIT_KERNEL:
    syscall_exit_kernel(task, arg);
    break;
  case SYSCALL_SEND:
    pre_time_recording(&task->send_execution_time);
    syscall_send(task, arg);
    post_time_recording(&task->send_execution_time);
    break;
  case SYSCALL_RECEIVE:
    pre_time_recording(&task->recv_execution_time);
    syscall_receive(task, arg);
    post_time_recording(&task->recv_execution_time);
    break;
  case SYSCALL_REPLY:
    pre_time_recording(&task->repl_execution_time);
    syscall_reply(task, arg);
    post_time_recording(&task->repl_execution_time);
    break;
  case SYSCALL_AWAIT:
    syscall_await(task, arg);
    break;
  case SYSCALL_MALLOC:
    syscall_malloc(task, arg);
    break;
  case SYSCALL_FREE:
    syscall_free(task, arg);
    break;
  case SYSCALL_DESTROY:
    syscall_destroy(task, arg);
    break;
  case SYSCALL_HW_INT:
    hwi(task, arg);
    break;
  default:
    KASSERT(false, "syscall not handled. tid=%d syscall_no=%d\n\r", task->tid, arg->syscall);
    break;
  }
}

void syscall_create(task_descriptor_t *task, kernel_request_t *arg) {
  syscall_create_arg_t *create_arg = arg->arguments;
  task_descriptor_t *new_task = td_create(ctx, task->tid, create_arg->priority, create_arg->entrypoint, create_arg->func_name, create_arg->is_recyclable);
  log_syscall("Create priority=%d tid=%d", task->tid, create_arg->priority, new_task->tid);
  scheduler_requeue_task(new_task);
  scheduler_requeue_task(task);
  ((syscall_pid_ret_t *) arg->ret_val)->tid = new_task->tid;
}

void syscall_my_tid(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("MyTid", task->tid);
  scheduler_requeue_task(task);

  ((syscall_pid_ret_t *) arg->ret_val)->tid = task->tid;
}

void syscall_my_parent_tid(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("MyParentTid ret=%d", task->tid, task->parent_tid);
  scheduler_requeue_task(task);

  ((syscall_pid_ret_t *) arg->ret_val)->tid = task->parent_tid;
}

void syscall_pass(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("Pass", task->tid);
  scheduler_requeue_task(task);
}

void syscall_malloc(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("Malloc", task->tid);
  unsigned int size = (unsigned int) arg->arguments;
  *(void **) arg->ret_val = alloc(size);
  scheduler_requeue_task(task);
}

void free_message_blocked_tasks(int tid) {
  task_descriptor_t *task = &ctx->descriptors[tid];
  // Free up the sendQ
  while (!cbuffer_empty(&task->send_queue)) {
    task_descriptor_t *sending_task = (task_descriptor_t *) cbuffer_pop(&task->send_queue, NULL);
    syscall_message_t *sending_task_ret = sending_task->current_request.ret_val;
    sending_task_ret->status = -3; // denotes zombie'd task
    sending_task->state = STATE_READY;
    scheduler_requeue_task(sending_task);
  }

  // Free up REPLY_BLOCKED tasks
  for (int i = 0; i < ctx->used_descriptors; i++) {
    task_descriptor_t *task = &ctx->descriptors[i];

    if (task->reply_blocked_on == tid && task->state == STATE_REPLY_BLOCKED) {
      syscall_message_t *task_msg = task->current_request.ret_val;
      task_msg->status = -3;
      task->state = STATE_READY;
      scheduler_requeue_task(task);
    }
  }
}

void syscall_destroy(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("Destroy", task->tid);
  unsigned int tid = (unsigned int) arg->arguments;
  // If this task is getting destroyed, it'll be skipped in main.c
  scheduler_requeue_task(task);

  void *children_buffer[MAX_TASKS];
  cbuffer_t children;
  cbuffer_init(&children, children_buffer, MAX_TASKS);
  cbuffer_add(&children, (void *) tid);
  // "recursively" find and destroy all of the children of this task
  while (cbuffer_size(&children) > 0) {
    int next_to_kill = (int) cbuffer_pop(&children, NULL);
    // Is the child not already dead? if so, re-allocate resources
    if (ctx->descriptors[next_to_kill].state != STATE_ZOMBIE) {
      ctx->descriptors[next_to_kill].state = STATE_ZOMBIE;
      td_free_stack(next_to_kill);
      free_message_blocked_tasks(next_to_kill);
    }
    // We still need to recursively search for it's children
    for (int i = 0; i < ctx->used_descriptors; i++) {
      if (ctx->descriptors[i].parent_tid == next_to_kill) {
        cbuffer_add(&children, (void *) i);
      }
    }
  }
}

void syscall_free(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("Free", task->tid);
  void *data = arg->arguments;
  *(int *) arg->ret_val = jfree(data);
  scheduler_requeue_task(task);
}

void syscall_exit(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("Exit", task->tid);
  // don't reschedule task
  task->state = STATE_ZOMBIE;
  td_free_stack(task->tid);
  free_message_blocked_tasks(task->tid);
  // do some cleanup, like remove the thread (x86)
  scheduler_exit_task(task);
}

void syscall_exit_kernel(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("ExitKernel", task->tid);
  // don't reschedule task
  task->state = STATE_ZOMBIE;
  // do some cleanup, like remove the thread (x86)
  scheduler_exit_task(task);
  should_exit = true;
}

void copy_msg(task_descriptor_t *src_task, task_descriptor_t *dest_task) {
  syscall_message_t *src_msg = src_task->current_request.arguments;
  syscall_message_t *dest_msg = dest_task->current_request.ret_val;

  // Always let the dest_msg know the tid of the sender, even if there's a problem
  dest_msg->tid = src_task->tid;

  // short circuit conditions
  if (src_msg->msglen == 0) {
    // if there's nothing to send, just return
    dest_msg->status = 0;
    return;
  } else if (dest_msg->msglen == 0) {
    // not ok if the src_msg is sending something
    if (src_msg->msglen != 0) {
      dest_msg->status = -1;
    }
    return;
  }

  int min_msglen = src_msg->msglen < dest_msg->msglen ? src_msg->msglen : dest_msg->msglen;

  jmemcpy((void *) dest_msg->msg, (void *) src_msg->msg, min_msglen);

  if (src_msg->msglen > min_msglen) {
    dest_msg->status = -1;
  } else {
    dest_msg->status = min_msglen;
  }
}


bool is_valid_task(int tid) {
  return ctx->used_descriptors > tid && tid >= 0;
}

void syscall_send(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("Send", task->tid);
  task->state = STATE_RECEIVE_BLOCKED;
  syscall_message_t *msg = arg->arguments;

  // check if the target task is valid
  KASSERT(is_valid_task(msg->tid), "Sending to impossible task %d", msg->tid);

  if (ctx->descriptors[msg->tid].state == STATE_ZOMBIE) {
    msg->status = -3;
    task->state = STATE_READY;
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
    task->reply_blocked_on = target_task->tid;
  } else {
    // if receiver is not blocked, add to their send queue
    /* int status = */ cbuffer_add(&target_task->send_queue, task);
    // FIXME: handle bad status of cbuffer, likely panic
  }
}

void syscall_receive(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("Receive", task->tid);
  task->state = STATE_SEND_BLOCKED;

  // if senders are blocked, get the message and continue
  if (!cbuffer_empty(&task->send_queue)) {
    int status;
    task_descriptor_t *sending_task = (task_descriptor_t *) cbuffer_pop(&task->send_queue, &status);

    while (sending_task->state == STATE_ZOMBIE && !cbuffer_empty(&task->send_queue)) {
      sending_task = (task_descriptor_t *) cbuffer_pop(&task->send_queue, &status);
    }

    if (sending_task->state == STATE_ZOMBIE) {
      syscall_message_t * msg = arg->ret_val;
      msg->status = -2;
      task->state = STATE_READY;
      scheduler_requeue_task(task);
      return;
    }

    // FIXME: handle bad status of cbuffer, likely panic
    copy_msg(sending_task, task);

    task->state = STATE_READY;
    scheduler_requeue_task(task);

    // reply block the sending task
    sending_task->state = STATE_REPLY_BLOCKED;
    sending_task->reply_blocked_on = task->tid;
  }
}

void syscall_reply(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("Reply", task->tid);
  syscall_message_t *msg = arg->arguments;

  // check if the target task is valid
  KASSERT(is_valid_task(msg->tid), "Replying to impossible task %d", msg->tid);

  task_descriptor_t *sending_task = (task_descriptor_t *) &ctx->descriptors[msg->tid];
  if (sending_task->state == STATE_REPLY_BLOCKED && sending_task->reply_blocked_on == task->tid) {

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
    task->state = STATE_READY;
    scheduler_requeue_task(task);
    return;
  }
}

void syscall_await(task_descriptor_t *task, kernel_request_t *arg) {
  log_syscall("Await", task->tid);
  syscall_await_arg_t *await_arg = arg->arguments;
  await_event_t event_type = await_arg->event;

  #ifndef DEBUG_MODE
  if (event_type == EVENT_UART1_RX) {
    VMEM(UART1_BASE + UART_CTLR_OFFSET) |= RIEN_MASK;
  }
  if (event_type == EVENT_UART1_TX) {
    VMEM(UART1_BASE + UART_CTLR_OFFSET) |= TIEN_MASK | MSIEN_MASK;
  }
  if (event_type == EVENT_UART2_RX) {
    VMEM(UART2_BASE + UART_CTLR_OFFSET) |= RIEN_MASK;
  }
  if (event_type == EVENT_UART2_TX) {
    VMEM(UART2_BASE + UART_CTLR_OFFSET) |= TIEN_MASK;
  }
  #endif

  interrupts_set_waiting_task(event_type, task);

  task->state = STATE_EVENT_BLOCKED;
}

void hwi(task_descriptor_t *task, kernel_request_t *arg) {
  task->was_interrupted = true;

  if (IS_INTERRUPT_ACTIVE(INTERRUPT_TIMER2)) {
    hwi_timer2(task, arg);
  } else if (IS_INTERRUPT_ACTIVE(INTERRUPT_UART1)) {
    if (VMEM(UART1_BASE + UART_INTR_OFFSET) & UART_INTR_RX) {
      hwi_uart1_rx(task, arg);
    }
    if (VMEM(UART1_BASE + UART_INTR_OFFSET) & UART_INTR_TX) {
      hwi_uart1_tx(task, arg);
    }
    if (VMEM(UART1_BASE + UART_INTR_OFFSET) & UART_INTR_MS) {
      hwi_uart1_modem(task, arg);
    }
    scheduler_requeue_task(task);
  } else if (IS_INTERRUPT_ACTIVE(INTERRUPT_UART2)) {
    if (VMEM(UART2_BASE + UART_INTR_OFFSET) & UART_INTR_RX) {
      hwi_uart2_rx(task, arg);
    } else if (VMEM(UART2_BASE + UART_INTR_OFFSET) & UART_INTR_TX) {
      hwi_uart2_tx(task, arg);
    }
    scheduler_requeue_task(task);
  } else {
    log_interrupt("HWI=Unknown interrupt");
    scheduler_requeue_task(task);
  }
}

task_descriptor_t *hwi_unblock_task_for_event(await_event_t event) {
  task_descriptor_t *event_blocked_task = interrupts_get_waiting_task(event);
  if (event_blocked_task != NULL) {
    interrupts_clear_waiting_task(event);
    event_blocked_task->state = STATE_READY;
    scheduler_requeue_task(event_blocked_task);
  }
  return event_blocked_task;
}

void hwi_uart2_tx(task_descriptor_t *task, kernel_request_t *arg) {
  log_interrupt("HWI=UART 2 TX interrupt");

  // write character
  task_descriptor_t *event_blocked_task = interrupts_get_waiting_task(EVENT_UART2_TX);
  syscall_await_arg_t *await_arg = event_blocked_task->current_request.arguments;
  VMEM(UART2_BASE + UART_DATA_OFFSET) = await_arg->arg;

  // disable interrupt
  VMEM(UART2_BASE + UART_CTLR_OFFSET) &= ~TIEN_MASK;

  hwi_unblock_task_for_event(EVENT_UART2_TX);
}

void hwi_uart2_rx(task_descriptor_t *task, kernel_request_t *arg) {
  log_interrupt("HWI=UART 2 RX interrupt");
  hwi_unblock_task_for_event(EVENT_UART2_RX);
  VMEM(UART2_BASE + UART_CTLR_OFFSET) &= ~RIEN_MASK;
}

static bool uart1_tx_saw_low = false;

void hwi_uart1_tx(task_descriptor_t *task, kernel_request_t *arg) {
  log_interrupt("HWI=UART 1 TX interrupt");

  // write character
  task_descriptor_t *event_blocked_task = interrupts_get_waiting_task(EVENT_UART1_TX);
  syscall_await_arg_t *await_arg = event_blocked_task->current_request.arguments;
  uart1_tx_saw_low = false;
  VMEM(UART1_BASE + UART_DATA_OFFSET) = await_arg->arg;

  VMEM(UART1_BASE + UART_CTLR_OFFSET) &= ~TIEN_MASK;
}

void hwi_uart1_modem(task_descriptor_t *task, kernel_request_t *arg) {
  log_interrupt("HWI=UART 1 MODEM interrupt");

  bool cts = VMEM(UART1_BASE + UART_FLAG_OFFSET) & CTS_MASK;
  if (!cts) {
    uart1_tx_saw_low = true;
  } else if (uart1_tx_saw_low) {
    hwi_unblock_task_for_event(EVENT_UART1_TX);
  }

  // Clear the modem interrupt;
  VMEM(UART1_BASE + UART_INTR_OFFSET) = 0x0;
}

void hwi_uart1_rx(task_descriptor_t *task, kernel_request_t *arg) {
  log_interrupt("HWI=UART 1 RX interrupt");
  hwi_unblock_task_for_event(EVENT_UART1_RX);
  VMEM(UART1_BASE + UART_CTLR_OFFSET) &= ~RIEN_MASK;
}

void hwi_timer2(task_descriptor_t *task, kernel_request_t *arg) {
  log_interrupt("HWI=Timer 2 interrupt");
  hwi_unblock_task_for_event(EVENT_TIMER);
  *((int*)(TIMER2_BASE+CLR_OFFSET)) = 0x0;
  scheduler_requeue_task(task);
}
