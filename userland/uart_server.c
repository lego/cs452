#include <basic.h>
#include <kernel.h>
#include <uart_server.h>
#include <kernel.h>
#include <nameserver.h>
#include <heap.h>
#include <bwio.h>

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

  RegisterAs(UART_NOTIFIER);
  int uart_server_tid = MyParentTid();

  uart_request_t req;
  ReceiveS(&receiver, req);
  KASSERT(receiver != uart_server_tid, "uart_tx_notifier receive message not from uart_server (parent)");
  ReplyN(receiver);
  int channel = req.channel;

  log_uart_server("uart_tx_notifier initialized", tid);

  req.type = TX_NOTIFIER;
  while (true) {
    SendS(uart_server_tid, req, ch);
    if (channel == COM1) {
      AwaitEvent(EVENT_UART2_TX);
      *(int*)(UART2_BASE+UART_DATA_OFFSET) = ch;
    } else if (channel == COM2) {
      AwaitEvent(EVENT_UART2_TX);
      *(int*)(UART2_BASE+UART_DATA_OFFSET) = ch;
    } else {
      KASSERT(false, "Invalid channel provided to uart_tx_notifier");
    }
  }
}

/**
 * FIXME: This is all pretty brittle. In particular there is no error checks
 * for the return values of Send/Receive/Reply or heap_push
 * which is important to log if there is a fatal error
 */

#define OUTPUT_QUEUE_MAX 2000

void uart_server() {
  int tid = MyTid();
  int requester;

  uart_request_t request;

  char outputQueue[OUTPUT_QUEUE_MAX];
  int uart2_outputStart = 0;
  int uart2_outputQueueLength = 0;

  RegisterAs(UART_SERVER);

  int uart2_notifier_tid = Create(1, uart_tx_notifier);
  request.channel = COM2;
  Send(uart2_notifier_tid, &request, sizeof(request), NULL, 0);
  int uart2_ready = false;


  log_uart_server("uart_server initialized", tid);

  while (true) {
    ReceiveS(&requester, request);

    switch ( request.type ) {
    case TX_NOTIFIER:
      log_uart_server("uart_server: NOTIFIER", tid);
      uart2_ready = true;
      break;
    case PUT_REQUEST:
      log_uart_server("uart_server: NOTIFIER", tid);
      if (uart2_ready && uart2_outputQueueLength == 0) {
        char c = request.ch;
        ReplyS(uart2_notifier_tid, c);
        uart2_ready = false;
      } else {
        KASSERT(uart2_outputQueueLength < OUTPUT_QUEUE_MAX, "UART output server queue has reached its limits!", tid);
        int i = (uart2_outputStart+uart2_outputQueueLength) % OUTPUT_QUEUE_MAX;
        uart2_outputQueue[i] = request.ch;
        uart2_outputQueueLength += 1;
      }
      ReplyN(requester);
      break;
    default:
      KASSERT(false, "uart_server received unknown request type");
      break;
    }

    if (uart2_ready && uart2_outputQueueLength > 0) {
      char c = uart2_outputQueue[uart2_outputStart];
      ReplyS(uart2_notifier_tid, c);
      uart2_outputStart = (uart2_outputStart+1) % OUTPUT_QUEUE_MAX;
      uart2_outputQueueLength -= 1;
      uart2_ready = false;
    }
  }
}

int Putc( int tid, int channel, char c ) {
  log_uart_server("Putc tid=%d", active_task->tid, tid);
  uart_request_t req;
  req.type = PUT_REQUEST;
  req.channel = channel;
  req.ch = c;
  Send(tid, &req, sizeof(req), NULL, 0);
  return 0;
}
