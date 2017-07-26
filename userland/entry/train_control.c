#include <bwio.h>
#include <kernel.h>
#include <servers/nameserver.h>
#include <idle_task.h>
#include <trains/navigation.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <servers/uart_rx_server.h>
#include <detective/sensor_detector_multiplexer.h>
#include <interactive.h>
#include <interactive/command_parser.h>
#include <interactive/command_interpreter.h>
#include <train_command_server.h>
#include <trains/switch_controller.h>
#include <trains/executor.h>
#include <trains/sensor_collector.h>
#include <track/pathing.h>
#include <trains/reservoir.h>
#include <priorities.h>
#include <trains/train_controller.h>

void train_control_entry_task() {
  InitPathing();
  InitNavigation();
  InitTrainControllers();

  Create(PRIORITY_NAMESERVER, nameserver);
  Create(PRIORITY_CLOCK_SERVER, clock_server);
  // FIXME: priority
  Create(0, uart_tx);
  // FIXME: priority
  Create(0, uart_rx);
  Create(PRIORITY_IDLE_TASK, idle_task);

  // FIXME: priority
  Create(4, reservoir_task);

  // FIXME: priority
  Create(4, sensor_detector_multiplexer_task);

  // FIXME: priority
  Create(3, executor_task);
  Create(PRIORITY_TRAIN_COMMAND_SERVER, train_command_server);
  Create(PRIORITY_SWITCH_CONTROLLER, switch_controller);

  // FIXME: priority
  Create(PRIORITY_SWITCH_CONTROLLER+1, sensor_attributer);
  Create(PRIORITY_SWITCH_CONTROLLER+2, sensor_collector_task);
  // Create(PRIORITY_SWITCH_CONTROLLER+2, fake_sensor_collector_task);

  Create(PRIORITY_INTERACTIVE, interactive);
  // FIXME: priority
  Create(6, command_interpreter_task);
  // FIXME: priority
  Create(7, command_parser_task);
}
