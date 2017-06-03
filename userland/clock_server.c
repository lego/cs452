#include <clock_server.h>
#include <kernel.h>
#include <nameserver.h>

enum {
  NOTIFIER,
  TIME_REQUEST,
  DELAY_REQUEST
};

typedef struct {
  int type;
  volatile unsigned int time_value;
} clock_request_t;

void clock_notifier() {
  RegisterAs(CLOCK_NOTIFIER);
  int clock_server_tid = MyParentTid();

  clock_request_t req;
  req.type = NOTIFIER;
  while (true) {
    req.time_value = AwaitEvent(EVENT_TIMER);
    int result = Send(clock_server_tid, &req, sizeof(clock_request_t), NULL, 0);
    // FIXME: handle bad results ?
  }
}

void clock_server() {
  RegisterAs(CLOCK_SERVER);
  Create(1, clock_notifier);

  int requester;
  unsigned long int ticks;

  clock_request_t request;

  while (true) {
      int size_received = Receive(&requester, &request, sizeof(clock_request_t));
      // FIXME: handle size_received for error handling

      switch ( request.type ) {
      case NOTIFIER:
        // update time and check for terminated delays
        ReplyN(requester);
        // ticks = request.time_value;
        ticks += 0xFFFF; // MAX_TIME from io.c
      case TIME_REQUEST:
        // reply with time
        ReplyS(requester, ticks);
      case DELAY_REQUEST:
        break;
        // Add requester to list of suspended tasks
      }
      // Reply to suspended tasks that have timed out
  }
}

int Delay( int tid, unsigned int delay ) {
  return 0;
}

int Time( int tid ) {
  clock_request_t req;
  req.type = TIME_REQUEST;
  volatile unsigned long int time_value;
  SendS(tid, req, time_value);
  return time_value;
}

int DelayUntil( int tid, unsigned long int until ) {
  return 0;
}
