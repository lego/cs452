#include <basic.h>
#include <kern/context_switch.h>
#include <kern/kernel_request.h>
#include <kernel.h>

int Create(int priority, void (*entrypoint)()) {
  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_CREATE;

  syscall_create_arg_t arg;
  arg.priority = priority;
  arg.entrypoint = entrypoint;

  request.arguments = &arg;

  // NOTE: this is kinda weird, the return value is set in userspace
  // by the kernel
  syscall_pid_ret_t ret_val;
  request.ret_val = &ret_val;
  context_switch(&request);
  return ret_val.tid;
}

int MyTid( ) {
  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_MY_TID;

  // NOTE: this is kinda weird, the return value is set in userspace
  // by the kernel
  syscall_pid_ret_t ret_val;
  request.ret_val = &ret_val;
  context_switch(&request);
  return ret_val.tid;
}

int MyParentTid( ) {
  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_MY_PARENT_TID;

  // NOTE: this is kinda weird, the return value is set in userspace
  // by the kernel
  syscall_pid_ret_t ret_val;
  request.ret_val = &ret_val;
  context_switch(&request);
  return ret_val.tid;
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
