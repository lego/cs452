#include <k4_entry.h>
#include <basic.h>
#include <nameserver.h>
#include <idle_task.h>
#include <clock_server.h>
#include <uart_tx_server.h>
#include <uart_rx_server.h>

typedef struct {
  int type;
  int channel;
  char ch;
} uart_request_t;

void print_task() {
  int uart_server_tid = WhoIs(UART_TX_SERVER);
  int uart_server_tid_rx = WhoIs(UART_RX_SERVER);
  int clock_server_tid = WhoIs(CLOCK_SERVER);

  bwprintf(COM2, "print_task: %d\n\r", MyTid());

  int x = 0;

  //Putc(uart_server_tid, COM2, 'H');

  while (1) {
    char c = Getc(uart_server_tid_rx, COM2);
    Putc(uart_server_tid, COM2, c);
    Putc(uart_server_tid, COM2, ' ');
    Putc(uart_server_tid, COM2, '0' + x);
    Putc(uart_server_tid, COM2, ' ');
    Putc(uart_server_tid, COM2, 'H');
    Putc(uart_server_tid, COM2, 'e');
    Putc(uart_server_tid, COM2, 'l');
    Putc(uart_server_tid, COM2, 'l');
    Putc(uart_server_tid, COM2, 'o');
    Putc(uart_server_tid, COM2, '\n');
    Putc(uart_server_tid, COM2, '\r');
    Delay(clock_server_tid, 1);
    x = (x+1)%10;
  }
}

void train_control_task() {
  int uart_server_tid = WhoIs(UART_TX_SERVER);
  int clock_server_tid = WhoIs(CLOCK_SERVER);

  // Putstr(uart_server_tid, COM2, "Starting set\n\r");
  Putc(uart_server_tid, COM1, 0x60);

  Delay(clock_server_tid, 500);
  // Putstr(uart_server_tid, COM2, "Train 70 -> 14\n\r");
  Putc(uart_server_tid, COM1, 14);
  Putc(uart_server_tid, COM1, 70);

  Delay(clock_server_tid, 500);
  // Putstr(uart_server_tid, COM2, "Train 70 -> 0\n\r");
  Putc(uart_server_tid, COM1, 0);
  Putc(uart_server_tid, COM1, 70);

  Delay(clock_server_tid, 500);
  bwprintf(COM2, "Exiting\n\r");
  ExitKernel();
}

void k4_entry_task() {
  Create(1, &nameserver);
  Create(2, &clock_server);
  Create(2, &uart_tx_server);
  Create(2, &uart_rx_server);
  Create(IDLE_TASK_PRIORITY, &idle_task);

  Create(4, &print_task);
}
