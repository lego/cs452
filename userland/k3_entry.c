#include <basic.h>
#include <bwio.h>
#include <k3_entry.h>
#include <kernel.h>
#include <nameserver.h>
#include <clock_server.h>
#include <idle_task.h>
#include <io.h>

typedef struct {
  int delay_ticks;
  int delay_amount;
} client_data_t;

volatile int programs_active;
volatile io_time_t start_time;

static inline void Prepare() {
  programs_active++;
}


static inline void ExitIfComplete() {
  programs_active--;
  if (programs_active == 0) {
    bwprintf(COM2, "finishing program total_time=%dms", io_time_difference_ms(io_get_time(), start_time));
    ExitKernel();
  }
}

void k3_client_task() {
  Prepare();
  volatile client_data_t data;
  int my_tid = MyTid();
  int parent_tid = MyParentTid();

  Send(parent_tid, NULL, 0, &data, sizeof(data));
  int ticks = data.delay_ticks;
  int amount = data.delay_amount;

  int i;
  for (i = 0; i < amount; i++) {
    Delay(ticks);
    bwprintf(COM2, "Finished delay tid=%d ticks=%d completed=%d\n\r", my_tid, ticks, i+1);
  }

  ExitIfComplete();
}

void k3_entry_task() {
  start_time = io_get_time();

  Create(1, &nameserver);
  Create(2, &clock_server);
  Create(IDLE_TASK_PRIORITY, &idle_task);

  client_data_t data;
  int recv_tid;
  programs_active = 0;

  // Client
  // priority = 3
  // delay_ticks = 10
  // delay_amount = 20
  Create(3, &k3_client_task);
  data.delay_ticks = 10;
  data.delay_amount = 20;
  ReceiveN(&recv_tid);
  ReplyS(recv_tid, data);

  // Client
  // priority = 4
  // delay_ticks = 23
  // delay_amount = 9
  Create(4, &k3_client_task);
  data.delay_ticks = 23;
  data.delay_amount = 9;
  ReceiveN(&recv_tid);
  ReplyS(recv_tid, data);

  // Client
  // priority = 5
  // delay_ticks = 33
  // delay_amount = 6
  Create(5, &k3_client_task);
  data.delay_ticks = 33;
  data.delay_amount = 6;
  ReceiveN(&recv_tid);
  ReplyS(recv_tid, data);

  // Client
  // priority = 6
  // delay_ticks = 71
  // delay_amount = 3
  Create(6, &k3_client_task);
  data.delay_ticks = 71;
  data.delay_amount = 3;
  ReceiveN(&recv_tid);
  ReplyS(recv_tid, data);
}

// origin/terrance/hardware-interrupts
// kept just because, this testing base might be useful
//
// #include <basic.h>
// #include <bwio.h>
// #include <k2_entry.h>
// #include <nameserver.h>
// #include <kernel.h>
// #include <ts7200.h>
//
// void k3_entry_task() {
//   bwprintf(COM2, "k3_entry\n\r");
//   // First things first, make the nameserver
//   Create(1, &nameserver);
//
//   volatile int volatile *flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
//   volatile int volatile *data = (int *)( UART2_BASE + UART_DATA_OFFSET );
//
//   while (!( *flags & RXFF_MASK ));
//   char x = *data;
// }
