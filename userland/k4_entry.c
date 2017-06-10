#include <k4_entry.h>
#include <basic.h>
#include <nameserver.h>
#include <idle_task.h>
#include <clock_server.h>
#include <uart_server.h>

typedef struct {
  int type;
  int channel;
  char ch;
} uart_request_t;

void print_task() {
  int uart_server_tid = WhoIs(UART_SERVER);
  int clock_server_tid = WhoIs(CLOCK_SERVER);

  bwprintf(COM2, "print_task: %d\n\r", MyTid());

  int x = 0;

  //Putc(uart_server_tid, COM2, 'H');

  while (1) {
    Putc(uart_server_tid, COM2, '0' + (x / 1000) % 10);
    Putc(uart_server_tid, COM2, '0' + (x / 100) % 10);
    Putc(uart_server_tid, COM2, '0' + (x / 10) % 10);
    Putc(uart_server_tid, COM2, '0' + x % 10);
    Putc(uart_server_tid, COM2, ' ');
    Putc(uart_server_tid, COM2, 'H');
    Putc(uart_server_tid, COM2, 'e');
    Putc(uart_server_tid, COM2, 'l');
    Putc(uart_server_tid, COM2, 'l');
    Putc(uart_server_tid, COM2, 'o');
    Putc(uart_server_tid, COM2, '\n');
    Putc(uart_server_tid, COM2, '\r');
    Delay(clock_server_tid, 1);
    x++;
  }
}

void k4_entry_task() {
  int requester;
  uart_request_t request;

  char* str = "Hello\n\r";
  int len = 7;
  int index = 0;

  Create(1, &nameserver);
  Create(2, &clock_server);
  Create(2, &uart_server);
  Create(IDLE_TASK_PRIORITY, &idle_task);

  Create(4, &print_task);

  while(true) {
    ReceiveS(&requester, request);
    char c = str[index];
    index = (index+1) % len;
    ReplyS(requester, c);
  }
}
