#include <packet.h>
#include <basic.h>
#include <kernel.h>
#include <util.h>
#include <bwio.h>
#include <servers/nameserver.h>
#include <servers/clock_server.h>
#include <servers/uart_rx_server.h>
#include <servers/uart_tx_server.h>
#include <trains/sensor_collector.h>
#include <trains/switch_controller.h>
#include <trains/train_controller.h>
#include <trains/navigation.h>
#include <track/pathing.h>
#include <priorities.h>

#define NUM_SENSORS 80
#define SENSOR_MEMORY 80

int sensor_attributer_tid = -1;

typedef struct {
  // type = SENSOR_ATTRIB_ASSIGN_TRAIN
  packet_t packet;

  int train;
} sensor_attributer_req_t;

void sensor_notifier() {
  int requester;

  int train;
  sensor_data_t data;

  ReceiveS(&requester, data);
  ReplyN(requester);
  ReceiveS(&requester, train);
  ReplyN(requester);

  // Send sensor attribution to UI
  uart_packet_fixed_size_t packet;
  packet.len = 6;
  packet.type = PACKET_SENSOR_DATA;
  jmemcpy(&packet.data[0], &data.timestamp, sizeof(int));
  packet.data[4] = data.sensor_no;
  packet.data[5] = train;
  PutFixedPacket(&packet);

  // Send sensor attribution to train
  if (train != -1) {
    AlertTrainController(train, data.sensor_no, data.timestamp);
  }
}

void sensor_attributer() {
  sensor_attributer_tid = MyTid();

  int active_train = -1;

  int lastTrainAtSensor[NUM_SENSORS][SENSOR_MEMORY];
  for (int i = 0; i < NUM_SENSORS; i++) {
    for (int j = 0; j < SENSOR_MEMORY; j++) {
      lastTrainAtSensor[i][j] = -1;
    }
  }

  int trainControllerTids[TRAINS_MAX];
  for (int i = 0; i < TRAINS_MAX; i++) {
    trainControllerTids[i] = -1;
  }

  int requester;
  char buffer[128] __attribute__((aligned (4)));
  packet_t *packet = (packet_t *)buffer;
  sensor_attributer_req_t *request = (sensor_attributer_req_t *)buffer;
  sensor_data_t *data = (sensor_data_t *)buffer;

  while (true) {
    ReceiveS(&requester, buffer);
    ReplyN(requester);
    switch (packet->type) {
      case SENSOR_ATTRIB_ASSIGN_TRAIN:
        KASSERT(active_train == -1, "More than one unknown train active at the same time. Cannot do sensor attribution.");
        active_train = request->train;
        trainControllerTids[active_train] = requester;
        break;
      case SENSOR_DATA: {
          int attrib = -1;
          if (active_train != -1) {
            attrib = active_train;
            active_train = -1;
            lastTrainAtSensor[data->sensor_no][0] = attrib;
          } else {
            int node = track[data->sensor_no].reverse->id;
            for (int i = 0; i < 3 && attrib == -1; i++) {
              node = findSensorOrBranch(node);
              while (node >= 0 && track[node].type == NODE_BRANCH) {
                int state = GetSwitchState(track[node].num);
                node = track[node].edge[state].dest->id;
                if (track[node].type != NODE_SENSOR) {
                  node = findSensorOrBranch(node);
                }
              }
              if (node >= 0 && track[node].type == NODE_SENSOR) {
                int reverseNode = track[node].reverse->id;
                if (lastTrainAtSensor[reverseNode][0] != -1) {
                  attrib = lastTrainAtSensor[reverseNode][0];
                  lastTrainAtSensor[reverseNode][0] = -1;
                  lastTrainAtSensor[data->sensor_no][0] = attrib;
                }
              }
            }
          }
          // TODO: break this out into a AlertSensorAttribution func
          int notifier = Create(PRIORITY_UART2_TX_SERVER, sensor_notifier);
          // Send sensor data
          Send(notifier, data, sizeof(sensor_data_t), NULL, 0);
          // Send train attributed to
          SendSN(notifier, attrib);
        }
        break;
      default:
        KASSERT(false, "Unhandled packet type=%d", packet->type);
    }
  }
}

void sensor_collector_task() {
  int tid = MyTid();
  int parent = MyParentTid();
  int sensor_detector_multiplexer_tid = WhoIsEnsured(NS_SENSOR_DETECTOR_MULTIPLEXER);

  sensor_data_t req;
  req.packet.type = SENSOR_DATA;

  log_task("sensor_reader initialized parent=%d", tid, parent);
  int oldSensors[5];
  int sensors[5];
  Delay(50); // Wait half a second for old COM1 input to be read
  while (true) {
    log_task("sensor_reader sleeping", tid);
    ClearRx(COM1);
    Putc(COM1, 0x85);
    int queueLength = 0;
    const int maxTries = 100;
    int i;
    for (i = 0; i < maxTries && queueLength < 10; i++) {
      Delay(1);
      queueLength = GetRxQueueLength(COM1);
    }
    // We tried to read but failed to get the correct number of bytes, ABORT
    if (i == maxTries) {
      continue;
    }
    log_task("sensor_reader reading", tid);
    for (int i = 0; i < 5; i++) {
      char high = Getc(COM1);
      char low = Getc(COM1);
      sensors[i] = (high << 8) | low;
    }
    log_task("sensor_reader read", tid);
    req.timestamp = Time();
    for (int i = 0; i < 5; i++) {
      if (sensors[i] != oldSensors[i]) {
        for (int j = 0; j < 16; j++) {
          if ((sensors[i] & (1 << j)) & ~(oldSensors[i] & (1 << j))) {
            // Send to attributer
            req.sensor_no = i*16+(15-j);
            SendSN(sensor_attributer_tid, req);
            // Send to detector multiplexer
            SendSN(sensor_detector_multiplexer_tid, req);
          }
        }
      }
      oldSensors[i] = sensors[i];
    }
  }
}

int RegisterTrain(int train) {
  KASSERT(train >= 0 && train <= TRAINS_MAX, "Invalid train when registering with sensor attributer. Got %d", train);
  if (sensor_attributer_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "Sensor Attributer server not initialized");
    return -1;
  }
  sensor_attributer_req_t request;
  request.packet.type = SENSOR_ATTRIB_ASSIGN_TRAIN;
  request.train = train;
  SendSN(sensor_attributer_tid, request);
  return 0;
}
