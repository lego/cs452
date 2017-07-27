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
#include <trains/train_controller.h>
#include <trains/navigation.h>
#include <track/pathing.h>
#include <priorities.h>

#define NUM_SENSORS 80
#define SENSOR_MEMORY 5

int sensor_attributer_tid = -1;

typedef struct {
  // type = SENSOR_ATTRIB_ASSIGN_TRAIN
  packet_t packet;

  int train;
} sensor_attributer_req_t;

typedef struct {
  // type = SENSOR_ATTRIB_TRAIN_<COMMAND>
  packet_t packet;

  int train;
  int lastSensor;
} sensor_attributer_train_update_t;

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
  RegisterAs(NS_SENSOR_ATTRIBUTER);

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
  sensor_attributer_train_update_t *train_update = (sensor_attributer_train_update_t *)buffer;
  sensor_data_t *data = (sensor_data_t *)buffer;

  // Returns the next expected train for some sensor
  int getNextExpectedTrainAt(int sensor) {
    int ret = -1;
    if (lastTrainAtSensor[sensor][0] != -1) {
      ret = lastTrainAtSensor[sensor][0];
      jmemmove(&lastTrainAtSensor[sensor][0], &lastTrainAtSensor[sensor][1], (SENSOR_MEMORY-1)*sizeof(int));
      lastTrainAtSensor[sensor][SENSOR_MEMORY-1] = -1;
    }
    return ret;
  }

  // sets a train as expected at a given sensor
  int expectTrainAt(int train, int sensor) {
    for (int i = 0; i < SENSOR_MEMORY; i++) {
      if (lastTrainAtSensor[sensor][i] == -1) {
        lastTrainAtSensor[sensor][i] = train;
        return 0;
      }
    }
    return -1;
  }

  // sets a train as expected at a given sensor
  int expectTrainAtNext(int train, int sensor) {
    int node = nextSensor(sensor).node;
    if (node != -1) {
      return expectTrainAt(train, node);
    }
    return -1;
  }

  // Assuming attribution for the given sensor has failed, we should:
  //   1: look backwards and see if we passed a branch, and if so check if we
  //      expected a train on the other side of the branch
  //   2: look backwards one sensor to see if we expect a train there, and if so
  //      we most likely missed the sensor
  int checkForBrokenSwitchOrSensor(int sensor) {
    // Look backwards to the prev sensor
    int prevSensor = nextSensor(track[sensor].reverse->id).node;
    if (prevSensor != -1) {
      prevSensor = track[prevSensor].reverse->id;

      // Check the branch first to see if it failed
      int branch = findSensorOrBranch(prevSensor).node;
      if (branch != -1 && track[branch].type == NODE_BRANCH) {
        int node = track[branch].edge[DIR_STRAIGHT].dest->id;
        if (track[node].type != NODE_SENSOR) {
          node = nextSensor(node).node;
        }
        if (node == -1 || lastTrainAtSensor[node][0] == -1) {
          node = track[branch].edge[DIR_CURVED].dest->id;
          if (track[node].type != NODE_SENSOR) {
            node = nextSensor(node).node;
          }
        }
        if (node != -1 && lastTrainAtSensor[node][0] != -1) {
          return getNextExpectedTrainAt(node);
        }
      }

      // If it's not a switch that failed, check if a sensor failed
      if (lastTrainAtSensor[prevSensor][0] != -1) {
        return getNextExpectedTrainAt(prevSensor);
      }
    }
    return -1;
  }

  // Clears the attribution of a given train by first looking where its attribution should be,
  //   and if it's not there, looking everywhere it find it
  //   lastSensor can be -1 to just look everywhere to clear it
  int clearTrainAttribution(int train, int lastSensor) {
    // Look forward from the last sensor
    //   This is mostly to save time looking everywhere
    int foundTrainAt = -1;
    if (lastSensor != -1) {
      int node = nextSensor(lastSensor).node;
      if (node != -1) {
        for (int j = 0; j < SENSOR_MEMORY; j++) {
          if (lastTrainAtSensor[node][j] == train) {
            lastTrainAtSensor[node][j] = -1;
            jmemmove(&lastTrainAtSensor[node][j], &lastTrainAtSensor[node][j+1], (SENSOR_MEMORY-1-j)*sizeof(int));
            foundTrainAt = node;
            break;
          }
        }
      }
    }
    // Ok, we didn't find it where we expected, look everywhere
    if (foundTrainAt == -1) {
      // Didn't find it there, fall back to looking everywhere
      for (int i = 0; i < NUM_SENSORS && foundTrainAt == -1; i++) {
        for (int j = 0; j < SENSOR_MEMORY; j++) {
          if (lastTrainAtSensor[i][j] == train) {
            lastTrainAtSensor[i][j] = -1;
            jmemmove(&lastTrainAtSensor[i][j], &lastTrainAtSensor[i][j+1], (SENSOR_MEMORY-1-j)*sizeof(int));
            foundTrainAt = i;
            break;
          }
        }
      }
    }
    return foundTrainAt;
  }

  while (true) {
    Receive(&requester, buffer, sizeof(buffer));
    ReplyN(requester);
    switch (packet->type) {
      case SENSOR_ATTRIB_ASSIGN_TRAIN:
        KASSERT(active_train == -1, "More than one unknown train active at the same time. Cannot do sensor attribution.");
        active_train = request->train;
        trainControllerTids[active_train] = requester;
        break;
      case SENSOR_ATTRIB_TRAIN_REREGISTER: {
          clearTrainAttribution(train_update->train, train_update->lastSensor);
          expectTrainAtNext(train_update->train, train_update->lastSensor);
        }
        break;
      case SENSOR_ATTRIB_TRAIN_REVERSE: {
          clearTrainAttribution(train_update->train, train_update->lastSensor);
          int node = track[train_update->lastSensor].reverse->id;
          if (node != -1) {
            expectTrainAt(train_update->train, node);
          }
        }
        break;
      case SENSOR_DATA: {
          int attrib = -1;
          int sensor = data->sensor_no;

          // First check if we expected a train here
          attrib = getNextExpectedTrainAt(sensor);

          // Next, check if we have an unattributed train
          if (attrib == -1 && active_train != -1) {
            attrib = active_train;
            active_train = -1;
          }

          // Next, check if we found a broken switch or sensor
          if (attrib == -1) {
            attrib = checkForBrokenSwitchOrSensor(sensor);
          }

          // If we found a train, and have a next sensor, set it to be expected
          if (attrib != -1) {
            expectTrainAtNext(attrib, sensor);

          }
          // Finally, notify the appropriate train controller as a new task
          { //if (attrib == 71 || attrib == 63){
            // TODO: break this out into a AlertSensorAttribution func
            int notifier = CreateRecyclable(PRIORITY_UART2_TX_SERVER, sensor_notifier);
            // Send sensor data
            Send(notifier, data, sizeof(sensor_data_t), NULL, 0);
            // Send train attributed to
            SendSN(notifier, attrib);
          }
        }
        break;
      default:
        KASSERT(false, "Unhandled packet type=%d", packet->type);
    }
  }
}

void fake_sensor_collector_task() {
  int sensor_detector_multiplexer_tid = WhoIsEnsured(NS_SENSOR_DETECTOR_MULTIPLEXER);

  int last = Name2Node("E14");

  Delay(600);

  int index = 0;
  sensor_data_t req;
  req.packet.type = SENSOR_DATA;

  while (true) {
    int next = nextSensor(last).node;
    req.timestamp = Time();
    req.sensor_no = next;
    SendSN(sensor_attributer_tid, req);
    // Send to detector multiplexer
    SendSN(sensor_detector_multiplexer_tid, req);

    last = next;
    Delay(100);
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
    { // Wait until we have enough bytes in the input buffer
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
    }
    { // Actually read the bytes from the input buffer
      log_task("sensor_reader reading", tid);
      for (int i = 0; i < 5; i++) {
        char high = Getc(COM1);
        char low = Getc(COM1);
        sensors[i] = (high << 8) | low;
      }
    }
    { // Check if any sensors changed, and send the update to the attributer
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

int RegisterTrainReverse(int train, int lastSensor) {
  KASSERT(train >= 0 && train <= TRAINS_MAX, "Invalid train when registering reverse with sensor attributer. Got %d", train);
  if (sensor_attributer_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "Sensor Attributer server not initialized");
    return -1;
  }
  sensor_attributer_train_update_t request;
  request.packet.type = SENSOR_ATTRIB_TRAIN_REVERSE;
  request.train = train;
  request.lastSensor = lastSensor;
  SendSN(sensor_attributer_tid, request);
  return 0;
}

int ReRegisterTrain(int train, int lastSensor) {
  KASSERT(train >= 0 && train <= TRAINS_MAX, "Invalid train when re-registering with sensor attributer. Got %d", train);
  if (sensor_attributer_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "Sensor Attributer server not initialized");
    return -1;
  }
  sensor_attributer_train_update_t request;
  request.packet.type = SENSOR_ATTRIB_TRAIN_REREGISTER;
  request.train = train;
  request.lastSensor = lastSensor;
  SendSN(sensor_attributer_tid, request);
  return 0;
}
