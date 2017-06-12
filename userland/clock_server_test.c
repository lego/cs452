#include <basic.h>
#include <bwio.h>
#include <k3_entry.h>
#include <kernel.h>
#include <nameserver.h>
#include <clock_server.h>
#include <idle_task.h>
#include <io.h>
#include <priorities.h>


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

  int i;

  bwprintf(COM2, "Initialized ticks=%d amount=%d\n\r", ticks, amount);

  io_time_t start_time = io_get_time();
  io_time_t curr_time = start_time;

  for (i = 0; i < amount; i++) {
    log_task("Delaying ticks=%d", tid, ticks);
    Delay(ticks);
    io_time_t last_time = curr_time;
    curr_time = io_get_time();
    bwprintf(COM2, "tid=%d delay=%d completed_delays=%d ticktime=%dms cumtime=%dms\n\r", tid, ticks, i+1,  io_time_difference_ms(curr_time, last_time), io_time_difference_ms(curr_time, start_time));
  }

  int time_ticks = Time();

  io_time_t end_time = io_get_time();

  bwprintf(COM2, "finished. standard_time=%dms tick_time=%d\n\r", io_time_difference_ms(end_time, start_time), time_ticks);

  ExitKernel();
}

void clock_server_test() {
  int tid = MyTid();
  log_task("In clock_server_test", tid);

  Create(1, &nameserver);
  Create(2, &clock_server);
  Create(PRIORITY_IDLE_TASK, &idle_task);

  client_data_t data;
  int recv_tid;

  log_task("Initialize child task", tid);
  Create(3, &clock_server_test_child);
  data.delay_ticks = 10;
  data.delay_amount = 10;
  log_task("Send child task data", tid);
  ReceiveN(&recv_tid);
  ReplyS(recv_tid, data);
  log_task("Task ended", tid);
}
