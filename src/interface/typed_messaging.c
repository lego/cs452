#include <kernel.h>
#include <kern/kernel_request.h>

int _SendTyped(int dest, int type, void * data, int data_len) {
  KASSERT(tid != active_task->tid, "Attempted send to self. from_tid=%d to_tid=%d", active_task->tid, tid);
  KASSERT(tid >= 0, "Attempted to send to a negative tid. from_tid=%d to_tid=%d", active_task->tid, tid);

  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_SEND_TYPED;

  syscall_typed_message_t arg;
  arg.tid = tid;
  arg.type = type;
  arg.data = data;
  arg.data_len = data_len;
  request.arguments = &arg;

  syscall_message_t ret_val;
  ret_val.msglen = 0;
  ret_val.msg = NULL;
  request.ret_val = &ret_val;

  context_switch(&request);
  return ret_val.status;
}

int RecvTyped(int * sender, packet_t * buffer, int buffer_size) {
  kernel_request_t request;
  request.tid = active_task->tid;
  request.syscall = SYSCALL_RECV_TYPED;

  syscall_typed_message_t arg;
  arg.tid = tid;
  arg.data = buffer;
  arg.data_len = buffer_size;
  request.arguments = &arg;

  syscall_message_t ret_val;
  ret_val.msglen = 0;
  ret_val.msg = NULL;
  request.ret_val = &ret_val;

  context_switch(&request);
  return ret_val.status;
}
