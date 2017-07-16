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
  char request_buffer[1024] __attribute__ ((aligned (4)));
  packet_t * packet = (packet_t *) request_buffer;
  train_command_msg_t * msg = (train_command_msg_t *) request_buffer;
  sensor_data_t * sensor_data = (sensor_data_t *) request_buffer;

  int train;
  ReceiveS(&requester, train);
  ReplyN(requester);

  RegisterTrain(train);

  while (true) {
    ReceiveS(&requester, request_buffer);
    ReplyN(requester);
    switch (packet->type) {
      case SENSOR_DATA:
        break;
      case TRAIN_CONTROLLER_COMMAND:
        switch (msg->type) {
          case TRAIN_CONTROLLER_SET_SPEED:
            SetTrainSpeed(train, msg->speed);
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
