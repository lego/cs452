#include <basic.h>

#include <bwio.h>
#include <servers/nameserver.h>
#include <kernel.h>
#include <ts7200.h>
#include <io.h>

#define TIMING_START(val) n = val; t2 = 0; for (i = 0; i < n; i++) { t1 = io_get_time();
#define TIMING_LOG(msg) bwprintf(COM2, msg " cumtime=%dus ncalls=%d percall=%dus\n\r", io_time_difference_us(t2, 0), n, io_time_difference_us(t2, 0) / n)
#define TIMING_END(msg) t2 += io_get_time() - t1; } TIMING_LOG(msg)

void msg_child_task() {
  int from_tid;
  char buf[64];
  while (true) {
    Receive(&from_tid, buf, 64);
    Reply(from_tid, NULL, 0);
  }
  Exit();
}

void empty_task() {
  Exit();
}

void timing_start_task() {
  io_time_t t = io_get_time();
  Send(MyParentTid(), &t, sizeof(io_time_t), NULL, 0);
  Exit();
}

void hello_recv_child_task() {
  int from_tid;
  char buf[6];
  int result;
  log_task("Going to receive", 1);
  result = Receive(&from_tid, buf, 6);
  log_task("Received result=%d", 1, result);
  bwprintf(COM2, "T1 Received message. result=%d source_tid=%d msg=%s\n\r", result, from_tid, buf);
  log_task("Going to reply reply_to_tid=%d", 1, from_tid);
  Reply(from_tid, NULL, 0);
  log_task("Replied, now exiting", 1);
  Exit();
}

void benchmark_entry_task() {
  int from_task;
  int new_task_id;
  char *msg_4 = "HI!";
  char *msg_64 = "HELLO HOW ARE YOU TODAY LEZ GO MY FRIEND BROSKI WOWZA ME TOO HE";
  int i, n;
  io_time_t t1;
  io_time_t t2;

  bwprintf(COM2, "=== WARMING CACHE ===\n\r");

  // SRR
  new_task_id = Create(2, &msg_child_task);
  Send(new_task_id, msg_64, 64, NULL, 0);

  // RSR
  new_task_id = Create(0, &msg_child_task);
  Send(new_task_id, msg_4, 4, NULL, 0);

  // Regular pass
  Pass();

  bwprintf(COM2, "=== BENCHMARKS ===\n\r");

  new_task_id = Create(2, &msg_child_task);
  TIMING_START(100);
  Send(new_task_id, msg_4, 4, NULL, 0);
  TIMING_END("4 byte message SRR");

  new_task_id = Create(2, &msg_child_task);
  TIMING_START(100);
  Send(new_task_id, msg_64, 64, NULL, 0);
  TIMING_END("64 byte message SRR");

  new_task_id = Create(0, &msg_child_task);
  TIMING_START(100);
  Send(new_task_id, msg_4, 4, NULL, 0);
  TIMING_END("4 byte message RSR");

  new_task_id = Create(0, &msg_child_task);
  TIMING_START(100);
  Send(new_task_id, msg_64, 64, NULL, 0);
  TIMING_END("64 byte message RSR");

  TIMING_START(1);
  Create(0, &empty_task);
  TIMING_END("Task start, enter, exit");

  t1 = io_get_time();
  new_task_id = Create(0, &timing_start_task);
  Receive(&from_task, &t2, sizeof(io_time_t));
  Reply(from_task, NULL, 0);
  bwprintf(COM2, "Task start and enter time=%dus\n\r", io_time_difference_us(t2, t1));

  TIMING_START(100);
  Pass();
  TIMING_END("Task reschedule");

  // Sanity check SRR, prints result value
  bwprintf(COM2, "=== SANITY CHECK ===\n\r");

  int result;
  char *hello = "HELLO";
  log_task("Going to create hello_recv", 0);
  new_task_id = Create(2, &hello_recv_child_task);
  log_task("Done creating hello_recv", 0);
  bwprintf(COM2, "T0 Sending message. tid=%d msg=%s\n\r", new_task_id, hello);
  result = Send(new_task_id, hello, 6, NULL, 0);
  log_task("Sent msg", 0);
  bwprintf(COM2, "T0 Sent message. result=%d to_tid=%d\n\r", result, new_task_id);
<<<<<<< HEAD:userland/entry/benchmark.c
  ExitKernel();
||||||| merged common ancestors
  log_task("Exiting", 0);
  Exit();
=======
>>>>>>> Run cache and non-cache everything:userland/benchmark_entry.c
}
