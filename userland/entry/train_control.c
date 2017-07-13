#include <bwio.h>
#include <kernel.h>
#include <servers/nameserver.h>
#include <idle_task.h>
#include <trains/navigation.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <servers/uart_rx_server.h>
#include <interactive.h>
#include <train_controller.h>
#include <trains/sensor_collector.h>
#include <priorities.h>

// from interactive
void sensor_saver();

void train_control_entry_task() {
  Create(PRIORITY_NAMESERVER, nameserver);
  Create(PRIORITY_CLOCK_SERVER, clock_server);
  Create(0, uart_tx);
  Create(0, uart_rx);
  Create(PRIORITY_IDLE_TASK, idle_task);

  InitNavigation();

  Create(PRIORITY_TRAIN_CONTROLLER_SERVER, train_controller_server);

  Create(PRIORITY_UART1_RX_SERVER, sensor_saver);
  Create(PRIORITY_UART1_RX_SERVER+1, sensor_collector_task);

  Create(PRIORITY_INTERACTIVE, interactive);
}
