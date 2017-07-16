#include <kernel.h>
#include <bwio.h>
#include <entries.h>
#include <idle_task.h>
#include <priorities.h>
#include <servers/clock_server.h>
#include <kern/context.h>

void delaying_child_task() {
  int tid = MyTid();
  RecordLogf("Entering delating_child_task tid=%d\n\r", tid);
  Delay(10);
  RecordLogf("Finished delating_child_task tid=%d\n\r", tid);
}

void nested_child_task() {
  // Do some stuff
}

void basic_child_task() {
  int tid = MyTid();
  RecordLogf("Entering basic_child_task tid=%d\n\r", tid);
  Pass();
  RecordLogf("Finishing basic_child_task tid=%d\n\r", tid);
}

void no_reply_child_task() {
  int tid = MyTid();
  RecordLogf("Entering no_reply_child_task tid=%d\n\r", tid);
  int i;
  RecordLogf("Receiving no_reply_child_task tid=%d\n\r", tid);
  Receive(&i, NULL, 0);
  RecordLogf("Destroying self (no_reply_child_task) tid=%d\n\r", tid);
  Destroy(tid);
}

void no_receive_child_task() {
  int tid = MyTid();
  RecordLogf("Entering no_receive_child_task tid=%d\n\r", tid);
  RecordLogf("Destroying self (no_receive_child_task) tid=%d\n\r", tid);
  Destroy(tid);
}

void reply_blocked_child_task() {
  // Do some stuff
}

void send_blocked_child_task() {
  // Do some stuff
}

void lower_priority_entry() {
  int tid;
  int result;
  RecordLogf("Entering lower_priority_entry\n\r");

  RecordLogf("Starting basic_child_task\n\r");
  Create(1, basic_child_task);
  RecordLogf("Starting basic_child_task\n\r");
  Create(1, basic_child_task);
  RecordLogf("Starting basic_child_task\n\r");
  Create(1, basic_child_task);
  RecordLogf("Starting basic_child_task\n\r");
  Create(1, basic_child_task);

  RecordLogf("Starting same priority basic_child_task\n\r");
  tid = Create(5, basic_child_task);
  RecordLogf("Destroying same priority basic_child_task\n\r");
  Destroy(tid);

  RecordLogf("Starting another basic_child_task\n\r");
  tid = Create(5, basic_child_task);
  RecordLogf("Destroying same priority basic_child_task\n\r");
  Destroy(tid);

  RecordLogf("Starting no_reply_child_task\n\r");
  tid = Create(2, no_reply_child_task);
  RecordLogf("Sending message to no_reply_child_task\n\r");
  result = Send(tid, NULL, 0, NULL, 0);
  RecordLogf("message status=%d\n\r", result);

  RecordLogf("Starting no_receive_child_task\n\r");
  tid = Create(6, no_receive_child_task);
  RecordLogf("Sending message to no_receive_child_task\n\r");
  result = Send(tid, NULL, 0, NULL, 0);
  RecordLogf("message status=%d\n\r", result);

  RecordLogf("==Info==\n\r");
  RecordLogf("Used stacks=%d\n\r\n\r", ctx->used_stacks);

  for (int i = 0; i < ctx->used_descriptors; i++) {
    RecordLogf("  Task tid=%d sid=%d name=%s\n\r", i, ctx->descriptors[i].stack_id, ctx->descriptors[i].name);
  }

  RecordLogf("Finished lower_priority_entry\n\r");
  ExitKernel();
}

void destroy_test_task() {
  Create(PRIORITY_CLOCK_SERVER, clock_server);
  RecordLogf("Entering destroy_test_task\n\r");
  Create(5, lower_priority_entry);
  RecordLogf("Finished destroy_test_task\n\r");
}
