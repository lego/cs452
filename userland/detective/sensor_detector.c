#include <detective/detector.h>
#include <detective/sensor_detector.h>
#include <trains/sensor_collector.h>
#include <servers/nameserver.h>
#include <servers/uart_tx_server.h>
#include <track/pathing.h>
#include <priorities.h>
#include <kernel.h>
#include <packet.h>

volatile int sensor_detector_counter = 1;

typedef struct {
  int send_to;
  int sensor_no;
  int identifier;
} sensor_detector_init_t;

void sensor_detector() {
  int sender;
  int sensor_multiplexer_tid = WhoIsEnsured(NS_SENSOR_DETECTOR_MULTIPLEXER);
  sensor_detector_init_t init;
  ReceiveS(&sender, init);
  ReplyN(sender);

  detector_message_t msg;
  msg.packet.type = SENSOR_DETECT;
  msg.details = init.sensor_no;
  msg.identifier = init.identifier;

  packet_t request;
  request.type = SENSOR_DETECTOR_REQUEST;

  sensor_data_t data;

  Logf(EXECUTOR_LOGGING, "Detector started for %s", track[init.sensor_no].name);

  // Listen to all sensor data, waiting for the one we want
  while (true) {
    SendS(sensor_multiplexer_tid, request, data);
    if (data.sensor_no == init.sensor_no) break;
  }

  SendSN(init.send_to, msg);
}

int StartSensorDetector(const char * name, int send_to, int sensor_no) {
  int tid = CreateWithName(PRIORITY_SENSOR_DETECTOR, sensor_detector, name);
  sensor_detector_init_t init;
  init.send_to = send_to;
  init.sensor_no = sensor_no;
  init.identifier = sensor_detector_counter++;
  SendSN(tid, init);
  return init.identifier;
}
