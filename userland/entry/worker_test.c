#include <worker.h>
#include <bwio.h>
#include <kernel.h>
#include <servers/nameserver.h>

void worker_code(int parent, void * data) {
  bwprintf(COM2, "  worker started, and sending\n\r");
  Send(parent, NULL, 0, NULL, 0);
  bwprintf(COM2, "  worker send finished\n\r");
  bwprintf(COM2, "  WORKER TASK DONE\n\r");
}

void worker_test_task() {
  int sender;

  Create(0, nameserver);
  bwprintf(COM2, "===Worker test===\n\r");


  bwprintf(COM2, "  creating worker\n\r");
  int worker_tid = _CreateWorker(2, worker_code, NULL, 0);
  bwprintf(COM2, "  receiving from worker\n\r");
  ReceiveN(&sender);
  bwprintf(COM2, "  replying to tid=%d worker_tid=%d\n\r", sender, worker_tid);
  ReplyN(sender);
  bwprintf(COM2, "  MAIN TASK DONE\n\r");
  ExitKernel();
}
