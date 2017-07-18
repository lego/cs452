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
      buf[0] = data.speed + 2;
      if (buf[0] > 14) {
        buf[0] = 14;
      }
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

void calibrate_set_speed() {
  train_task_t data;
  int receiver;
  ReceiveS(&receiver, data);
  ReplyN(receiver);
  TellTrainController(data.train, TRAIN_CONTROLLER_SET_SPEED, data.speed);
}

bool gluInvertMatrix(const float m[16], float invOut[16]) {
  float inv[16], det;
  int i;

  inv[0] = m[5]  * m[10] * m[15] -
  m[5]  * m[11] * m[14] -
  m[9]  * m[6]  * m[15] +
  m[9]  * m[7]  * m[14] +
  m[13] * m[6]  * m[11] -
  m[13] * m[7]  * m[10];

  inv[4] = -m[4]  * m[10] * m[15] +
  m[4]  * m[11] * m[14] +
  m[8]  * m[6]  * m[15] -
  m[8]  * m[7]  * m[14] -
  m[12] * m[6]  * m[11] +
  m[12] * m[7]  * m[10];

  inv[8] = m[4]  * m[9] * m[15] -
  m[4]  * m[11] * m[13] -
  m[8]  * m[5] * m[15] +
  m[8]  * m[7] * m[13] +
  m[12] * m[5] * m[11] -
  m[12] * m[7] * m[9];

  inv[12] = -m[4]  * m[9] * m[14] +
  m[4]  * m[10] * m[13] +
  m[8]  * m[5] * m[14] -
  m[8]  * m[6] * m[13] -
  m[12] * m[5] * m[10] +
  m[12] * m[6] * m[9];

  inv[1] = -m[1]  * m[10] * m[15] +
  m[1]  * m[11] * m[14] +
  m[9]  * m[2] * m[15] -
  m[9]  * m[3] * m[14] -
  m[13] * m[2] * m[11] +
  m[13] * m[3] * m[10];

  inv[5] = m[0]  * m[10] * m[15] -
  m[0]  * m[11] * m[14] -
  m[8]  * m[2] * m[15] +
  m[8]  * m[3] * m[14] +
  m[12] * m[2] * m[11] -
  m[12] * m[3] * m[10];

  inv[9] = -m[0]  * m[9] * m[15] +
  m[0]  * m[11] * m[13] +
  m[8]  * m[1] * m[15] -
  m[8]  * m[3] * m[13] -
  m[12] * m[1] * m[11] +
  m[12] * m[3] * m[9];

  inv[13] = m[0]  * m[9] * m[14] -
  m[0]  * m[10] * m[13] -
  m[8]  * m[1] * m[14] +
  m[8]  * m[2] * m[13] +
  m[12] * m[1] * m[10] -
  m[12] * m[2] * m[9];

  inv[2] = m[1]  * m[6] * m[15] -
  m[1]  * m[7] * m[14] -
  m[5]  * m[2] * m[15] +
  m[5]  * m[3] * m[14] +
  m[13] * m[2] * m[7] -
  m[13] * m[3] * m[6];

  inv[6] = -m[0]  * m[6] * m[15] +
  m[0]  * m[7] * m[14] +
  m[4]  * m[2] * m[15] -
  m[4]  * m[3] * m[14] -
  m[12] * m[2] * m[7] +
  m[12] * m[3] * m[6];

  inv[10] = m[0]  * m[5] * m[15] -
  m[0]  * m[7] * m[13] -
  m[4]  * m[1] * m[15] +
  m[4]  * m[3] * m[13] +
  m[12] * m[1] * m[7] -
  m[12] * m[3] * m[5];

  inv[14] = -m[0]  * m[5] * m[14] +
  m[0]  * m[6] * m[13] +
  m[4]  * m[1] * m[14] -
  m[4]  * m[2] * m[13] -
  m[12] * m[1] * m[6] +
  m[12] * m[2] * m[5];

  inv[3] = -m[1] * m[6] * m[11] +
  m[1] * m[7] * m[10] +
  m[5] * m[2] * m[11] -
  m[5] * m[3] * m[10] -
  m[9] * m[2] * m[7] +
  m[9] * m[3] * m[6];

  inv[7] = m[0] * m[6] * m[11] -
  m[0] * m[7] * m[10] -
  m[4] * m[2] * m[11] +
  m[4] * m[3] * m[10] +
  m[8] * m[2] * m[7] -
  m[8] * m[3] * m[6];

  inv[11] = -m[0] * m[5] * m[11] +
  m[0] * m[7] * m[9] +
  m[4] * m[1] * m[11] -
  m[4] * m[3] * m[9] -
  m[8] * m[1] * m[7] +
  m[8] * m[3] * m[5];

  inv[15] = m[0] * m[5] * m[10] -
  m[0] * m[6] * m[9] -
  m[4] * m[1] * m[10] +
  m[4] * m[2] * m[9] +
  m[8] * m[1] * m[6] -
  m[8] * m[2] * m[5];

  det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

  if (det == 0)
    return false;

  det = 1.0 / det;

  for (i = 0; i < 16; i++)
    invOut[i] = inv[i] * det;

  return true;
}


void train_controller() {
  int requester;
  char request_buffer[1024] __attribute__ ((aligned (4)));
  packet_t * packet = (packet_t *) request_buffer;
  train_command_msg_t * msg = (train_command_msg_t *) request_buffer;
  sensor_data_t * sensor_data = (sensor_data_t *) request_buffer;
  train_navigate_t * navigate_msg = (train_navigate_t *) request_buffer;

  path_t navigation_data;
  // This is here as a signifier for not properly initializing this.
  navigation_data.len = 0xDEADBEEF;

  int train;
  ReceiveS(&requester, train);
  ReplyN(requester);

  RegisterTrain(train);

  int lastSpeed = 0;
  int lastSensor = -1;
  int lastSensorTime = 0;

  bool calibrating = false;
  const int numLastVelocities = 3;
  int lastVels[numLastVelocities];
  int lastVelsStart = 0;
  int nSamples = 0;
  int calibratedSpeeds[4] = { -1, -1, -1, -1 };
  int calibrationSpeeds[4] = { 5, 7, 11, 13 };
  int calibrationSpeedN = 0;
  int calibrationLockTime = 0;

  while (true) {
    ReceiveS(&requester, request_buffer);
    ReplyN(requester);
    int time = Time();
    switch (packet->type) {
      case SENSOR_DATA:
        {
          SetTrainLocation(train, sensor_data->sensor_no);

          if (calibrating) {
            int velocity = 0;
            int dist = adjSensorDist(lastSensor, sensor_data->sensor_no);
            if (dist != -1) {
              int time_diff = sensor_data->timestamp - lastSensorTime;
              velocity = (dist * 100) / time_diff;
            }
            if (Time() > calibrationLockTime && velocity > 0) {
              record_velocity_sample(train, lastSpeed, velocity);
              int avgV = Velocity(train, lastSpeed);
              nSamples++;
              if (nSamples > 5) {
                bool same = true;
                for (int i = 0; i < numLastVelocities; i++) {
                  if (ABS(lastVels[i] - avgV) > calibrationSpeedN) {
                    same = false;
                    break;
                  }
                }
                lastVels[lastVelsStart] = avgV;
                lastVelsStart = (lastVelsStart + 1) % numLastVelocities;
                if (same) {
                  Logf(PACKET_LOG_INFO, "Calibrated train %d velocity for speed %d: %d",
                      train, lastSpeed, avgV);
                  {
                    calibratedSpeeds[calibrationSpeedN] = avgV;
                    int nextSpeed = 0;
                    if (calibrationSpeedN < 3) {
                      calibrationSpeedN += 1;
                      nextSpeed = calibrationSpeeds[calibrationSpeedN];
                    } else {
                      Logf(PACKET_LOG_INFO, "Calibration Done!");
                      for (int i = 0; i < 4; i++) {
                        Logf(PACKET_LOG_INFO, "Calibration[%d] = %d!", calibrationSpeeds[i], calibratedSpeeds[i]);
                      }
                      float coeffMat[16];
                      float invMat[16];
                      float coeffs[4];
                      for (int i = 0; i < 4; i++) {
                        float s = (float)calibrationSpeeds[i];
                        coeffMat[i*4+0] = s*s*s;
                        coeffMat[i*4+1] = s*s;
                        coeffMat[i*4+2] = s;
                        coeffMat[i*4+3] = 1;
                      }
                      gluInvertMatrix(coeffMat, invMat);
                      for (int i = 0; i < 4; i++) {
                        float total = 0.0f;
                        for (int j = 0; j < 4; j++) {
                          total += invMat[i*4+j] * (float)calibratedSpeeds[j];
                        }
                        coeffs[i] = total;
                      }
                      for (int i = 0; i < 10; i++) {
                        int x = i + 5;
                        int est = (int)(coeffs[0]*x*x*x + coeffs[1]*x*x + coeffs[2]*x + coeffs[3]);
                        Logf(PACKET_LOG_INFO, "Est[%d] = %d!", x, est);
                        set_velocity(train, x, est);
                      }
                      calibrating = false;
                    }
                    int tid = Create(PRIORITY_TRAIN_COMMAND_TASK, calibrate_set_speed);
                    train_task_t sp;
                    sp.train = train;
                    sp.speed = nextSpeed;
                    SendSN(tid, sp);
                    nSamples = 0;
                    calibrationLockTime = Time() + 400;
                    lastSensor = -1;
                  }
                }
              }
            }
            Logf(PACKET_LOG_INFO, "Train %d velocity sample at %s: %d, avg: %d",
                train, track[sensor_data->sensor_no].name,
                velocity, Velocity(train, lastSpeed));
            lastSensor = sensor_data->sensor_no;
            lastSensorTime = sensor_data->timestamp;
          }
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
        case TRAIN_CONTROLLER_CALIBRATE: {
            Logf(EXECUTOR_LOGGING, "TC executing calibrate cmd");
            for (int i = 0; i < numLastVelocities; i++) {
              lastVels[i] = 0;
            }
            nSamples = 0;
            calibrationSpeedN = 0;
            int tid = Create(PRIORITY_TRAIN_COMMAND_TASK, calibrate_set_speed);
            train_task_t sp;
            sp.train = train;
            sp.speed = calibrationSpeeds[calibrationSpeedN];
            SendSN(tid, sp);
            calibrationLockTime = Time() + 400;
            calibrating = true;
          }
          break;
        case TRAIN_CONTROLLER_SET_SPEED: {
            Logf(EXECUTOR_LOGGING, "TC executing speed cmd");
            lastSpeed = msg->speed;
            int tid = Create(PRIORITY_TRAIN_COMMAND_TASK, train_speed_task);
            train_task_t sp;
            sp.train = train;
            sp.speed = msg->speed;
            SendSN(tid, sp);
          }
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
  Logf(EXECUTOR_LOGGING, "alerting train=%d sensor %4s timestamp=%d", train, track[sensor_no].name, timestamp);
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
