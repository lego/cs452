#include <trains/train_controller.h>
#include <basic.h>
#include <bwio.h>
#include <servers/nameserver.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <train_command_server.h>
#include <trains/sensor_collector.h>
#include <trains/navigation.h>
#include <priorities.h>

int train_controllers[TRAINS_MAX] = {-1};

static void execute_basic_command(int train, train_command_msg_t * msg) {
  switch (msg->type) {
  case TRAIN_CONTROLLER_SET_SPEED:
    SetTrainSpeed(train, msg->speed);
    break;
  }
}

static void start_navigation(path_t * path) {
  // TODO: navigation stuff
}

void train_controller() {
  int requester;
  char request_buffer[1024] __attribute__ ((aligned (4)));
  packet_t * packet = (packet_t *) request_buffer;
  train_command_msg_t * msg = (train_command_msg_t *) request_buffer;
  sensor_data_t * sensor_data = (sensor_data_t *) request_buffer;
  train_navigate_t * navigate_data = (train_navigate_t *) request_buffer;

  path_t navigation_data;

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
        execute_basic_command(train, msg);
        break;
      case TRAIN_NAVIGATE_COMMAND:
        navigation_data = navigate_data->path; // Persist the path
        start_navigation(&navigation_data);
        break;
    }
  }
}

int CreateTrainController(int train) {
  int tid = Create(PRIORITY_TRAIN_CONTROLLER, train_controller);
  train_controllers[train] = tid;
  SendSN(tid, train);
  return tid;
}

static void ensure_train_controller(int train) {
  if (train_controllers[train] == -1) {
    train_controllers[train] = CreateTrainController(train);
  }
}

void TellTrainController(int train, int type, int speed) {
  ensure_train_controller(train);
  train_command_msg_t msg;
  msg.packet.type = TRAIN_CONTROLLER_COMMAND;
  msg.type = type;
  msg.speed = speed;
  SendSN(train_controllers[train], msg);
}
