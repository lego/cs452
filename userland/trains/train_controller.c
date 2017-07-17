#include <trains/train_controller.h>
#include <basic.h>
#include <util.h>
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
  int speed;
} train_task_t;

void train_speed_task() {
    train_task_t data;
    int receiver;
    ReceiveS(&receiver, data);
    ReplyN(receiver);
    char buf[2];
    buf[1] = data.train;
    if (data.speed > 0) {
      buf[0] = 14;
      Putcs(COM1, buf, 2);
      Delay(10);
    }
    buf[0] = data.speed;
    Putcs(COM1, buf, 2);
}

void reverse_train_task() {
    train_task_t data;
    int receiver;
    ReceiveS(&receiver, data);
    ReplyN(receiver);
    char buf[2];
    buf[0] = 0;
    buf[1] = data.train;
    Putcs(COM1, buf, 2);
    Delay(300);
    buf[0] = 15;
    Putcs(COM1, buf, 2);
    int lastSensor = WhereAmI(data.train);
    if (lastSensor >= 0) {
      RegisterTrainReverse(data.train, lastSensor);
    }
    Delay(10);
    buf[0] = data.speed;
    Putcs(COM1, buf, 2);
}

static void start_navigation(int train, path_t * path) {
  // FIXME: priority
  CreateRouteExecutor(10, train, path);
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
  int lastSensorTime = 0;

  while (true) {
    ReceiveS(&requester, request_buffer);
    ReplyN(requester);
    int time = Time();
    switch (packet->type) {
      case SENSOR_DATA:
        {
          SetTrainLocation(train, sensor_data->sensor_no);

          int velocity = 0;
          int dist = adjSensorDist(lastSensor, sensor_data->sensor_no);
          if (dist != -1) {
            int time_diff = sensor_data->timestamp - lastSensorTime;
            velocity = (dist * 100) / time_diff;
          }
          if (velocity > 0) {
            record_velocity_sample(train, lastSpeed, velocity);
          }
          //Logf(PACKET_LOG_INFO, "Train %d velocity sample at %s: %d, avg: %d",
          //    train, track[sensor_data->sensor_no].name,
          //    velocity, Velocity(train, lastSpeed));

          lastSensor = sensor_data->sensor_no;
          lastSensorTime = sensor_data->timestamp;
          // TODO: add other offset and calibration functions
          // (or move them from interactive)
          {
            node_dist_t nd = nextSensor(sensor_data->sensor_no);
            uart_packet_fixed_size_t packet;
            packet.type = PACKET_TRAIN_LOCATION_DATA;
            packet.len = 11;
            jmemcpy(&packet.data[0], &time, sizeof(int));
            packet.data[4] = train;
            packet.data[5] = sensor_data->sensor_no;
            packet.data[6] = nd.node;
            int nextDist = nd.dist;
            int tmp = 0;
            if (nextDist > 0) {
              tmp = (10000*Velocity(train, lastSpeed)) / nextDist;
            }
            jmemcpy(&packet.data[7], &tmp, sizeof(int));
            PutFixedPacket(&packet);
          }
        }
        break;
      case TRAIN_CONTROLLER_COMMAND:
        switch (msg->type) {
        case TRAIN_CONTROLLER_SET_SPEED:
          Logf(EXECUTOR_LOGGING, "TC executing speed cmd");
          lastSpeed = msg->speed;
          lastSensor = -1;
          int tid = Create(PRIORITY_TRAIN_COMMAND_TASK, train_speed_task);
          train_task_t sp;
          sp.train = train;
          sp.speed = msg->speed;
          SendSN(tid, sp);
          break;
        case TRAIN_CONTROLLER_REVERSE: {
            Logf(EXECUTOR_LOGGING, "TC executing reverse cmd");
            int tid = Create(PRIORITY_TRAIN_COMMAND_TASK, reverse_train_task);
            train_task_t rev;
            rev.train = train;
            rev.speed = lastSpeed;
            SendSN(tid, rev);
          }
          break;
        }
        break;
      case TRAIN_NAVIGATE_COMMAND:
        navigation_data = navigate_msg->path; // Persist the path
        start_navigation(train, &navigation_data);
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
