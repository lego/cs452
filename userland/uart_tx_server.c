#include <basic.h>
#include <kernel.h>
#include <uart_tx_server.h>
#include <kernel.h>
#include <nameserver.h>
#include <heap.h>
#include <bwio.h>
#include <priorities.h>

static int uart1_tx_server_tid = -1;
static int uart2_tx_server_tid = -1;

enum {
  TX_NOTIFIER,
  PUT_REQUEST,
};

typedef struct {
  int type;
  int channel;
  char *ch;
  int len;
} uart_request_t;

void uart_tx_notifier() {
  char ch;
  int receiver;
  int tid = MyTid();

  int uart_server_tid = MyParentTid();

  uart_request_t req;
  ReceiveS(&receiver, req);
  int channel = req.channel;
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided to uart_tx_notifier: got channel=%d", channel);
  KASSERT(receiver == uart_server_tid, "uart_tx_notifier receive message not from uart_server (parent)");
  ReplyN(receiver);

  log_uart_server("uart_tx_notifier initialized tid=%d channel=%d", tid, channel);

  volatile int volatile *flags = (int *)( UART1_BASE + UART_FLAG_OFFSET );

  req.type = TX_NOTIFIER;
  while (true) {
    SendS(uart_server_tid, req, ch);
    log_uart_server("uart_notifer channel=%d", channel);
    switch(channel) {
      case COM1:
        AwaitEventPut(EVENT_UART1_TX, ch);
        // VMEM(UART1_BASE + UART_DATA_OFFSET) = ch;
        log_uart_server("uart_notifer COM1 putc=%c", ch);
        // TODO: replace this with interrupt based approach
        int i;
        const int max = 1000000;
        int low = false;
        for (i = 0; i < max && (!low || !(*flags & CTS_MASK)); i++) {
          if (!(*flags & CTS_MASK)) {
            low = true;
          }
        }
        //KASSERT(i < max, "Waited for CTS low then high, but it didn't come :( %x", ch);
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
  int requester;
  char c;

  uart_request_t request;

  ReceiveS(&requester, request);
  int channel = request.channel;
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided to uart_tx_server: got channel=%d", channel);
  ReplyN(requester);

  if (channel == COM1) {
    RegisterAs(UART1_TX_SERVER);
  } else if (channel == COM2) {
    RegisterAs(UART2_TX_SERVER);
  }

  char outputQueue[OUTPUT_QUEUE_MAX];
  int outputStart = 0;
  int outputQueueLength = 0;
  int notifier_priority = ((channel == COM1) ? PRIORITY_UART1_TX_NOTIFIER : PRIORITY_UART2_TX_NOTIFIER);
  int notifier_tid = Create(notifier_priority, uart_tx_notifier);
  request.channel = channel;
  Send(notifier_tid, &request, sizeof(request), NULL, 0);
  int ready = false;

  log_uart_server("uart_server initialized channel=%d tid=%d", channel, tid);

  while (true) {
    ReceiveS(&requester, request);

    switch ( request.type ) {
    case TX_NOTIFIER:
      ready = true;
      break;
    case PUT_REQUEST:
      while (true) {
        if ((request.len == -1 && !(*request.ch)) || request.len == 0) {
          break;
        }
        c = *request.ch;
        if (request.len != -1) {
          request.len--;
        }
        request.ch++;
        if (ready && outputQueueLength == 0) {
          ReplyS(notifier_tid, c);
          ready = false;
        } else {
          KASSERT(outputQueueLength < OUTPUT_QUEUE_MAX, "UART output server queue has reached its limits for channel %d!", channel);
          int i = (outputStart+outputQueueLength) % OUTPUT_QUEUE_MAX;
          outputQueue[i] = c;
          outputQueueLength += 1;
        }
      }
      ReplyN(requester);
      break;
    default:
      KASSERT(false, "uart_server received unknown request type=%d", request.type);
      break;
    }

    if (ready && outputQueueLength > 0) {
      char c = outputQueue[outputStart];
      ReplyS(notifier_tid, c);
      outputStart = (outputStart+1) % OUTPUT_QUEUE_MAX;
      outputQueueLength -= 1;
      ready = false;
    }
  }
}

void uart_tx() {
  uart_request_t request;

  uart1_tx_server_tid = Create(PRIORITY_UART1_TX_SERVER, uart_tx_server);
  request.channel = COM1;
  SendSN(uart1_tx_server_tid, request);

  uart2_tx_server_tid = Create(PRIORITY_UART2_TX_SERVER, uart_tx_server);
  request.channel = COM2;
  SendSN(uart2_tx_server_tid, request);
}

int Putcs( int channel, char* c, int len ) {
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
  log_task("Putc c=%c", active_task->tid, c);
  int server_tid = ((channel == COM1) ? uart1_tx_server_tid : uart2_tx_server_tid);
  if (server_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "UART tx server not initialized");
    return -1;
  }

  uart_request_t req;
  req.type = PUT_REQUEST;
  req.channel = channel;
  req.ch = c;
  req.len = len;
  SendSN(server_tid, req);
  return 0;
}

int Putc( int channel, char c ) {
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
  log_task("Putc tid=%d char=%c", active_task->tid, uart_tx_server_tid, c);
  int server_tid = ((channel == COM1) ? uart1_tx_server_tid : uart2_tx_server_tid);
  if (server_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "UART tx server not initialized");
    return -1;
  }
  uart_request_t req;
  req.type = PUT_REQUEST;
  req.channel = channel;
  req.ch = &c;
  req.len = 1;
  SendSN(server_tid, req);
  return 0;
}

int Putstr(int channel, char *str ) {
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
  log_task("Putstr str=%s", active_task->tid, str);
  int server_tid = ((channel == COM1) ? uart1_tx_server_tid : uart2_tx_server_tid);
  if (server_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    KASSERT(false, "UART tx server not initialized");
    return -1;
  }

  uart_request_t req;
  req.type = PUT_REQUEST;
  req.channel = channel;
  req.ch = str;
  req.len = -1;
  SendSN(server_tid, req);
  str++;
  return 0;
}
