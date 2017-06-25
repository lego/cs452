#include <entry/navigation_test.h>
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
  ExitKernel();
}

void navigation_test_task() {
  Create(10, &navigation_test);
}
