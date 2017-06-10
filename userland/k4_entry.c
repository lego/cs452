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
  //int clock_server_tid = WhoIs(CLOCK_SERVER);

  bwprintf(COM2, "print_task: %d\n\r", MyTid());

  int x = 0;

  //Putc(uart_server_tid, COM2, 'H');

  while (1) {
    uart_request_t req;
    req.type = 1;
    req.channel = COM2;
    req.ch = 'H';
    Send(uart_server_tid, &req, sizeof(req), NULL, 0);
    //Putc(uart_server_tid, COM2, '0' + x);
    //Putc(uart_server_tid, COM2, ' ');
    //Putc(uart_server_tid, COM2, 'H');
    //Putc(uart_server_tid, COM2, 'e');
    //Putc(uart_server_tid, COM2, 'l');
    //Putc(uart_server_tid, COM2, 'l');
    //Putc(uart_server_tid, COM2, 'o');
    //Putc(uart_server_tid, COM2, '\n');
    //Putc(uart_server_tid, COM2, '\r');
    //Delay(clock_server_tid, 100);
    x = (x+1)%10;
  }
}

void k4_entry_task() {
  Create(1, &nameserver);
  //Create(2, &clock_server);
  Create(2, &uart_server);
  Create(IDLE_TASK_PRIORITY, &idle_task);

  Create(4, &print_task);

  int requester;
  uart_request_t request;

  char* str = "Hello\n\r";
  int len = 7;
  int index = 0;

  while(true) {
    Receive(&requester, &request, sizeof(uart_request_t));
    char c = str[index];
    index = (index+1) % len;
    Reply(requester, &c, sizeof(char));
  }
}
