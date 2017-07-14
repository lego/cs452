#pragma once

#include <kassert.h>

#define REGISTER_CALL 0
#define WHOIS_CALL 1

enum task_name_t {
  PRODUCER_TEST,
  RPS_SERVER,
  CLOCK_SERVER,
  CLOCK_NOTIFIER,
  UART1_TX_SERVER,
  UART2_TX_SERVER,
  UART1_RX_SERVER,
  UART2_RX_SERVER,
  TRAIN_CONTROLLER_SERVER,
  IDLE_TASK,
  SENSOR_SAVER,

  NS_EXECUTOR,
  NS_COMMAND_PARSER,
  NS_COMMAND_INTERPRETER,
  NS_INTERACTIVE,

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


static inline int _WhoIsEnsured( task_name_t name, char * actual_name) {
  int result = WhoIs(name);
  KASSERT(result >= 0, "Expected WhoIs lookup did not succeed. attempted to lookup %s", actual_name);
  return result;
}

/**
 * Ensured WhoIs, which KASSERTs on failure
 * This is ideal for when we expect a starting order, else we it's a
 * programming error
 */
#define WhoIsEnsured(name) _WhoIsEnsured(name, #name)
