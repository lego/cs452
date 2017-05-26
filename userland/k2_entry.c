#include <basic.h>
#include <bwio.h>
#include <k2_entry.h>
#include <nameserver.h>
#include <kernel.h>
#include <ts7200.h>
#include <io.h>

void k2_child_task() {
  int from_tid;
  char buf[100];
  int result;
  char *bye = "BYE";
  log_debug(COM2, "T1 Child task started.\n\r");
  log_debug(COM2, "T1 Receiving message.\n\r");
  result = Receive(&from_tid, buf, 100);
  log_debug(COM2, "T1 Received message. result=%d source_tid=%d msg=%s\n\r", result, from_tid, buf);

  Reply(from_tid, NULL, 0);
  Exit();
  log_debug(COM2, "T1 Getting parent tid.\n\r");
  int parent_tid = MyParentTid();

  log_debug(COM2, "T1 Sending message to tid=%d msg=%s\n\r", parent_tid, bye);
  result = Send(parent_tid, bye, 4, NULL, 0);

  log_debug(COM2, "T1 Child task exiting\n\r");
  Exit();
}

void k2_entry_task() {
  int new_task_id;
  int from_tid;
  int result;
  char buf[100];
  char *hello = "HELLO";

  log_debug(COM2, "T0 Task started\n\r");
  new_task_id = Create(2, &k2_child_task);
  log_debug(COM2, "T0 Created tid=%d\n\r", new_task_id);

  log_debug(COM2, "T0 Sending message. tid=%d msg=%s\n\r", new_task_id, hello);

  io_time_t t1 = io_get_time();
  result = Send(new_task_id, hello, 6, NULL, 0);
  io_time_t t2 = io_get_time();
  bwprintf(COM2, "T0 Sent message. result=%d to_tid=%d time=%dus t1=%d t2=%d\n\r", result, new_task_id, io_time_difference_us(t2, t1), t1, t2);
  Exit();
  log_debug(COM2, "T0 Going to receive message.\n\r");
  result = Receive(&from_tid, buf, 100);
  log_debug(COM2, "T0 Received message. result=%d source_tid=%d msg=%s\n\r", result, from_tid, buf);
  Reply(from_tid, NULL, 0);

  log_debug(COM2, "T0 Entry task exiting\n\r");
  Exit();
}
