#include <k4_entry.h>
#include <basic.h>
#include <nameserver.h>
#include <idle_task.h>
#include <clock_server.h>
#include <uart_tx_server.h>
#include <uart_rx_server.h>
#include <interactive.h>
#include <train_controller.h>

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
  Create(1, &nameserver);
  Create(2, &clock_server);
  Create(2, &uart_tx_server);
  Create(2, &uart_rx_server);
  Create(IDLE_TASK_PRIORITY, &idle_task);

  Create(5, &train_controller_server);

  Create(10, &interactive);
  //Create(11, &print_task);
}
