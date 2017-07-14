#include <basic.h>
#include <detective/detector.h>
#include <detective/interval_detector.h>
#include <priorities.h>
#include <kernel.h>
#include <servers/clock_server.h>

volatile int interval_detector_counter = 0;

typedef struct {
  int send_to;
  int interval_ticks;
  int identifier;
} interval_detector_init_t;

void interval_detector_task() {
  int sender;
  interval_detector_init_t init;
  ReceiveS(&sender, init);
  ReplyN(sender);

  detector_message_t msg;
  msg.packet.type = INTERVAL_DETECT;
  msg.details = init.interval_ticks;
  msg.identifier = init.identifier;

  while (true) {
    Delay(init.interval_ticks);
    SendSN(init.send_to, msg);
  }
}

int StartIntervalDetector(const char * name, int send_to, int interval_ticks) {
  int tid = CreateWithName(PRIORITY_DELAY_DETECTOR, interval_detector_task, name);
  interval_detector_init_t init;
  init.send_to = send_to;
  init.interval_ticks = interval_ticks;
  init.identifier = interval_detector_counter++;
  SendSN(tid, init);
  return init.identifier;
}
