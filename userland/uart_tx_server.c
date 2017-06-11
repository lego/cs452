#include <basic.h>
#include <kernel.h>
#include <uart_tx_server.h>
#include <kernel.h>
#include <nameserver.h>
#include <heap.h>
#include <bwio.h>

static int uart_tx_server_tid = -1;

enum {
  TX_NOTIFIER,
  PUT_REQUEST,
};

typedef struct {
  int type;
  int channel;
  char ch;
} uart_request_t;

void uart_tx_notifier() {
  char ch;
  int receiver;
  int tid = MyTid();

  RegisterAs(UART_TX_NOTIFIER);
  int uart_server_tid = MyParentTid();

  uart_request_t req;
  ReceiveS(&receiver, req);
  int channel = req.channel;
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided to uart_tx_notifier: got channel=%d", channel);
  KASSERT(receiver == uart_server_tid, "uart_tx_notifier receive message not from uart_server (parent)");
  ReplyN(receiver);

  log_uart_server("uart_tx_notifier initialized tid=%d channel=%d", tid, channel);

  req.type = TX_NOTIFIER;
  while (true) {
    SendS(uart_server_tid, req, ch);
    log_uart_server("uart_notifer channel=%d", channel);
    switch(channel) {
      case COM1:
        AwaitEventPut(EVENT_UART1_TX, ch);
        // VMEM(UART1_BASE + UART_DATA_OFFSET) = ch;
        log_uart_server("uart_notifer COM1 putc=%c", ch);
        break;
      case COM2:
        AwaitEventPut(EVENT_UART2_TX, ch);
        // VMEM(UART2_BASE + UART_DATA_OFFSET) = ch;
        // log_uart_server("uart_notifer COM1 putc=%c", ch);
        break;
    }
  }
}

/**
 * FIXME: This is all pretty brittle. In particular there is no error checks
 * for the return values of Send/Receive/Reply or heap_push
 * which is important to log if there is a fatal error
 */

#define OUTPUT_QUEUE_MAX 2000

void uart_tx_server() {
  int tid = MyTid();
  uart_tx_server_tid = tid;
  int requester;
  char c;

  uart_request_t request;

  RegisterAs(UART_TX_SERVER);

  // uart2 stuff
  char uart2_outputQueue[OUTPUT_QUEUE_MAX];
  int uart2_outputStart = 0;
  int uart2_outputQueueLength = 0;
  int uart2_notifier_tid = Create(1, uart_tx_notifier);
  request.channel = COM2;
  Send(uart2_notifier_tid, &request, sizeof(request), NULL, 0);
  int uart2_ready = false;

  // uart1 stuff
  char uart1_outputQueue[OUTPUT_QUEUE_MAX];
  int uart1_outputStart = 0;
  int uart1_outputQueueLength = 0;
  int uart1_notifier_tid = Create(1, uart_tx_notifier);
  request.channel = COM1;
  Send(uart1_notifier_tid, &request, sizeof(request), NULL, 0);
  int uart1_ready = false;

  log_uart_server("uart_server initialized tid=%d", tid);

  while (true) {
    ReceiveS(&requester, request);

    switch ( request.type ) {
    case TX_NOTIFIER:
      if (request.channel == COM1) {
        uart1_ready = true;
      } else if (request.channel == COM2) {
        uart2_ready = true;
      }
      break;
    case PUT_REQUEST:
      c = request.ch;
      if (request.channel == COM1) {
        if (uart1_ready && uart1_outputQueueLength == 0) {
          ReplyS(uart1_notifier_tid, c);
          uart1_ready = false;
        } else {
          KASSERT(uart1_outputQueueLength < OUTPUT_QUEUE_MAX, "UART1 output server queue has reached its limits! size=%d limit=%d", uart1_outputQueueLength, OUTPUT_QUEUE_MAX);
          int i = (uart1_outputStart+uart1_outputQueueLength) % OUTPUT_QUEUE_MAX;
          uart1_outputQueue[i] = c;
          uart1_outputQueueLength += 1;
        }
      } else if (request.channel == COM2) {
        if (uart2_ready && uart2_outputQueueLength == 0) {
          ReplyS(uart2_notifier_tid, c);
          uart2_ready = false;
        } else {
          KASSERT(uart2_outputQueueLength < OUTPUT_QUEUE_MAX, "UART1 output server queue has reached its limits! size=%d limit=%d", uart2_outputQueueLength, OUTPUT_QUEUE_MAX);
          int i = (uart2_outputStart+uart2_outputQueueLength) % OUTPUT_QUEUE_MAX;
          uart2_outputQueue[i] = c;
          uart2_outputQueueLength += 1;
        }
      }
      ReplyN(requester);
      break;
    default:
      KASSERT(false, "uart_server received unknown request type=%d", request.type);
      bwprintf(COM2, " ERR %d\n\r", request.type);
      break;
    }

    if (uart2_ready && uart2_outputQueueLength > 0) {
      char c = uart2_outputQueue[uart2_outputStart];
      ReplyS(uart2_notifier_tid, c);
      uart2_outputStart = (uart2_outputStart+1) % OUTPUT_QUEUE_MAX;
      uart2_outputQueueLength -= 1;
      uart2_ready = false;
    }

    if (uart1_ready && uart1_outputQueueLength > 0) {
      char c = uart1_outputQueue[uart1_outputStart];
      ReplyS(uart1_notifier_tid, c);
      uart1_outputStart = (uart1_outputStart+1) % OUTPUT_QUEUE_MAX;
      uart1_outputQueueLength -= 1;
      uart1_ready = false;
    }
  }
}

int Putc(int channel, char c ) {
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
  log_task("Putc c=%c", active_task->tid, c);
  if (uart_tx_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "UART tx server not initialized");
    return -1;
  }

  uart_request_t req;
  req.type = PUT_REQUEST;
  req.channel = channel;
  req.ch = c;
  Send(uart_tx_server_tid, &req, sizeof(req), NULL, 0);
  return 0;
}

int Putstr(int channel, char *str ) {
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
  log_task("Putstr str=%s", active_task->tid, str);
  if (uart_tx_server_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "UART tx server not initialized");
    return -1;
  }

  uart_request_t req;
  req.type = PUT_REQUEST;
  req.channel = channel;
  while (*str != '\0') {
    req.ch = *str;
    Send(uart_tx_server_tid, &req, sizeof(req), NULL, 0);
    str++;
  }
  return 0;
}
