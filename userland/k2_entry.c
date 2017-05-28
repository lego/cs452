#include <basic.h>
#include <bwio.h>
#include <k2_entry.h>
#include <nameserver.h>
#include <kernel.h>
#include <ts7200.h>

void _done_send() {
  Send(0, NULL, 0, NULL, 0);
}

void done() {
  Create(31, &_done_send);
}

void waitUntilDone(void* code) {
  bwprintf(COM2, "---\n\r\n\r\n\r");
  Create(0, code);
  int tid; Receive(&tid, NULL, 0); Reply(tid, NULL, 0);
  bwprintf(COM2, "\n\r\n\r---\n\r");
}

void producer() {
  bwprintf(COM2, "P: Registering as PRODUCER_TEST with tid: %d\n\r", MyTid());
  RegisterAs(PRODUCER_TEST);
  bwprintf(COM2, "P: Exit RegisterAs\n\r");

  int source_tid = -1;

  while (true) {
    bwprintf(COM2, "P: Waiting to receive...\n\r");
    int status = Receive(&source_tid, NULL, 0);
    bwprintf(COM2, "P: Received request from %d (status: %d)\n\r", source_tid, status);
    int x = 5;
    Reply(source_tid, &x, sizeof(int));
  }
}

void consumer() {
  bwprintf(COM2, "C: Requesting PRODUCER_TEST tid\n\r");
  int producer_tid = WhoIs(PRODUCER_TEST);
  bwprintf(COM2, "C: Received %d for PRODUCER_TEST\n\r", producer_tid);
  int i;
  for (i = 0; i < 3; i++) {
    int x;
    bwprintf(COM2, "C: Requesting from producer\n\r");
    int status = Send(producer_tid, NULL, 0, &x, sizeof(x));
    bwprintf(COM2, "C: Received %d from producer (status %d)\n\r", x, status);
  }
}

void nameserver_test() {
  Create(3, &consumer);
  Create(2, &producer);

  done();
}

void k2_child_task() {
  int from_tid;
  char buf[100];
  int result;
  char *bye = "BYE";
  bwprintf(COM2, "T1 Child task started.\n\r");
  bwprintf(COM2, "T1 Receiving message.\n\r");
  result = Receive(&from_tid, buf, 100);
  bwprintf(COM2, "T1 Received message. result=%d source_tid=%d msg=%s\n\r", result, from_tid, buf);

  Reply(from_tid, NULL, 0);

  bwprintf(COM2, "T1 Getting parent tid.\n\r");
  int parent_tid = MyParentTid();

  bwprintf(COM2, "T1 Sending message to tid=%d msg=%s\n\r", parent_tid, bye);
  result = Send(parent_tid, bye, 4, NULL, 0);

  bwprintf(COM2, "T1 Child task exiting\n\r");
}

void send_receive_test() {
  int new_task_id;
  int from_tid;
  int result;
  char buf[100];
  char *hello = "HELLO";

  bwprintf(COM2, "T0 Task started\n\r");
  new_task_id = Create(2, &k2_child_task);
  bwprintf(COM2, "T0 Created tid=%d\n\r", new_task_id);

  bwprintf(COM2, "T0 Sending message. tid=%d msg=%s\n\r", new_task_id, hello);
  result = Send(new_task_id, hello, 6, NULL, 0);
  bwprintf(COM2, "T0 Sent message. result=%d to_tid=%d\n\r", result, new_task_id);

  bwprintf(COM2, "T0 Going to receive message.\n\r");
  result = Receive(&from_tid, buf, 100);
  bwprintf(COM2, "T0 Received message. result=%d source_tid=%d msg=%s\n\r", result, from_tid, buf);
  Reply(from_tid, NULL, 0);

  bwprintf(COM2, "T0 Entry task exiting\n\r");

  done();
}

void k2_entry_task() {
  bwprintf(COM2, "k2_entry\n\r");
  // First things first, make the nameserver
  Create(1, &nameserver);

  waitUntilDone(&send_receive_test);
  waitUntilDone(&nameserver_test);

  //Exit();
}
