#include <basic.h>
#include <bwio.h>
#include <kern/context.h>
#include <kern/context_switch.h>
#include <kern/kernel_request.h>
#include <kernel.h>
#include <nameserver.h>
#include <jstring.h>

int Create(int priority, void (*entrypoint)()) {
  assert(0 <= priority && priority < 32);

  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_CREATE;

  syscall_create_arg_t arg;
  arg.priority = priority;
  arg.entrypoint = entrypoint;

  request.arguments = &arg;

  // NOTE: this is kinda weird, the return value is set in userspace by the kernel
  syscall_pid_ret_t ret_val;
  request.ret_val = &ret_val;
  context_switch(&request);
  return ret_val.tid;
}

int MyTid( ) {
  // Don't make data syscall, but still reschedule
  Pass();
  return active_task->tid;
}

int MyParentTid( ) {
  // Don't make data syscall, but still reschedule
  Pass();
  return active_task->parent_tid;
}

void Pass( ) {
  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_PASS;
  context_switch(&request);
}

void Exit( ) {
  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_EXIT;
  context_switch(&request);
}

void ExitKernel( ) {
  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_EXIT_KERNEL;
  context_switch(&request);
}

int Send( int tid, void *msg, int msglen, volatile void *reply, int replylen) {
  KASSERT(tid != active_task->tid, "Attempted send to self. from_tid=%d to_tid=%d", active_task->tid, tid);
  KASSERT(tid >= 0, "Attempted to send to a negative tid. from_tid=%d to_tid=%d", active_task->tid, tid);

  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_SEND;

  syscall_message_t arg;
  arg.tid = tid;
  arg.msglen = msglen;
  arg.msg = msg;
  request.arguments = &arg;

  syscall_message_t ret_val;
  ret_val.msglen = replylen;
  ret_val.msg = reply;
  request.ret_val = &ret_val;

  context_switch(&request);
  return ret_val.status;
}

int Receive( int *tid, volatile void *msg, int msglen ) {
  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_RECEIVE;

  syscall_message_t ret_val;
  ret_val.msglen = msglen;
  ret_val.msg = msg;
  request.ret_val = &ret_val;

  context_switch(&request);
  if (tid != NULL) *tid = ret_val.tid;
  return ret_val.status;
}

int Reply( int tid, void *reply, int replylen ) {
  // See send for why this is commented out
  KASSERT(tid != active_task->tid, "Attempted reply to self tid=%d", tid);
  // FIXME: assert tid is valid, replylen is positive or 0

  // Ensure if reply is null, replylen is 0
  assert(reply != NULL || replylen == 0);

  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_REPLY;

  syscall_message_t arg;
  arg.tid = tid;
  arg.msglen = replylen;
  arg.msg = reply;
  request.arguments = &arg;

  context_switch(&request);
  return arg.status;
}

int AwaitEvent( await_event_t event_type ) {
  // FIXME: assert valid event

  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_AWAIT;
  syscall_await_arg_t arg;
  arg.event = event_type;
  request.arguments = &arg;
  context_switch(&request);
  return 0;
}

int AwaitEventPut( await_event_t event_type, char ch) {
  // FIXME: assert valid event

  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_AWAIT;
  syscall_await_arg_t arg;
  arg.event = event_type;
  arg.arg = ch;
  request.arguments = &arg;

  context_switch(&request);
  return 0;
}
