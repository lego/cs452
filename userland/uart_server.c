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
  int tid = MyTid();
  RegisterAs(UART_NOTIFIER);
  log_uart_server("uart_tx_notifier initialized", tid);
  int uart_server_tid = MyParentTid();
  char ch;
  int i;

  uart_request_t req;
  req.type = TX_NOTIFIER;
  volatile int volatile *flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
  while (true) {
    volatile int i = 0;
    SendS(uart_server_tid, req, ch);
    AwaitEvent(EVENT_UART2_TX);
    //while( ( *flags & TXFF_MASK ) ) ;
    // Write the data
    *(int*)(UART2_BASE+UART_DATA_OFFSET) = ch;
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
  int outputStart = 0;
  int outputQueueLength = 0;

  RegisterAs(UART_SERVER);
  int uart_notifier_tid = Create(1, uart_tx_notifier);
  int uart_ready = false;

  log_uart_server("uart_server initialized", tid);

  while (true) {
    ReceiveS(&requester, request);

    switch ( request.type ) {
    case TX_NOTIFIER:
      log_uart_server("uart_server: NOTIFIER", tid);
      uart_ready = true;
      break;
    case PUT_REQUEST:
      log_uart_server("uart_server: NOTIFIER", tid);
      if (uart_ready && outputQueueLength == 0) {
        char c = request.ch;
        ReplyS(uart_notifier_tid, c);
        uart_ready = false;
      } else {
        KASSERT(outputQueueLength < OUTPUT_QUEUE_MAX, "UART output server queue has reached its limits!", tid);
        int i = (outputStart+outputQueueLength) % OUTPUT_QUEUE_MAX;
        outputQueue[i] = request.ch;
        outputQueueLength += 1;
      }
      ReplyN(requester);
      break;
    default:
      KASSERT(false, "uart_server received unknown request type");
      break;
    }

    if (uart_ready && outputQueueLength > 0) {
      char c = outputQueue[outputStart];
      ReplyS(uart_notifier_tid, c);
      outputStart = (outputStart+1) % OUTPUT_QUEUE_MAX;
      outputQueueLength -= 1;
      uart_ready = false;
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
