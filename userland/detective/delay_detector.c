#include <detective/detector.h>
#include <detective/delay_detector.h>
#include <priorities.h>
#include <kernel.h>
#include <servers/clock_server.h>

volatile int delay_detector_counter = 1;

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
  msg.identifier = MyTid();

  Delay(init.ticks);
  SendSN(init.send_to, msg);
}

int StartRecyclableDelayDetector(const char * name, int send_to, int ticks) {
  KASSERT(ticks <= 1000 && ticks >= 0, "StartDelayDetector got a negative or huge value Please fix me! ticks=%d", ticks);
  int tid = CreateRecyclableWithName(PRIORITY_DELAY_DETECTOR, delay_detector, name);
  delay_detector_init_t init;
  init.send_to = send_to;
  init.ticks = ticks;
  init.identifier = delay_detector_counter++;
  SendSN(tid, init);
  return tid;
}

int StartDelayDetector(const char * name, int send_to, int ticks) {
  KASSERT(ticks <= 1000 && ticks >= 0, "StartDelayDetector got a negative or huge value Please fix me! ticks=%d", ticks);
  int tid = CreateWithName(PRIORITY_DELAY_DETECTOR, delay_detector, name);
  delay_detector_init_t init;
  init.send_to = send_to;
  init.ticks = ticks;
  init.identifier = delay_detector_counter++;
  SendSN(tid, init);
  return tid;
}
