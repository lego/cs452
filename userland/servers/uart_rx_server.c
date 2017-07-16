#include <basic.h>
#include <bwio.h>
#include <kernel.h>
#include <servers/nameserver.h>
#include <servers/uart_rx_server.h>
#include <heap.h>
#include <priorities.h>

static int uart1_rx_server_tid = -1;
static int uart2_rx_server_tid = -1;

enum {
  RX_NOTIFIER,
  GET_REQUEST,
  GET_QUEUE_REQUEST,
  CLEAR_REQUEST,
};

typedef struct {
  int type;
  int channel;
  char ch;
} uart_request_t;

void uart_rx_notifier() {
  int receiver;
  int tid = MyTid();

  int uart_server_tid = MyParentTid();

  uart_request_t req;
  ReceiveS(&receiver, req);
  int channel = req.channel;
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided to uart_rx_notifier: got channel=%d", channel);
  ReplyN(receiver);

  log_uart_server("uart_rx_notifier initialized tid=%d channel=%d", tid, channel);

  req.type = RX_NOTIFIER;
  while (true) {
    log_uart_server("uart_rx_notifer channel=%d", channel);
    switch (channel) {
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

  ReceiveS(&requester, request);
  int channel = request.channel;
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided to uart_tx_server: got channel=%d", channel);
  ReplyN(requester);

  if (channel == COM1) {
    RegisterAs(NS_UART1_RX_SERVER);
  } else if (channel == COM2) {
    RegisterAs(NS_UART2_RX_SERVER);
  }

  char outputQueue[OUTPUT_QUEUE_MAX];
  int outputStart = 0;
  int outputQueueLength = 0;
  int notifier_priority = ((channel == COM1) ? PRIORITY_UART1_RX_NOTIFIER : PRIORITY_UART2_RX_NOTIFIER);
  int notifier_tid = Create(notifier_priority, uart_rx_notifier);
  request.channel = channel;
  Send(notifier_tid, &request, sizeof(request), NULL, 0);
  int waiting_tid = -1;

  log_uart_server("uart_rx_server initialized channel=%d tid=%d", channel, tid);

  while (true) {
    ReceiveS(&requester, request);

    switch (request.type) {
    case RX_NOTIFIER:
      KASSERT(outputQueueLength < OUTPUT_QUEUE_MAX, "UART input server queue has reached its limits for channel %d!", channel);
      int i = (outputStart + outputQueueLength) % OUTPUT_QUEUE_MAX;
      outputQueue[i] = request.ch;
      outputQueueLength += 1;
      ReplyN(requester);
      break;
    case CLEAR_REQUEST:
      outputQueueLength = 0;
      ReplyN(requester);
      break;
    case GET_QUEUE_REQUEST:
      ReplyS(requester, outputQueueLength);
      break;
    case GET_REQUEST:
      c = request.ch;
      KASSERT(waiting_tid == -1, "Already a pending UART request for channel %d!", channel);
      waiting_tid = requester;
      break;
    default:
      KASSERT(false, "uart_server received unknown request type=%d", request.type);
      break;
    }

    if (waiting_tid != -1 && outputQueueLength > 0) {
      char c = outputQueue[outputStart];
      ReplyS(waiting_tid, c);
      outputStart = (outputStart + 1) % OUTPUT_QUEUE_MAX;
      outputQueueLength -= 1;
      waiting_tid = -1;
    }
  }
}

void uart_rx() {
  uart_request_t request;

  uart1_rx_server_tid = Create(PRIORITY_UART1_RX_SERVER, uart_rx_server);
  request.channel = COM1;
  SendSN(uart1_rx_server_tid, request);

  uart2_rx_server_tid = Create(PRIORITY_UART2_RX_SERVER, uart_rx_server);
  request.channel = COM2;
  SendSN(uart2_rx_server_tid, request);
}

char Getc(int channel) {
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
  log_task("Getc tid=%d", active_task->tid, uart_rx_server_tid);
  int server_tid = ((channel == COM1) ? uart1_rx_server_tid : uart2_rx_server_tid);
  if (server_tid == -1) {
    KASSERT(false, "UART rx server not initialized");
    return -1;
  }
  uart_request_t req;
  req.type = GET_REQUEST;
  req.channel = channel;
  char result __attribute__((aligned(4)));
  SendS(server_tid, req, result);
  return result;
}

int ClearRx(int channel) {
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
  log_task("ClearRx tid=%d", active_task->tid, uart_rx_server_tid);
  int server_tid = ((channel == COM1) ? uart1_rx_server_tid : uart2_rx_server_tid);
  if (server_tid == -1) {
    KASSERT(false, "UART rx server not initialized");
    return -1;
  }
  uart_request_t req;
  req.type = CLEAR_REQUEST;
  req.channel = channel;
  SendSN(server_tid, req);
  return 0;
}

int GetRxQueueLength(int channel) {
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
  log_task("Getc tid=%d", active_task->tid, uart_rx_server_tid);
  int server_tid = ((channel == COM1) ? uart1_rx_server_tid : uart2_rx_server_tid);
  if (server_tid == -1) {
    KASSERT(false, "UART rx server not initialized");
    return -1;
  }
  uart_request_t req;
  req.type = GET_QUEUE_REQUEST;
  req.channel = channel;
  int result;
  SendS(server_tid, req, result);
  return result;
}
