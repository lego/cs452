#include <basic.h>
#include <bwio.h>
#include <k3_entry.h>
#include <kernel.h>
#include <nameserver.h>
#include <clock_server.h>
#include <idle_task.h>


typedef struct {
  int delay_ticks;
  int delay_amount;
} client_data_t;

void clock_server_test_child() {
  int tid = MyTid();
  log_task("In clock_server_test_child", tid);
  volatile client_data_t data;
  int parent_tid = MyParentTid();

  Send(parent_tid, NULL, 0, &data, sizeof(data));
  int ticks = data.delay_ticks;
  int amount = data.delay_amount;

  int clock_server_tid = WhoIs(CLOCK_SERVER);
  int i;

  log_task("Initialized", tid);

  for (i = 0; i < amount; i++) {
    log_task("Delaying ticks=%d", tid, ticks);
    Delay(clock_server_tid, ticks);
    bwprintf(COM2, "tid=%d delay=%d completed_delays=%d\n\r", tid, ticks, i+1);
  }
  ExitKernel();
}

void clock_server_test() {
  int tid = MyTid();
  log_task("In clock_server_test", tid);

  Create(1, &nameserver);
  Create(2, &clock_server);
  Create(IDLE_TASK_PRIORITY, &idle_task);

  client_data_t data;
  int recv_tid;

  log_task("Initialize child task", tid);
  Create(3, &clock_server_test_child);
  data.delay_ticks = 10;
  data.delay_amount = 20;
  log_task("Send child task data", tid);
  ReceiveN(&recv_tid);
  ReplyS(recv_tid, data);
  log_task("Task ended", tid);
}
