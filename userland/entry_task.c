#include <bwio.h>
#include <entry_task.h>
#include <nameserver.h>
#include <kernel.h>
#include <ts7200.h>
#include <debug.h>

void child_task() {
  int from_tid;
  char buf[100];
  int result;
  char *bye = "BYE";
  bwprintf(COM2, "T1 Child task started.\n\r");
  bwprintf(COM2, "T1 Receiving message.\n\r");
  result = Receive(&from_tid, buf, 100);
  bwprintf(COM2, "T1 Received message. result=%d source_tid=%d msg=%s\n\r", result, from_tid, buf);

  bwprintf(COM2, "T1 Getting parent tid.\n\r");
  int parent_tid = MyParentTid();

  debugger();

  bwprintf(COM2, "T1 Sending message to tid=%d msg=%s\n\r", parent_tid, bye);
  result = Send(parent_tid, bye, 4, NULL, 0);
  result = Receive(&from_tid, buf, 100);

  bwprintf(COM2, "T1 Child task exiting\n\r");
}

void entry_task() {
  int new_task_id;
  int from_tid;
  int result;
  char buf[100];
  char *hello = "HELLO";

  bwprintf(COM2, "T0 Task started\n\r");
  new_task_id = Create(PRIORITY_LOW, &child_task);
  bwprintf(COM2, "T0 Created tid=%d\n\r", new_task_id);

  bwprintf(COM2, "T0 Sending message. tid=%d msg=%s\n\r", new_task_id, hello);
  result = Send(new_task_id, hello, 6, NULL, 0);
  bwprintf(COM2, "T0 Sent message. result=%d to_tid=%d\n\r", result, new_task_id);

  bwprintf(COM2, "T0 Going to receive message.\n\r");
  result = Receive(&from_tid, buf, 100);
  bwprintf(COM2, "T0 Received message. result=%d source_tid=%d msg=%s\n\r", result, from_tid, buf);

  bwprintf(COM2, "T0 Entry task exiting\n\r");
  Exit();
}

// void entry_task() {
//   Create(PRIORITY_HIGHEST, &nameserver);
// }
