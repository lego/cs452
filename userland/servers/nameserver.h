#pragma once

#include <kassert.h>

#define REGISTER_CALL 0
#define WHOIS_CALL 1

enum task_name_t {
  NS_PRODUCER_TEST,
  NS_RPS_SERVER,
  NS_CLOCK_SERVER,
  NS_CLOCK_NOTIFIER,
  NS_UART1_TX_SERVER,
  NS_UART2_TX_SERVER,
  NS_UART1_RX_SERVER,
  NS_UART2_RX_SERVER,
  NS_TRAIN_CONTROLLER_SERVER,
  NS_IDLE_TASK,
  NS_SENSOR_SAVER,
  NS_SENSOR_ATTRIBUTER,

  NS_SENSOR_DETECTOR_MULTIPLEXER,
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
int RegisterAs(task_name_t name);
int WhoIs(task_name_t name);

static inline int _WhoIsEnsured(task_name_t name, char *actual_name) {
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
