#include <train_command_server.h>
#include <basic.h>
#include <kernel.h>
#include <bwio.h>
#include <servers/nameserver.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <priorities.h>

static int train_command_server_tid = -1;

enum {
  TRAIN_COMMAND,
  TRAIN_SET_SPEED,
  TRAIN_REVERSE,
  TRAIN_INSTANT_STOP
};

typedef struct {
  int index;
  int value;
  int command;
} train_command_request_t;

void train_command_task() {
  char buf[2];
  buf[0] = 0;
  buf[1] = 0;

  train_command_request_t request;

  int receiver = -1;

  while (1) {
    ReceiveS(&receiver, request);
    ReplyN(receiver);
    buf[1] = request.index;
    if (request.command == TRAIN_SET_SPEED) {
      buf[0] = request.value; Putcs(COM1, buf, 2);
    } else if (request.command == TRAIN_REVERSE) {
      buf[0] = 0; Putcs(COM1, buf, 2);
      Delay(300);
      buf[0] = 15; Putcs(COM1, buf, 2);
      Delay(10);
      buf[0] = request.value; Putcs(COM1, buf, 2);
    } else if (request.command == TRAIN_INSTANT_STOP) {
      buf[0] = 15; Putcs(COM1, buf, 2);
      Delay(10);
      buf[0] = 15; Putcs(COM1, buf, 2);
    }
  }
}

void train_command_server() {
  train_command_server_tid = MyTid();
  RegisterAs(NS_TRAIN_CONTROLLER_SERVER);

  int receiver;
  train_command_request_t request;

  while (1) {
    ReceiveS(&receiver, request);
    ReplyN(receiver);
    int tid = Create(PRIORITY_TRAIN_COMMAND_TASK, train_command_task);
    SendSN(tid, request);
  }
}

int SetTrainSpeed(int train, int speed) {
  log_task("SetTrainSpeed train=%d speed=%d", active_task->tid, train, speed);
  if (train_command_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "Train Command server not initialized");
    return -1;
  }
  train_command_request_t request;
  request.index = train;
  request.value = speed;
  request.command = TRAIN_SET_SPEED;
  SendSN(train_command_server_tid, request);
  return 0;
}

int MoveTrain(int train, int speed, int node_id) {
  log_task("MoveTrain train=%d speed=%d node_id=%d", active_task->tid, train, speed, node_id);
  if (train_command_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    KASSERT(false, "Train Command server not initialized");
    return -1;
  }
  return 0;
}

int ReverseTrain(int train, int currentSpeed) {
  log_task("ReverseTrain train=%d currentSpeed=%d", active_task->tid, train, currentSpeed);
  if (train_command_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "Train Command server not initialized");
    return -1;
  }
  train_command_request_t request;
  request.index = train;
  request.value = currentSpeed;
  request.command = TRAIN_REVERSE;
  SendSN(train_command_server_tid, request);
  return 0;
}

int InstantStop(int train) {
  log_task("InstantStop train=%d", active_task->tid, train);
  if (train_command_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "Train Command server not initialized");
    return -1;
  }
  train_command_request_t request;
  request.index = train;
  request.command = TRAIN_INSTANT_STOP;
  SendSN(train_command_server_tid, request);
  return 0;
}
