#include <kernel.h>
#include <worker.h>

typedef struct {
  void (*worker_func)(int, void *);
  int len;
} worker_init_t;

void worker_task() {
  int tid = MyTid();
  int parent_tid = MyParentTid();
  int sender;

  worker_init_t init_data;
  ReceiveS(&sender, init_data);
  ReplyN(sender);

  char worker_data[init_data.len];
  Receive(&sender, worker_data, init_data.len);
  ReplyN(sender);

  init_data.worker_func(parent_tid, (void *) worker_data);

  // Destroy self, in the event that worker_func creates extra tasks
  // and doesn't already call destroy
  Destroy(tid);
}

int _CreateWorker(int priority, void (*worker_func)(int, void *), void * data, int data_len) {
  int worker_tid = Create(priority, worker_task);
  worker_init_t init_data;
  SendSN(worker_tid, init_data);
  Send(worker_tid, data, data_len, NULL, 0);
  return worker_tid;
}
