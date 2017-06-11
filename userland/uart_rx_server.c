#include <basic.h>
#include <kernel.h>
#include <uart_rx_server.h>
#include <kernel.h>
#include <nameserver.h>
#include <heap.h>
#include <bwio.h>

enum {
  RX_NOTIFIER,
  GET_REQUEST,
};

typedef struct {
  int type;
  int channel;
  char ch;
} uart_request_t;

void uart_rx_notifier() {
  int receiver;
  int tid = MyTid();

  RegisterAs(UART_RX_NOTIFIER);
  int uart_server_tid = MyParentTid();

  uart_request_t req;
  ReceiveS(&receiver, req);
  int channel = req.channel;
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided to uart_rx_notifier: got channel=%d", channel);
  KASSERT(receiver == uart_server_tid, "uart_rx_notifier receive message not from uart_server (parent)");
  ReplyN(receiver);

  log_uart_server("uart_rx_notifier initialized tid=%d channel=%d", tid, channel);

  req.type = RX_NOTIFIER;
  while (true) {
    log_uart_server("uart_rx_notifer channel=%d", channel);
    switch(channel) {
      case COM1:
        AwaitEvent(EVENT_UART1_RX);
        req.ch = VMEM(UART1_BASE + UART_DATA_OFFSET);
        log_uart_server("uart_rx_notifer COM1 getc=%c", req.ch);
        break;
      case COM2:
        AwaitEvent(EVENT_UART2_RX);
        req.ch = VMEM(UART2_BASE + UART_DATA_OFFSET);
        log_uart_server("uart_rx_notifer COM2 getc=%c", req.ch);
        break;
    }
    Send(uart_server_tid, &req, sizeof(uart_request_t), NULL, 0);
  }
}

/**
 * FIXME: This is all pretty brittle. In particular there is no error checks
 * for the return values of Send/Receive/Reply or heap_push
 * which is important to log if there is a fatal error
 */

#define OUTPUT_QUEUE_MAX 2000

void uart_rx_server() {
  int tid = MyTid();
  int requester;
  char c;

  uart_request_t request;

  RegisterAs(UART_RX_SERVER);

  // uart2 stuff
  char uart2_outputQueue[OUTPUT_QUEUE_MAX];
  int uart2_outputStart = 0;
  int uart2_outputQueueLength = 0;
  int uart2_notifier_tid = Create(1, uart_rx_notifier);
  request.channel = COM2;
  Send(uart2_notifier_tid, &request, sizeof(request), NULL, 0);
  int uart2_waiting_tid = -1;

  // uart1 stuff
  char uart1_outputQueue[OUTPUT_QUEUE_MAX];
  int uart1_outputStart = 0;
  int uart1_outputQueueLength = 0;
  int uart1_notifier_tid = Create(1, uart_rx_notifier);
  request.channel = COM1;
  Send(uart1_notifier_tid, &request, sizeof(request), NULL, 0);
  int uart1_waiting_tid = -1;

  log_uart_server("uart_rx_server initialized tid=%d", tid);

  while (true) {
    ReceiveS(&requester, request);

    switch ( request.type ) {
    case RX_NOTIFIER:
      if (request.channel == COM1) {
        KASSERT(uart1_outputQueueLength < OUTPUT_QUEUE_MAX, "UART input server queue has reached its limits!");
        int i = (uart1_outputStart+uart1_outputQueueLength) % OUTPUT_QUEUE_MAX;
        uart1_outputQueue[i] = request.ch;
        uart1_outputQueueLength += 1;
      } else if (request.channel == COM2) {
        KASSERT(uart2_outputQueueLength < OUTPUT_QUEUE_MAX, "UART input server queue has reached its limits!");
        int i = (uart2_outputStart+uart2_outputQueueLength) % OUTPUT_QUEUE_MAX;
        uart2_outputQueue[i] = request.ch;
        uart2_outputQueueLength += 1;
      }
      ReplyN(requester);
      break;
    case GET_REQUEST:
      c = request.ch;
      if (request.channel == COM1) {
        KASSERT(uart1_waiting_tid == -1, "Already a pending UART1 request!");
        uart1_waiting_tid = requester;
      } else if (request.channel == COM2) {
        KASSERT(uart1_waiting_tid == -1, "Already a pending UART2 request!");
        uart2_waiting_tid = requester;
      }
      break;
    default:
      KASSERT(false, "uart_server received unknown request type=%d", request.type);
      break;
    }

    if (uart2_waiting_tid != -1 && uart2_outputQueueLength > 0) {
      char c = uart2_outputQueue[uart2_outputStart];
      ReplyS(uart2_waiting_tid, c);
      uart2_outputStart = (uart2_outputStart+1) % OUTPUT_QUEUE_MAX;
      uart2_outputQueueLength -= 1;
      uart2_waiting_tid = -1;
    }

    if (uart1_waiting_tid != -1 && uart1_outputQueueLength > 0) {
      char c = uart1_outputQueue[uart1_outputStart];
      ReplyS(uart1_waiting_tid, c);
      uart1_outputStart = (uart1_outputStart+1) % OUTPUT_QUEUE_MAX;
      uart1_outputQueueLength -= 1;
      uart1_waiting_tid = -1;
    }
  }
}

char Getc( int tid, int channel ) {
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
  log_task("Getc tid=%d", active_task->tid, tid);
  uart_request_t req;
  req.type = GET_REQUEST;
  req.channel = channel;
  char result;
  Send(tid, &req, sizeof(req), &result, sizeof(char));
  return result;
}

//int Putstr( int tid, int channel, char *str ) {
//  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
//  log_task("Putstr tid=%d str=%s", active_task->tid, tid, str);
//  uart_request_t req;
//  req.type = PUT_REQUEST;
//  req.channel = channel;
//  while (*str != NULL) {
//    req.ch = *str;
//    Send(tid, &req, sizeof(req), NULL, 0);
//    str++;
//  }
//  return 0;
//}
