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
    Send(uart_server_tid, &req, sizeof(uart_request_t), &ch, sizeof(char));
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
  Create(1, uart_tx_notifier);

  int tx_ready_tid = -1;

  log_uart_server("uart_server initialized", tid);

  char* str = "Hello\n\r";
  int len = 7;
  int index = 0;

  int i;

  while (true) {
    Receive(&requester, &request, sizeof(uart_request_t));

    switch ( request.type ) {
    case TX_NOTIFIER:
      //bwprintf(COM2, "TX_NOTIFIER\n\r");
      log_uart_server("uart_server: NOTIFIER", requester);
      tx_ready_tid = requester;
      break;
    case PUT_REQUEST:
      //bwprintf(COM2, "PUT_REQUEST\n\r");
      if (tx_ready_tid != -1 && outputQueueLength == 0) {
        //char c = str[index];
        //char c = outputQueue[outputStart];
        char c = request.ch;
        //index = (index+1) % len;
        //bwprintf(COM2, "Send character\n\r");
        Reply(tx_ready_tid, &c, sizeof(char));
        //outputStart = (outputStart+1) % OUTPUT_QUEUE_MAX;
        //outputQueueLength -= 1;
        tx_ready_tid = -1;
      } else {
        KASSERT(outputQueueLength < OUTPUT_QUEUE_MAX, "UART output server queue has reached its limits!", tid);
        i = (outputStart+outputQueueLength) % OUTPUT_QUEUE_MAX;
        outputQueue[i] = request.ch;
        outputQueueLength += 1;
      }
      //bwprintf(COM2, "PUT_REQUEST\n\r");
      //index = (outputStart+outputQueueLength) % OUTPUT_QUEUE_MAX;
      //outputQueue[index] = request.ch;
      ReplyN(requester);
      break;
    default:
      // FIXME: Uart server receive request.type unknown.
      assert(false);
      break;
    }
    //bwprintf(COM2, "After switch\n\r");

    if (tx_ready_tid >= 0 && outputQueueLength > 0) {
      //char c = str[index];
      char c = outputQueue[outputStart];
      //index = (index+1) % len;
      //bwprintf(COM2, "Send character\n\r");
      Reply(tx_ready_tid, &c, sizeof(char));
      outputStart = (outputStart+1) % OUTPUT_QUEUE_MAX;
      outputQueueLength -= 1;
      tx_ready_tid = -1;
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
