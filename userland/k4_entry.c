#include <k4_entry.h>
#include <basic.h>
#include <nameserver.h>
#include <idle_task.h>
#include <clock_server.h>
#include <uart_tx_server.h>
#include <uart_rx_server.h>
#include <interactive.h>

typedef struct {
  int type;
  int channel;
  char ch;
} uart_request_t;

void print_task() {
  int x = 0;

  int train = 69;

  //Putc(COM2, 'H');

  char buf[2];
  buf[0] = 0;
  buf[1] = train;

  while (1) {
    char c = Getc(COM2);
    if (c == 's') {
      Putc(COM2, '!');
      Putstr(COM2, "STOP!\n\r");
      buf[0] =  0; Putcs(COM1, buf, 2);
    } else if (c == 'g') {
      Putstr(COM2, "GO!\n\r");
      buf[0] = 14; Putcs(COM1, buf, 2);
    } else if (c == 'r') {
      Putstr(COM2, "REVERSE!\n\r");
      buf[0] = 0; Putcs(COM1, buf, 2);
      Delay(300);
      buf[0] = 15; Putcs(COM1, buf, 2);
      Delay(5);
      buf[0] = 14; Putcs(COM1, buf, 2);
    }

    //Putc(COM2, c);
    //Putc(COM2, ' ');

    //Putc(COM2, '0' + x);
    //Putc(COM2, ' ');
    //Putc(COM2, 'R');
    //Putc(COM2, 'u');
    //Putc(COM2, 'n');
    //Putc(COM2, 'n');
    //Putc(COM2, 'i');
    //Putc(COM2, 'n');
    //Putc(COM2, 'g');
    //Putc(COM2, '\n');
    //Putc(COM2, '\r');

    //Putc(COM1, '0' + x);
    //Putc(COM1, ' ');
    //Putc(COM1, 'H');
    //Putc(COM1, 'e');
    //Putc(COM1, 'l');
    //Putc(COM1, 'l');
    //Putc(COM1, 'o');
    //Putc(COM1, '\n');
    //Putc(COM1, '\r');

    Delay(5);
    x = (x+1)%10;
  }
}

void train_control_task() {
  // Putstr(COM2, "Starting set\n\r");
  Putc(COM1, 0x60);

  Delay(500);
  // Putstr(COM2, "Train 70 -> 14\n\r");
  Putc(COM1, 14);
  Putc(COM1, 70);

  Delay(500);
  // Putstr(COM2, "Train 70 -> 0\n\r");
  Putc(COM1, 0);
  Putc(COM1, 70);

  Delay(500);
  bwprintf(COM2, "Exiting\n\r");
  ExitKernel();
}

void k4_entry_task() {
  Create(1, &nameserver);
  Create(2, &clock_server);
  Create(2, &uart_tx_server);
  Create(2, &uart_rx_server);
  Create(IDLE_TASK_PRIORITY, &idle_task);

  Create(10, &interactive);
}
