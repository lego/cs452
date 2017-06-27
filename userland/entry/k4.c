#include <entry/k4.h>
#include <basic.h>
#include <bwio.h>
#include <nameserver.h>
#include <idle_task.h>
#include <clock_server.h>
#include <uart_tx_server.h>
#include <uart_rx_server.h>
#include <interactive.h>
#include <train_controller.h>
#include <priorities.h>

void print_task() {
  int train = 58;
  int sw = 16;

  Putstr(COM2, "Enter Character:\n\r");
  while (1) {
    char c = Getc(COM2);
    if (c == 's') {
      Putstr(COM2, "Stop!\n\r");
      SetTrainSpeed(train, 0);
    } else if (c == 'g') {
      Putstr(COM2, "Go!\n\r");
      SetTrainSpeed(train, 14);
    } else if (c == 'r') {
      Putstr(COM2, "Reverse!\n\r");
      ReverseTrain(train, 14);
    } else if (c == 'o') {
      Putstr(COM2, "Switch Curved!\n\r");
      SetSwitch(sw, SWITCH_CURVED);
    } else if (c == 'p') {
      Putstr(COM2, "Switch Straight!\n\r");
      SetSwitch(sw, SWITCH_STRAIGHT);
    }
  }
}

void k4_entry_task() {
  Create(PRIORITY_NAMESERVER, &nameserver);
  Create(PRIORITY_CLOCK_SERVER, &clock_server);
  Create(0, &uart_tx);
  Create(0, &uart_rx);
  Create(PRIORITY_IDLE_TASK, &idle_task);

  Create(PRIORITY_TRAIN_CONTROLLER_SERVER, &train_controller_server);

  Create(10, &interactive);
  //Create(11, &print_task);
}
