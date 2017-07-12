#include <detective/detector.h>
#include <detective/sensor_timeout_detective.h>
#include <detective/sensor_detector.h>
#include <detective/delay_detector.h>
#include <priorities.h>
#include <kernel.h>
#include <jstring.h>

volatile int sensor_timeout_detective_counter = 0;

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

  char buffer[128];
  jstrappend(my_name, " - sense", buffer);
  StartSensorDetector(buffer, tid, init.sensor_no);

  buffer[0] = '\0';
  jstrappend(my_name, " - delay", buffer);
  StartDelayDetector(buffer, tid, init.timeout);

  detector_message_t detector;
  ReceiveS(&sender, detector);
  int activated_action;
  switch (detector.details) {
    case DELAY_DETECT:
      activated_action = DETECTIVE_TIMEOUT;
      break;
    case SENSOR_DETECT:
      activated_action = DETECTIVE_SENSOR;
      break;
    default:
      activated_action = 0; // makes the compiler happy
      KASSERT(false, "Detective had bad case.");
  }

  sensor_timeout_message_t msg;
  msg.packet.type = SENSOR_TIMEOUT_DETECTIVE;
  msg.action = activated_action;
  msg.timeout = init.timeout;
  msg.sensor_no = init.sensor_no;
  msg.identifier = init.identifier;

  SendSN(init.send_to, msg);

  // Destroy self, in order to clean up children
  // FIXME: merge in complete destroy
  // Destroy(tid);
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
