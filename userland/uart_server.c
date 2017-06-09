#include <basic.h>
#include <kernel.h>
#include <uart_server.h>
#include <kernel.h>
#include <nameserver.h>
#include <heap.h>
#include <bwio.h>

enum {
  TX_NOTIFIER,
  TIME_REQUEST,
  DELAY_REQUEST,
  DELAY_UNTIL_REQUEST,
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

  uart_request_t req;
  req.type = TX_NOTIFIER;
  volatile int volatile *flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
  while (true) {
    //Send(uart_server_tid, &req, sizeof(uart_request_t), &ch, sizeof(char));
    AwaitEvent(EVENT_UART2_TX);
    //while( ( *flags & TXFF_MASK ) ) ;
    *(int*)(UART2_BASE+UART_DATA_OFFSET) = 'B';
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

  char outputQueue[OUTPUT_QUEUE_MAX];
  int outputStart = 0;
  int outputQueueLength = 0;

  RegisterAs(UART_SERVER);
  Create(1, uart_tx_notifier);

  int tx_ready_tid = -1;

  log_uart_server("uart_server initialized", tid);

  while (true) {
    Receive(&requester, &request, sizeof(uart_request_t));

    switch ( request.type ) {
    case TX_NOTIFIER:
      //bwprintf(COM2, "TX_NOTIFIER done!\n\r");
      log_uart_server("uart_server: NOTIFIER", requester);
      tx_ready_tid = requester;
      break;
    case PUT_REQUEST:
      //if (tx_ready_tid >= 0 && outputQueueLength == 0) {
      //  Reply(tx_ready_tid, &request.ch, sizeof(char));
      //  tx_ready_tid = -1;
      //} else {
      {
        int index = (outputStart+outputQueueLength) % OUTPUT_QUEUE_MAX;
        outputQueue[index] = request.ch;
        outputQueueLength += 1;
        //bwprintf(COM2, "\n\r PUT outputQueue[%d]: %c %d \n\r", index, outputQueue[index], outputQueueLength);
      }
      ReplyN(requester);
      break;
    default:
      // FIXME: Uart server receive request.type unknown.
      assert(false);
      break;
    }

    if (tx_ready_tid >= 0 && outputQueueLength > 0) {
      //If (outputQueue[outputStart] == 'H') {
      //  if (outputStart < 10) {
      //    bwprintf(COM2, "%d   ", outputStart);
      //  } else if (outputStart < 100) {
      //    bwprintf(COM2, "%d  ", outputStart);
      //  } else {
      //    bwprintf(COM2, "%d ", outputStart);
      //  }
      //}
      Reply(tx_ready_tid, &outputQueue[outputStart], sizeof(char));
      //bwprintf(COM2, "\n\r GET outputQueue[%d]: %c, length %d \n\r", outputStart, outputQueue[outputStart], outputQueueLength);
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
