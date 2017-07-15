#include <detective/detector.h>
#include <detective/sensor_timeout_detective.h>
#include <detective/sensor_detector.h>
#include <detective/delay_detector.h>
#include <track/pathing.h>
#include <priorities.h>
#include <kernel.h>
#include <jstring.h>

volatile int sensor_timeout_detective_counter = 1;

typedef struct {
  int send_to;
  int sensor_no;
  int timeout;
  int identifier;
} sensor_timeout_init_t;

void sensor_timeout_detective() {
  int tid = MyTid();
  const char * my_name = MyTaskName();

  int sender;
  sensor_timeout_init_t init;
  ReceiveS(&sender, init);
  ReplyN(sender);

  RecordLogf("Started sensor timeout detective: timeout=%d sensor=%s\n\r", init.timeout, track[init.sensor_no].name);

  char buffer[128];
  jformatf(buffer, sizeof(buffer), "SenTimeDet %d - sense %s", init.identifier, track[init.sensor_no].name);
  StartSensorDetector(buffer, tid, init.sensor_no);

  jformatf(buffer, sizeof(buffer), "SenTimeDet %d - delay %dms", init.identifier, init.timeout * 10);
  StartDelayDetector(buffer, tid, init.timeout);

  detector_message_t detector;
  ReceiveS(&sender, detector);
  int activated_action = -1; // makes compiler happy, make it something that will break stuff
  switch (detector.packet.type) {
    case DELAY_DETECT:
      activated_action = DETECTIVE_TIMEOUT;
      break;
    case SENSOR_DETECT:
      activated_action = DETECTIVE_SENSOR;
      break;
    default:
      KASSERT(false, "Detective received bad packet type=%d\n\r", detector.packet.type);
  }

  sensor_timeout_message_t msg;
  msg.packet.type = SENSOR_TIMEOUT_DETECTIVE;
  msg.action = activated_action;
  msg.timeout = init.timeout;
  msg.sensor_no = init.sensor_no;
  msg.identifier = init.identifier;

  SendSN(init.send_to, msg);

  // Destroy self, in order to clean up children
  Destroy(tid);
}

int StartSensorTimeoutDetective(const char * name, int send_to, int timeout, int sensor_no) {
  int tid = CreateWithName(PRIORITY_SENSOR_TIMEOUT_DETECTIVE, sensor_timeout_detective, name);
  sensor_timeout_init_t init;
  init.send_to = send_to;
  init.timeout = timeout;
  init.sensor_no = sensor_no;
  init.identifier = sensor_timeout_detective_counter++;
  SendSN(tid, init);
  return init.identifier;
}
