#include <trains/train_controller.h>
#include <basic.h>
#include <bwio.h>
#include <servers/nameserver.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <train_command_server.h>
#include <trains/sensor_collector.h>
#include <priorities.h>

void train_controller() {
  int requester;
  train_controller_msg_t msg;

  int train;
  ReceiveS(&requester, train);
  ReplyN(requester);

  RegisterTrain(train);

  while (true) {
    ReceiveS(&requester, msg);
    ReplyN(requester);
    switch (msg.type) {
      case TRAIN_CONTROLLER_SENSOR:
        break;
      case TRAIN_CONTROLLER_COMMAND:
        switch (msg.command.type) {
          case TRAIN_CONTROLLER_SET_SPEED:
            SetTrainSpeed(train, msg.command.speed);
            break;
        }
        break;
    }
  }
}

int CreateTrainController(int train) {
  int task = Create(PRIORITY_TRAIN_CONTROLLER, train_controller);
  SendSN(task, train);
  return task;
}
