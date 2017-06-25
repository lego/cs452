#include <entry/tc1.h>
#include <basic.h>
#include <nameserver.h>
#include <idle_task.h>
#include <clock_server.h>
#include <uart_tx_server.h>
#include <uart_rx_server.h>
#include <interactive.h>
#include <train_controller.h>
#include <priorities.h>

void navigation_test() {

}

void navigation_test_task() {
  Create(PRIORITY_NAMESERVER, &nameserver);
  Create(PRIORITY_CLOCK_SERVER, &clock_server);
  Create(PRIORITY_TX_SERVER, &uart_tx_server);
  Create(PRIORITY_RX_SERVER, &uart_rx_server);
  Create(PRIORITY_IDLE_TASK, &idle_task);

  // Create(PRIORITY_TRAIN_CONTROLLER_SERVER, &train_controller_server);

  Create(11, &navigation_test);
}
