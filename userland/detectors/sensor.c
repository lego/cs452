#include <detectors/detector.h>
#include <detectors/sensor.h>
#include <priorities.h>
#include <kernel.h>

volatile int sensor_detector_counter = 0;

typedef struct {
  int send_to;
  int sensor_no;
  int identifier;
} sensor_detector_init_t;

void sensor_detector() {
  int sender;
  sensor_detector_init_t init;
  ReceiveS(&sender, init);
  ReplyN(sender);

  detector_message_t msg;
  msg.packet.type = DELAY_DETECT;
  msg.details = init.sensor_no;
  msg.identifier = init.identifier;

  // FIXME: figure out a way to block for a sensor

  SendSN(init.send_to, msg);
}

int StartSensorDetector(const char * name, int send_to, int sensor_no) {
  int detector_tid = CreateWithName(PRIORITY_SENSOR_DETECTOR, sensor_detector, name);
  sensor_detector_init_t init;
  init.send_to = send_to;
  init.sensor_no = sensor_no;
  init.identifier = sensor_detector_counter++;
  SendSN(detector_tid, init);
  return init.identifier;
}
