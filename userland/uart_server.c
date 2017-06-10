#include <basic.h>
#include <cbuffer.h>
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
    Send(uart_server_tid, &req, sizeof(uart_request_t), &ch, sizeof(char));
    AwaitEvent(EVENT_UART2_TX);
    // while( ( *flags & TXFF_MASK ) ) ;
    // Write the data
    *(int*)(UART2_BASE+UART_DATA_OFFSET) = ch;
  }
}

/**
 * FIXME: This is all pretty brittle. In particular there is no error checks
 * for the return values of Send/Receive/Reply or heap_push
 * which is important to log if there is a fatal error
 */

#define OUTPUT_QUEUE_MAX 1000

void uart_server() {
  int tid = MyTid();
  int requester;
  uart_request_t request;
  int status;

  // Create circular buffer
  cbuffer_t output_buf;
  char tmp_buf[OUTPUT_QUEUE_MAX];
  cbuffer_init(&output_buf, tmp_buf, OUTPUT_QUEUE_MAX);

  RegisterAs(UART_SERVER);

  int uart_notifier = Create(1, uart_tx_notifier);
  bool xnot_ready = false;
  log_uart_server("uart_server initialized", tid);

  while (true) {
    ReceiveS(&requester, request);

    switch ( request.type ) {
    case TX_NOTIFIER:
      log_uart_server("uart_server: NOTIFIER requester=%d", tid, requester);
      xnot_ready = true;
      break;
    case PUT_REQUEST:
      log_uart_server("uart_server: PUT_REQUEST requester=%d", tid, requester);
      if (xnot_ready && cbuffer_empty(&output_buf)) {
        char c = request.ch;
        log_uart_server("uart_server: xnot ready, not queuing c=%c", tid, c);
        ReplyS(uart_notifier, c);
        xnot_ready = false;
      } else {
        status = cbuffer_add(&output_buf, (void *) request.ch);
        KASSERT(status == 0, "cbuffer_add error, likely full");
        log_uart_server("uart_server: xnot not ready, queuing c=%c", tid, request.ch);
      }
      ReplyN(requester);
      break;
    default:
      KASSERT(false, "UART server received unknown request type");
      break;
    }


    if (xnot_ready && !cbuffer_empty(&output_buf)) {
      char c = (char) cbuffer_pop(&output_buf, &status);
      KASSERT(status == 0, "cbuffer_pop error, likely empty");
      log_uart_server("uart_server: xnot now ready and sending char: c=%c queue_length=%d", tid, c, outputQueueLength);
      ReplyS(uart_notifier, c);
      xnot_ready = false;
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
