#include <bwio.h>
#include <kernel.h>
#include <servers/nameserver.h>
#include <idle_task.h>
#include <trains/navigation.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <servers/uart_rx_server.h>
#include <interactive.h>
#include <interactive/command_parser.h>
#include <interactive/command_interpreter.h>
#include <train_controller.h>
#include <trains/switch_controller.h>
#include <trains/executor.h>
#include <trains/sensor_collector.h>
#include <track/pathing.h>
#include <trains/reservoir.h>
#include <priorities.h>

// from interactive
void sensor_saver();

void train_control_entry_task() {
  Create(PRIORITY_NAMESERVER, nameserver);
  Create(PRIORITY_CLOCK_SERVER, clock_server);
  // FIXME: priority
  Create(0, uart_tx);
  // FIXME: priority
  Create(0, uart_rx);
  Create(PRIORITY_IDLE_TASK, idle_task);

  InitPathing();
  InitNavigation();

  // FIXME: priority
  Create(4, reservoir_task);

  // FIXME: priority
  Create(3, executor_task);
  Create(PRIORITY_TRAIN_CONTROLLER_SERVER, train_controller_server);
  Create(PRIORITY_SWITCH_CONTROLLER, switch_controller);

  Create(PRIORITY_UART1_RX_SERVER, sensor_saver);
  // FIXME: priority
  Create(PRIORITY_UART1_RX_SERVER+1, sensor_collector_task);

  Create(PRIORITY_INTERACTIVE, interactive);
  // FIXME: priority
  Create(6, command_interpreter_task);
  // FIXME: priority
  Create(7, command_parser_task);

}
