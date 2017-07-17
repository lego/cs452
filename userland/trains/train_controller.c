#include <trains/train_controller.h>
#include <basic.h>
#include <bwio.h>
#include <kernel.h>
#include <servers/nameserver.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <train_command_server.h>
#include <trains/sensor_collector.h>
#include <trains/route_executor.h>
#include <trains/navigation.h>
#include <priorities.h>

int train_controllers[TRAINS_MAX];

void InitTrainControllers() {
  for (int i = 0; i < TRAINS_MAX; i++) {
    train_controllers[i] = -1;
  }
}

typedef struct {
  int train;
  int lastSpeed;
} reverse_train_t;

void reverse_train_task() {
    reverse_train_t rev;
    int receiver;
    ReceiveS(&receiver, rev);
    ReplyN(receiver);
    char buf[2];
    buf[0] = 0;
    buf[1] = rev.train;
    Putcs(COM1, buf, 2);
    Delay(300);
    buf[0] = 15;
    Putcs(COM1, buf, 2);
    int lastSensor = WhereAmI(rev.train);
    if (lastSensor >= 0) {
      RegisterTrainReverse(rev.train, lastSensor);
    }
    Delay(10);
    buf[0] = rev.lastSpeed;
    Putcs(COM1, buf, 2);
}

void train_controller() {
  int requester;
  char request_buffer[1024] __attribute__ ((aligned (4)));
  packet_t * packet = (packet_t *) request_buffer;
  train_command_msg_t * msg = (train_command_msg_t *) request_buffer;
  sensor_data_t * sensor_data = (sensor_data_t *) request_buffer;
  train_navigate_t * navigate_msg = (train_navigate_t *) request_buffer;

  path_t navigation_data;

  int train;
  ReceiveS(&requester, train);
  ReplyN(requester);

  RegisterTrain(train);

  int lastSpeed = 0;
  int lastSensor = -1;

  while (true) {
    ReceiveS(&requester, request_buffer);
    ReplyN(requester);
    switch (packet->type) {
      case SENSOR_DATA:
        SetTrainLocation(train, sensor_data->sensor_no);
        lastSensor = sensor_data->sensor_no;
        // TODO: add other offset and calibration functions
        // (or move them from interactive)
        break;
      case TRAIN_CONTROLLER_COMMAND:
        switch (msg->type) {
        case TRAIN_CONTROLLER_SET_SPEED:
          Logf(EXECUTOR_LOGGING, "TC executing speed cmd");
          lastSpeed = msg->speed;
          SetTrainSpeed(train, msg->speed);
          break;
        case TRAIN_CONTROLLER_REVERSE: {
            Logf(EXECUTOR_LOGGING, "TC executing reverse cmd");
            int tid = Create(PRIORITY_TRAIN_COMMAND_TASK, reverse_train_task);
            reverse_train_t rev;
            rev.train = train;
            rev.lastSpeed = lastSpeed;
            SendSN(tid, rev);
          }
          break;
        }
        break;
      case TRAIN_NAVIGATE_COMMAND:
        SetTrainSpeed(train, navigate_msg->speed);
        // FIXME: priority
        CreateRouteExecutor(10, train, navigate_msg->speed, ROUTE_EXECUTOR_NAVIGATE, &navigation_data);
        break;
      case TRAIN_STOPFROM_COMMAND:
        navigation_data = navigate_msg->path; // Persist the path
        SetTrainSpeed(train, navigate_msg->speed);
        // FIXME: priority
        CreateRouteExecutor(10, train, navigate_msg->speed, ROUTE_EXECUTOR_STOPFROM, &navigation_data);
        break;
      default:
        KASSERT(false, "Train controller received unhandled packet. Got type=%d", packet->type);
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
    CreateTrainController(train);
  }
}

void AlertTrainController(int train, int sensor_no, int timestamp) {
  Logf(EXECUTOR_LOGGING, "alerting train=%d sensor=%d timestamp=%d", train, sensor_no, timestamp);
  ensure_train_controller(train);
  sensor_data_t data;
  data.packet.type = SENSOR_DATA;
  data.sensor_no = sensor_no;
  data.timestamp = timestamp;
  SendSN(train_controllers[train], data);
}

void TellTrainController(int train, int type, int speed) {
  Logf(EXECUTOR_LOGGING, "telling train=%d", train);
  ensure_train_controller(train);
  train_command_msg_t msg;
  msg.packet.type = TRAIN_CONTROLLER_COMMAND;
  msg.type = type;
  msg.speed = speed;
  SendSN(train_controllers[train], msg);
}

void NavigateTrain(int train, int speed, path_t * path) {
  ensure_train_controller(train);
  Logf(EXECUTOR_LOGGING, "navigating train=%d (tid=%d)", train, train_controllers[train]);
  train_navigate_t msg;
  msg.packet.type = TRAIN_NAVIGATE_COMMAND;
  msg.speed = speed;
  msg.path = *path;
  SendSN(train_controllers[train], msg);
}

void StopTrainAt(int train, int speed, path_t * path) {
  ensure_train_controller(train);
  Logf(EXECUTOR_LOGGING, "navigating train=%d (tid=%d)", train, train_controllers[train]);
  train_navigate_t msg;
  msg.packet.type = TRAIN_STOPFROM_COMMAND;
  msg.speed = speed;
  msg.path = *path;
  SendSN(train_controllers[train], msg);
}
