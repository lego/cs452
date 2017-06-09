#include <k4_entry.h>
#include <basic.h>
#include <nameserver.h>
#include <idle_task.h>
#include <clock_server.h>
#include <uart_server.h>

void print_task() {
  int uart_server_tid = WhoIs(UART_SERVER);
  //int clock_server_tid = WhoIs(CLOCK_SERVER);

  int x = 0;

  //Putc(uart_server_tid, COM2, 'H');

  //while (1) {
  //  Putc(uart_server_tid, COM2, '0' + x);
  //  Putc(uart_server_tid, COM2, ' ');
  //  Putc(uart_server_tid, COM2, 'H');
  //  Putc(uart_server_tid, COM2, 'e');
  //  Putc(uart_server_tid, COM2, 'l');
  //  Putc(uart_server_tid, COM2, 'l');
  //  Putc(uart_server_tid, COM2, 'o');
  //  Putc(uart_server_tid, COM2, '\n');
  //  Putc(uart_server_tid, COM2, '\r');
  //  //Delay(clock_server_tid, 100);
  //  x = (x+1)%10;
  //}
}

void k4_entry_task() {
  Create(1, &nameserver);
  Create(2, &clock_server);
  Create(2, &uart_server);
  Create(IDLE_TASK_PRIORITY, &idle_task);

  Create(4, &print_task);
}
