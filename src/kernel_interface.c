#include <stddef.h>
#include <kassert.h>

#include <bwio.h>
#include <kern/context.h>
#include <kern/context_switch.h>
#include <kern/kernel_request.h>
#include <kernel.h>
#include <servers/nameserver.h>
#include <jstring.h>

int CreateWithName(int priority, void (*entrypoint)(), const char *func_name) {
  KASSERT(0 <= priority && priority < 32, "Invalid priority provided.");

  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_CREATE;

  syscall_create_arg_t arg;
  arg.priority = priority;
  arg.entrypoint = entrypoint;
  arg.func_name = func_name;

  request.arguments = &arg;

  // NOTE: this is kinda weird, the return value is set in userspace by the kernel
  syscall_pid_ret_t ret_val;
  request.ret_val = &ret_val;
  context_switch(&request);
  return ret_val.tid;
}

int MyTid( ) {
  return active_task->tid;
}

const char * MyTaskName( ) {
  return active_task->name;
}

int MyParentTid( ) {
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
  KASSERT(reply != NULL || replylen == 0, "Must use size == 0 if sending NULL. got len=%d", replylen);

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

io_time_t GetIdleTaskExecutionTime() {
  int i;
  for (i = 0; i < ctx->used_descriptors; i++) {
    if (ctx->descriptors[i].priority == 31) {
      return ctx->descriptors[i].execution_time;
    }
  }
  KASSERT(false, "Could not find idle task");
  return 0;
}

void RecordLog(const char * msg) {
  int len = jstrlen(msg);
  if (!(len < LOG_SIZE - log_length)) {
    log_length = 0;
  }
  int i;
  int start = log_length;
  for (i = 0; i < len; i++) {
    logs[start + i] = msg[i];
  }
  // FIXME: chance this gets interrupted. if this happens, we want to do this
  // in the kernel and not userland
  log_length += len;
}

void RecordLogi(int i) {
  char bf[12];
  ji2a(i, bf);
  RecordLog(bf);
}

void RecordLogf(char *fmt, ...) {
  char buf[2048];
  va_list va;
  va_start(va,fmt);
  jformat(buf, 2048, fmt, va);
  va_end(va);
  return RecordLog(buf);
}

void *Malloc( unsigned int size ) {
  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_MALLOC;
  request.arguments = (void *) size;
  void *data = NULL;
  request.ret_val = &data;
  context_switch(&request);
  return data;
}

int Free( void * ptr ) {
  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_FREE;
  request.arguments = ptr;
  int ret_val = 0;
  request.ret_val = &ret_val;
  context_switch(&request);
  return ret_val;
}
