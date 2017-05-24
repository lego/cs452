#include <basic.h>
#include <kern/context_switch.h>
#include <kern/kernel_request.h>
#include <kernel.h>
#include <nameserver.h>
#include <jstring.h>

int nameserver_tid = -1;

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

int Send( int tid, void *msg, int msglen, void *reply, int replylen) {
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

int Receive( int *tid, void *msg, int msglen ) {
  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_RECEIVE;

  syscall_message_t ret_val;
  ret_val.msglen = msglen;
  ret_val.msg = msg;
  request.ret_val = &ret_val;

  context_switch(&request);
  *tid = ret_val.tid;
  return ret_val.status;
}

int Reply( int tid, void *reply, int replylen ) {
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

int RegisterAs( char *name ) {
  if (jstrcmp(name, "nameserver")) {
    // Don't make data syscall, but still reschedule
    Pass();
    nameserver_tid = active_task->tid;
    return 0;
  }
  if (nameserver_tid == -1){
    // Don't make data syscall, but still reschedule
    Pass();
    return -1;
  }

  nameserver_request_t req;
  req.call_type = REGISTER_CALL;
  req.name = name;

  int status = Send(nameserver_tid, &req, sizeof(nameserver_request_t), NULL, 0);
  // FIXME: proper status check
  return 0;
}

int WhoIs( char *name ) {
  if (nameserver_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    return -1;
  }
  if (jstrcmp(name, "nameserver")) {
    // Don't make data syscalll for data, but still reschedule
    Pass();
    return nameserver_tid;
  }

  nameserver_request_t req;
  req.call_type = WHOIS_CALL;
  req.name = name;

  int recv_tid;
  int bytes_recv = Send(nameserver_tid, &req, sizeof(nameserver_request_t), &recv_tid, sizeof(recv_tid));
  // FIXME: status check on bytes_recv

  return recv_tid;
}
