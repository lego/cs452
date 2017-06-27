#include <train_controller.h>
#include <basic.h>
#include <bwio.h>
#include <nameserver.h>
#include <clock_server.h>
#include <uart_tx_server.h>
#include <priorities.h>

static int train_controller_server_tid = -1;

enum {
  TRAIN_COMMAND,
  TRAIN_TASK_READY,
  TRAIN_SET_SPEED,
  TRAIN_REVERSE,
  TRAIN_INSTANT_STOP,
  SWITCH_SET,
};

typedef struct {
  int type;
  int index;
  int value;
  int command;
} train_control_request_t;

void train_controller_task() {
  int server_tid = MyParentTid();

  char buf[2];
  buf[0] = 0;
  buf[1] = 0;

  train_control_request_t notify;
  notify.type = TRAIN_TASK_READY;

  train_control_request_t request;

  while (1) {
    SendS(server_tid, notify, request);
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
    } else if (request.command == SWITCH_SET) {
      if (request.value == SWITCH_CURVED) {
        buf[0] = 34; Putcs(COM1, buf, 2);
      } else if (request.value == SWITCH_STRAIGHT) {
        buf[0] = 33; Putcs(COM1, buf, 2);
      }
      Delay(20);
      Putc(COM1, 32);
    }
  }
}

void train_controller_server() {
  train_controller_server_tid = MyTid();
  RegisterAs(TRAIN_CONTROLLER_SERVER);
  const int NUM_WORKERS = 4;
  int workers[NUM_WORKERS];
  bool workerReady[NUM_WORKERS];

  int receiver;
  train_control_request_t request;

  for (int i = 0; i < NUM_WORKERS; i++) {
    workers[i] = Create(PRIORITY_TRAIN_CONTROLLER_TASK, &train_controller_task);
    workerReady[i] = false;
  }

  while (1) {
    ReceiveS(&receiver, request);
    if (request.type == TRAIN_TASK_READY) {
      for (int i = 0; i < NUM_WORKERS; i++) {
        if (receiver == workers[i]) {
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

int SetTrainSpeed(int train, int speed) {
  log_task("SetTrainSpeed train=%d speed=%d", active_task->tid, train, speed);
  if (train_controller_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "Train Controller server not initialized");
    return -1;
  }
  train_control_request_t request;
  request.type = TRAIN_COMMAND;
  request.index = train;
  request.value = speed;
  request.command = TRAIN_SET_SPEED;
  SendSN(train_controller_server_tid, request);
  return 0;
}

int MoveTrain(int train, int speed, int node_id) {
  log_task("MoveTrain train=%d speed=%d node_id=%d", active_task->tid, train, speed, node_id);
  if (train_controller_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    KASSERT(false, "Train Controller server not initialized");
    return -1;
  }
  return 0;
}

int ReverseTrain(int train, int currentSpeed) {
  log_task("ReverseTrain train=%d currentSpeed=%d", active_task->tid, train, currentSpeed);
  if (train_controller_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "Train Controller server not initialized");
    return -1;
  }
  train_control_request_t request;
  request.type = TRAIN_COMMAND;
  request.index = train;
  request.value = currentSpeed;
  request.command = TRAIN_REVERSE;
  SendSN(train_controller_server_tid, request);
  return 0;
}

int InstantStop(int train) {
  log_task("InstantStop train=%d", active_task->tid, train);
  if (train_controller_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "Train Controller server not initialized");
    return -1;
  }
  train_control_request_t request;
  request.type = TRAIN_COMMAND;
  request.index = train;
  request.command = TRAIN_INSTANT_STOP;
  SendSN(train_controller_server_tid, request);
  return 0;
}

int SetSwitch(int sw, int state) {
  log_task("SetSwitch switch=%d state=%d", active_task->tid, sw, state);
  if (train_controller_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "Train Controller server not initialized");
    return -1;
  }
  train_control_request_t request;
  request.type = TRAIN_COMMAND;
  request.index = sw;
  request.value = state;
  request.command = SWITCH_SET;
  SendSN(train_controller_server_tid, request);
  return 0;
}
