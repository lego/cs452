#ifndef __KERNEL_REQUEST_H__
#define __KERNEL_REQUEST_H__

#include <kernel.h>

/*
 * kernel_request represents a syscall request to the kernel which should
 * be computed once the task goes back to execution
 */


typedef struct SyscallCreateArg {
  int priority;
  void (*entrypoint)();
} syscall_create_arg_t;

typedef struct SyscallPIDRet {
  volatile int tid;
} syscall_pid_ret_t;


typedef struct SyscallMessage {
  int tid;
  volatile int status;
  volatile int msglen;
  volatile char *msg;
} syscall_message_t;


typedef struct SyscallAwaitEventArg {
  await_event_t event;
  char arg;
} syscall_await_arg_t;


typedef struct {
  int tid;
  int syscall;
  void *arguments;
  void *ret_val;

} kernel_request_t;

#endif
