#include <basic.h>
#include <jstring.h>
#include <util.h>
#include <kernel.h>
#include <servers/uart_tx_server.h>
#include <servers/nameserver.h>
#include <heap.h>
#include <bwio.h>
#include <priorities.h>
#include <jstring.h>
#include <courier.h>
#include <warehouse.h>

static int uart1_tx_notifier_tid = -1;
static int uart2_tx_notifier_tid = -1;

static int uart1_tx_warehouse_tid = -1;
static int uart2_tx_warehouse_tid = -1;

static int uart1_tx_server_tid = -1;
static int uart2_tx_server_tid = -1;

static int logging_warehouse_tid = -1;

void uart_tx_notifier() {
  int tid = MyTid();

  int requester;
  int channel = -1;
  ReceiveS(&requester, channel);
  ReplyN(requester);
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided to uart_tx_notifier: got channel=%d", channel);

  log_uart_server("uart_tx_notifier initialized tid=%d channel=%d", tid, channel);

  uart_packet_t packet;

  while (true) {
    ReceiveS(&requester, packet);
    ReplyN(requester);
    log_uart_server("uart_notifer channel=%d", channel);
    switch(channel) {
      case COM1:
        for (int i = 0; i < packet.len; i++) {
          AwaitEventPut(EVENT_UART1_TX, packet.data[i]);
          log_uart_server("uart_notifer COM1 putc=%c", packet.data[i]);
        }
        break;
      case COM2:
#if NONTERMINAL_OUTPUT
        AwaitEventPut(EVENT_UART2_TX, packet.len);
        AwaitEventPut(EVENT_UART2_TX, packet.type);
#else
        if (packet.type != 1) {
          break;
        }
#endif
        for (int i = 0; i < packet.len; i++) {
          AwaitEventPut(EVENT_UART2_TX, packet.data[i]);
          log_uart_server("uart_notifer COM2 putc=%c", packet.data[i]);
        }
        break;
    }
  }
}

int createNotifier(int channel) {
  int notifier_priority = ((channel == COM1) ? PRIORITY_UART1_TX_NOTIFIER : PRIORITY_UART2_TX_NOTIFIER);
  int notifier_tid = Create(notifier_priority, uart_tx_notifier);
  SendSN(notifier_tid, channel);
  return notifier_tid;
}

/**
 * FIXME: This is all pretty brittle. In particular there is no error checks
 * for the return values of Send/Receive/Reply or heap_push
 * which is important to log if there is a fatal error
 */

#define OUTPUT_QUEUE_MAX 4096

enum {
  PUT_REQUEST,
  GET_QUEUE_REQUEST,
};

typedef struct {
  int type;
  int channel;
  const char *ch;
  int len;
} uart_request_t;

void uart_tx_server() {
  int tid = MyTid();
  int requester;
  char c;

  uart_request_t request;

  uart_packet_t packet;
  packet.type = 1;

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

  int courier_priority = ((channel == COM1) ? PRIORITY_UART1_TX_SERVER : PRIORITY_UART2_TX_SERVER);
  int warehouse_tid = ((channel == COM1) ? uart1_tx_warehouse_tid : uart2_tx_warehouse_tid);
  int ready = false;

  int courier_tid = createCourier(courier_priority, warehouse_tid, tid, sizeof(uart_packet_t));

  log_uart_server("uart_server initialized channel=%d tid=%d", channel, tid);

  while (true) {
    ReceiveS(&requester, request);

    if (requester == courier_tid) {
      ready = true;
    } else {
      switch ( request.type ) {
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
            packet.len = 1;
            packet.data[0] = c;
            ReplyS(courier_tid, packet);
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
      case GET_QUEUE_REQUEST:
        ReplyS(requester, outputQueueLength);
        break;
      default:
        KASSERT(false, "uart_server received unknown request type=%d", request.type);
        break;
      }
    }

    if (ready && outputQueueLength > 0) {
      packet.len = outputQueueLength;
      if (packet.len > RESPONSE_BUFFER_SIZE) {
        packet.len = RESPONSE_BUFFER_SIZE;
      }
      for (int i = 0; i < packet.len; i++) {
        int index = (outputStart+i) % OUTPUT_QUEUE_MAX;
        packet.data[i] = outputQueue[index];
      }
      outputStart = (outputStart+packet.len) % OUTPUT_QUEUE_MAX;
      outputQueueLength -= packet.len;
      ReplyS(courier_tid, packet);
      ready = false;
    }
  }
}

#define PACKET_QUEUE_MAX 100
#define LOGGING_PACKET_QUEUE 100

void uart_tx() {
  uart_request_t request;

  uart1_tx_notifier_tid = createNotifier(COM1);
  uart2_tx_notifier_tid = createNotifier(COM2);

  uart1_tx_warehouse_tid = createWarehouse(
      PRIORITY_UART1_TX_SERVER,
      uart1_tx_notifier_tid,
      PACKET_QUEUE_MAX,
      PRIORITY_UART1_TX_SERVER,
      sizeof(uart_packet_t));

  uart2_tx_warehouse_tid = createWarehouse(
      PRIORITY_UART2_TX_SERVER,
      uart2_tx_notifier_tid,
      PACKET_QUEUE_MAX,
      PRIORITY_UART2_TX_SERVER,
      sizeof(uart_packet_t));

  uart1_tx_server_tid = Create(PRIORITY_UART1_TX_SERVER, uart_tx_server);
  request.channel = COM1;
  SendSN(uart1_tx_server_tid, request);

  uart2_tx_server_tid = Create(PRIORITY_UART2_TX_SERVER, uart_tx_server);
  request.channel = COM2;
  SendSN(uart2_tx_server_tid, request);

  logging_warehouse_tid = createWarehouse(
      PRIORITY_LOGGING_WAREHOUSE,
      uart2_tx_warehouse_tid,
      LOGGING_PACKET_QUEUE,
      PRIORITY_LOGGING_COURIER,
      sizeof(uart_packet_t));
}

int Putcs( int channel, const char* c, int len ) {
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
  log_task("Putc c=%c", active_task->tid, c);
  int server_tid = ((channel == COM1) ? uart1_tx_server_tid : uart2_tx_server_tid);
  if (server_tid == -1) {
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

int Putc( int channel, const char c ) {
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
  log_task("Putc tid=%d char=%c", active_task->tid, uart2_tx_server_tid, c);
  int server_tid = ((channel == COM1) ? uart1_tx_server_tid : uart2_tx_server_tid);
  if (server_tid == -1) {
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

int Putstr(int channel, const char *str ) {
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
  log_task("Putstr str=%s", active_task->tid, str);
  int server_tid = ((channel == COM1) ? uart1_tx_server_tid : uart2_tx_server_tid);
  if (server_tid == -1) {
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

int Puti(int channel, int i) {
  char bf[12];
  ui2a(i, 10, bf);
  return Putstr(channel, bf);
}

int Logp(uart_packet_t *packet) {
  log_task("Logp str=%d", active_task->tid, packet.type);
  if (logging_warehouse_tid == -1) {
    KASSERT(false, "Logging relay not initialized");
    return -1;
  }

  SendSN(logging_warehouse_tid, *packet);

  return 0;
}

int Logs(int type, const char *str) {
  log_task("Logp str=%d", active_task->tid, packet.type);
  if (logging_warehouse_tid == -1) {
    KASSERT(false, "Logging relay not initialized");
    return -1;
  }

  int slen = jstrlen(str);
  uart_packet_t packet;
  packet.type = type;
  packet.len = slen;
  jmemcpy(&packet.data, str, slen*sizeof(char));

  SendSN(logging_warehouse_tid, packet);

  return 0;
}

void MoveTerminalCursor(unsigned int x, unsigned int y) {
  // FIXME: make this happen in one Putstr operation for escape sequence continuity
  char bf[12];
  Putc(COM2, ESCAPE_CH);
  Putc(COM2, '[');

  ui2a(y, 10, bf);
  Putstr(COM2, bf);

  Putc(COM2, ';');

  ui2a(x, 10, bf);
  Putstr(COM2, bf);

  Putc(COM2, 'H');
}

int GetTxQueueLength( int channel ) {
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided: got channel=%d", channel);
  log_task("Getc tid=%d", active_task->tid, uart_rx_server_tid);
  int server_tid = ((channel == COM1) ? uart1_tx_server_tid : uart2_tx_server_tid);
  if (server_tid == -1) {
    KASSERT(false, "UART tx server not initialized");
    return -1;
  }
  uart_request_t req;
  req.type = GET_QUEUE_REQUEST;
  req.channel = channel;
  int result;
  SendS(server_tid, req, result);
  return result;
}

int Putf(int channel, char *fmt, ...) {
  char buf[2048];
  va_list va;
  va_start(va,fmt);
  jformat(buf, 2048, fmt, va);
  va_end(va);
  return Putstr(channel, buf);
}
