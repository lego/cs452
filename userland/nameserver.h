#ifndef __NAMESERVER_H__
#define __NAMESERVER_H__

#define REGISTER_CALL 0
#define WHOIS_CALL 1

enum task_name_t {
  PRODUCER_TEST,
  RPS_SERVER,
  CLOCK_SERVER,
  CLOCK_NOTIFIER,
  UART_SERVER,
  UART_NOTIFIER,
  IDLE_TASK,

  // NOTE: leave this at the end, OR ELSE!
  NUM_TASK_NAMES,
};

typedef int task_name_t;

typedef struct {
  int call_type;
  task_name_t name;
} nameserver_request_t;

void nameserver();

/* Nameserver calls */
int RegisterAs( task_name_t name );
int WhoIs( task_name_t name );

#endif
