#include <k4_entry.h>
#include <basic.h>
#include <nameserver.h>
#include <idle_task.h>
#include <clock_server.h>
#include <uart_tx_server.h>
#include <uart_rx_server.h>

enum {
  TRAIN_COMMAND,
  TRAIN_TASK_READY,
  TRAIN_GO,
  TRAIN_STOP,
  TRAIN_REVERSE,
};

typedef struct {
  int type;
  int train;
  int command;
} train_control_request_t;

void train_controller_task() {
  int clock_server_tid = WhoIs(CLOCK_SERVER);
  int uart_tx_server_tid = WhoIs(UART_TX_SERVER);
  int server_tid = MyParentTid();

  char buf[2];
  buf[0] = 0;
  buf[1] = 0;

  train_control_request_t notify;
  notify.type = TRAIN_TASK_READY;
  notify.train = MyTid();

  train_control_request_t request;

  while (1) {
    SendS(server_tid, notify, request);
    buf[1] = request.train;
    if (request.command == TRAIN_STOP) {
      Putc(uart_tx_server_tid, COM2, '!');
      Putstr(uart_tx_server_tid, COM2, "STOP!\n\r");
      buf[0] =  0; Putcs(uart_tx_server_tid, COM1, buf, 2);
    } else if (request.command == TRAIN_GO) {
      Putstr(uart_tx_server_tid, COM2, "GO!\n\r");
      buf[0] = 14; Putcs(uart_tx_server_tid, COM1, buf, 2);
    } else if (request.command == TRAIN_REVERSE) {
      Putstr(uart_tx_server_tid, COM2, "REVERSE!\n\r");
      buf[0] = 0; Putcs(uart_tx_server_tid, COM1, buf, 2);
      Delay(clock_server_tid, 300);
      buf[0] = 15; Putcs(uart_tx_server_tid, COM1, buf, 2);
      Delay(clock_server_tid, 5);
      buf[0] = 14; Putcs(uart_tx_server_tid, COM1, buf, 2);
    }
  }
}

void train_controller_server() {
  RegisterAs(TRAIN_CONTROLLER_SERVER);
  int uart_tx_server_tid = WhoIs(UART_TX_SERVER);
  const int NUM_WORKERS = 4;
  int workers[NUM_WORKERS];
  bool workerReady[NUM_WORKERS];

  int receiver;
  train_control_request_t request;

  for (int i = 0; i < NUM_WORKERS; i++) {
    workers[i] = Create(3, &train_controller_task);
    workerReady[i] = false;
  }

  while (1) {
    ReceiveS(&receiver, request);
    if (request.type == TRAIN_TASK_READY) {
      for (int i = 0; i < NUM_WORKERS; i++) {
        if (request.train == workers[i]) {
          workerReady[i] = true;
          break;
        }
      }
    } else if (request.type == TRAIN_COMMAND) {
      for (int i = 0; i < NUM_WORKERS; i++) {
        if (workerReady[i]) {
          ReplyS(workers[i], request);
          workerReady[i] = false;
          break;
        }
      }
      ReplyN(receiver);
    }
  }
}

void print_task() {
  int uart_tx_server_tid = WhoIs(UART_TX_SERVER);
  int uart_rx_server_tid = WhoIs(UART_RX_SERVER);
  int train_controller_server_tid = WhoIs(TRAIN_CONTROLLER_SERVER);

  train_control_request_t request;
  train_control_request_t request2;

  request.type = TRAIN_COMMAND;
  request.train = 70;

  while (1) {
    char c = Getc(uart_rx_server_tid, COM2);
    if (c == 's') {
      request.command = TRAIN_STOP;
      request2.command = TRAIN_STOP;
    } else if (c == 'g') {
      request.command = TRAIN_GO;
      request2.command = TRAIN_GO;
    } else if (c == 'r') {
      request.command = TRAIN_REVERSE;
      request2.command = TRAIN_REVERSE;
    }
  }
}

void k4_entry_task() {
  Create(1, &nameserver);
  Create(2, &clock_server);
  Create(2, &uart_tx_server);
  Create(2, &uart_rx_server);
  Create(IDLE_TASK_PRIORITY, &idle_task);

  Create(10, &train_controller_server);
  Create(11, &print_task);
}
