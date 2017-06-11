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
  int uart_server_tid = WhoIs(UART_TX_SERVER);
  int uart_server_tid_rx = WhoIs(UART_RX_SERVER);
  int clock_server_tid = WhoIs(CLOCK_SERVER);

  int x = 0;

  int train = 69;

  //Putc(uart_server_tid, COM2, 'H');

  char buf[2];
  buf[0] = 0;
  buf[1] = train;

  while (1) {
    char c = Getc(uart_server_tid_rx, COM2);
    if (c == 's') {
      Putc(uart_server_tid, COM2, '!');
      Putstr(uart_server_tid, COM2, "STOP!\n\r");
      buf[0] =  0; Putcs(uart_server_tid, COM1, &buf, 2);
    } else if (c == 'g') {
      Putstr(uart_server_tid, COM2, "GO!\n\r");
      buf[0] = 14; Putcs(uart_server_tid, COM1, &buf, 2);
    } else if (c == 'r') {
      Putstr(uart_server_tid, COM2, "REVERSE!\n\r");
      buf[0] = 0; Putcs(uart_server_tid, COM1, &buf, 2);
      Delay(clock_server_tid, 300);
      buf[0] = 15; Putcs(uart_server_tid, COM1, &buf, 2);
      Delay(clock_server_tid, 5);
      buf[0] = 14; Putcs(uart_server_tid, COM1, &buf, 2);
    }

    //Putc(uart_server_tid, COM2, c);
    //Putc(uart_server_tid, COM2, ' ');

    //Putc(uart_server_tid, COM2, '0' + x);
    //Putc(uart_server_tid, COM2, ' ');
    //Putc(uart_server_tid, COM2, 'R');
    //Putc(uart_server_tid, COM2, 'u');
    //Putc(uart_server_tid, COM2, 'n');
    //Putc(uart_server_tid, COM2, 'n');
    //Putc(uart_server_tid, COM2, 'i');
    //Putc(uart_server_tid, COM2, 'n');
    //Putc(uart_server_tid, COM2, 'g');
    //Putc(uart_server_tid, COM2, '\n');
    //Putc(uart_server_tid, COM2, '\r');

    //Putc(uart_server_tid, COM1, '0' + x);
    //Putc(uart_server_tid, COM1, ' ');
    //Putc(uart_server_tid, COM1, 'H');
    //Putc(uart_server_tid, COM1, 'e');
    //Putc(uart_server_tid, COM1, 'l');
    //Putc(uart_server_tid, COM1, 'l');
    //Putc(uart_server_tid, COM1, 'o');
    //Putc(uart_server_tid, COM1, '\n');
    //Putc(uart_server_tid, COM1, '\r');

    Delay(clock_server_tid, 5);
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
