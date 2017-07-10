#include <detectors/detector.h>
#include <detectors/delay.h>
#include <priorities.h>
#include <kernel.h>
#include <servers/clock_server.h>

volatile int delay_detector_counter = 0;

typedef struct {
  int send_to;
  int ticks;
  int identifier;
} delay_detector_init_t;

void delay_detector() {
  int sender;
  delay_detector_init_t init;
  ReceiveS(&sender, init);
  ReplyN(sender);

  detector_message_t msg;
  msg.packet.type = DELAY_DETECT;
  msg.details = init.ticks;
  msg.identifier = init.identifier;

  Delay(init.ticks);
  SendSN(init.send_to, msg);
}

int StartDelayDetector(const char * name, int send_to, int ticks) {
  int detector_tid = CreateWithName(PRIORITY_DELAY_DETECTOR, delay_detector, name);
  delay_detector_init_t init;
  init.send_to = send_to;
  init.ticks = ticks;
  init.identifier = delay_detector_counter++;
  SendSN(detector_tid, init);
  return init.identifier;
}
