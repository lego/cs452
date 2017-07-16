#include <basic.h>
#include <bwio.h>
#include <servers/clock_server.h>
#include <kernel.h>
#include <servers/nameserver.h>
#include <heap.h>
#include <priorities.h>

static int clock_server_tid = -1;

enum {
  NOTIFIER,
  TIME_REQUEST,
  DELAY_REQUEST,
  DELAY_UNTIL_REQUEST
};

typedef struct {
  int type;
  volatile unsigned int time_value;
} clock_request_t;

void clock_notifier() {
  int tid = MyTid();
  RegisterAs(NS_CLOCK_NOTIFIER);
  log_clock_server("clock_notifier initialized", tid);
  int clock_server_tid = MyParentTid();

  clock_request_t req;
  req.type = NOTIFIER;
  while (true) {
    AwaitEvent(EVENT_TIMER);
    Send(clock_server_tid, &req, sizeof(clock_request_t), NULL, 0);
  }
}

/**
 * FIXME: This is all pretty brittle. In particular there is no error checks
 * for the return values of Send/Receive/Reply or heap_push
 * which is important to log if there is a fatal error
 */

void clock_server() {
  int tid = MyTid();
  clock_server_tid = tid;
  int requester;
  unsigned long int ticks = 0;

  clock_request_t request;

  // NOTE: MAX_TASKS + 1 because our heap is dumb, and needs 1 more...
  heapnode_t queue_nodes[MAX_TASKS + 1];
  heap_t delay_queue = heap_create(queue_nodes, MAX_TASKS + 1);

  RegisterAs(NS_CLOCK_SERVER);
  Create(PRIORITY_CLOCK_NOTIFIER, clock_notifier);

  log_clock_server("clock_server initialized", tid);

  while (true) {
    Receive(&requester, &request, sizeof(clock_request_t));

    switch (request.type) {
    case NOTIFIER:
      ReplyN(requester);
      ticks += 1;
      break;
    case TIME_REQUEST:
      log_clock_server("clock_server: time request tid=%d", tid, requester);
      // reply with time
      ReplyS(requester, ticks);
      break;
    case DELAY_REQUEST:
      // Add requester to list of suspended tasks
      log_clock_server("clock_server: delay tid=%d until=%d", tid, requester, ticks + request.time_value);
      heap_push(&delay_queue, ticks + request.time_value, (void *) requester);
      break;
    case DELAY_UNTIL_REQUEST:
      // Add requester to list of suspended tasks
      log_clock_server("clock_server: delay tid=%d until=%d", tid, requester, request.time_value);
      heap_push(&delay_queue, request.time_value, (void *) requester);
      break;
    default:
      KASSERT(false, "Clock server received unknown request.type: type=%d", request.type);
      break;
    }

    log_clock_server("clock_server: heap size=%d top=%d", tid, heap_size(&delay_queue), heap_peek_priority(&delay_queue));

    // Reply to suspended tasks that have timed out
    while (heap_size(&delay_queue) > 0 && heap_peek_priority(&delay_queue) <= ticks) {
      int tid_of_delay_done = (int) heap_pop(&delay_queue);
      log_clock_server("clock_server: undelay tid=%d", tid, tid_of_delay_done);
      ReplyN(tid_of_delay_done);
    }

    log_clock_server("clock_server: time=%d", tid, ticks);
  }
}

int Delay(unsigned int delay ) {
  log_clock_server("Delay delay=%d", active_task->tid, delay);
  if (clock_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    return -1;
  }

  clock_request_t req;
  req.type = DELAY_REQUEST;
  req.time_value = delay;
  Send(clock_server_tid, &req, sizeof(req), NULL, 0);
  return 0;
}

int Time() {
  log_clock_server("Time", active_task->tid);
  if (clock_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    return -1;
  }

  clock_request_t req;
  req.type = TIME_REQUEST;
  volatile unsigned long int time_value;
  SendS(clock_server_tid, req, time_value);
  return time_value;
}

int DelayUntil(unsigned long int until ) {
  log_clock_server("DelayUntil until=%d", active_task->tid, until);
  if (clock_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    return -1;
  }

  clock_request_t req;
  req.type = DELAY_UNTIL_REQUEST;
  req.time_value = until;
  Send(clock_server_tid, &req, sizeof(req), NULL, 0);
  return 0;
}
