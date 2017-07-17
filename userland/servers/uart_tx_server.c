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
// Pretty terrible, using track graph for some local test output
#include <track/pathing.h>

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

  char request_buffer[512] __attribute__ ((aligned (4)));
  uart_packet_t * packet = (uart_packet_t *) request_buffer;
  char * packet_data = request_buffer + sizeof(uart_packet_t);

  while (true) {
    int bytes = ReceiveS(&requester, request_buffer);
    KASSERT(bytes < 512, "Packet buffer overflown. Re-evaluate buffer sizes.");
    ReplyN(requester);
    log_uart_server("uart_notifer channel=%d", channel);
    switch(channel) {
      case COM1:
        for (int i = 0; i < packet->len; i++) {
          AwaitEventPut(EVENT_UART1_TX, packet_data[i]);
          log_uart_server("uart_notifer COM1 putc=%c", packet_data[i]);
        }
        break;
      case COM2:
#if NONTERMINAL_OUTPUT
        AwaitEventPut(EVENT_UART2_TX, packet->len);
        AwaitEventPut(EVENT_UART2_TX, packet->type);
#else
        if (packet->type != 1) {
          break;
        }
#endif
        for (int i = 0; i < packet->len; i++) {
          AwaitEventPut(EVENT_UART2_TX, packet_data[i]);
          log_uart_server("uart_notifer COM2 putc=%c", packet_data[i]);
        }
        break;
    }
  }
}

int createNotifier(int channel, char * name) {
  int notifier_priority = ((channel == COM1) ? PRIORITY_UART1_TX_NOTIFIER : PRIORITY_UART2_TX_NOTIFIER);
  int notifier_tid = CreateWithName(notifier_priority, uart_tx_notifier, name);
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


  char request_buffer[512] __attribute__ ((aligned (4)));
  uart_packet_t * packet = (uart_packet_t *) request_buffer;
  char * packet_data = request_buffer + sizeof(uart_packet_t);

  packet->type = 1;

  ReceiveS(&requester, request);
  int channel = request.channel;
  KASSERT(channel == COM1 || channel == COM2, "Invalid channel provided to uart_tx_server: got channel=%d", channel);
  ReplyN(requester);

  if (channel == COM1) {
    RegisterAs(NS_UART1_TX_SERVER);
  } else if (channel == COM2) {
    RegisterAs(NS_UART2_TX_SERVER);
  }

  char outputQueue[OUTPUT_QUEUE_MAX];
  int outputStart = 0;
  int outputQueueLength = 0;

  int courier_priority = ((channel == COM1) ? PRIORITY_UART1_TX_SERVER : PRIORITY_UART2_TX_SERVER);
  int warehouse_tid = ((channel == COM1) ? uart1_tx_warehouse_tid : uart2_tx_warehouse_tid);
  int ready = false;

  char * courier_name = (channel == COM1) ? "UART1 TX server courier" : "UART2 TX server courier";

  int courier_tid = createCourier(courier_priority, warehouse_tid, tid, 0, courier_name);

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
            packet->len = 1;
            packet_data[0] = c;
            Reply(courier_tid, request_buffer, sizeof(uart_packet_t) + packet->len);
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
      packet->len = outputQueueLength;
      if (packet->len > RESPONSE_BUFFER_SIZE) {
        packet->len = RESPONSE_BUFFER_SIZE;
      }
      for (int i = 0; i < packet->len; i++) {
        int index = (outputStart+i) % OUTPUT_QUEUE_MAX;
        packet_data[i] = outputQueue[index];
      }
      outputStart = (outputStart+packet->len) % OUTPUT_QUEUE_MAX;
      outputQueueLength -= packet->len;
      Reply(courier_tid, request_buffer, sizeof(uart_packet_t) + packet->len);
      ready = false;
    }
  }
}

#define PACKET_QUEUE_MAX 100
#define LOGGING_PACKET_QUEUE 100

void uart_tx() {
  uart_request_t request;

  uart1_tx_notifier_tid = createNotifier(COM1, "UART1 TX notifier");
  uart2_tx_notifier_tid = createNotifier(COM2, "UART2 TX notifier");

  uart1_tx_warehouse_tid = createWarehouse(
      PRIORITY_UART1_TX_SERVER,
      uart1_tx_notifier_tid,
      PACKET_QUEUE_MAX,
      PRIORITY_UART1_TX_SERVER,
      0,
      "UART1 TX warehouse");

  uart2_tx_warehouse_tid = createWarehouse(
      PRIORITY_UART2_TX_SERVER,
      uart2_tx_notifier_tid,
      PACKET_QUEUE_MAX,
      PRIORITY_UART2_TX_SERVER,
      0,
      "UART2 TX warehouse");

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
      0,
      "UART2 TX logging warehouse");
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

int PutPacket(uart_packet_t *packet) {
  #if defined(DEBUG_MODE)
  char *packet_data = ((char *)packet) + sizeof(uart_packet_t);
  uart_packet_fixed_size_t *p = (uart_packet_fixed_size_t *)packet;
  if (p->type == PACKET_TRAIN_LOCATION_DATA) {
    bwprintf(COM2, "TRAIN LOCATION: %d at %4s\n", (int) p->data[4], track[(int) p->data[5]].name);
  } else if (p->type == PACKET_SENSOR_DATA) {
    bwprintf(COM2, "SENSOR DATA: %4s. Attributed to %d\n", track[(int) p->data[4]].name, (int) p->data[5]);
  } else if (p->type == PACKET_RESEVOIR_SET_DATA) {
    bwprintf(COM2, "RESERVING: %d got %d segments\n", (int) packet_data[4], (packet->len - 5) / 2);
  } else if (p->type == PACKET_RESEVOIR_UNSET_DATA) {
    bwprintf(COM2, "UNRESERVE: %d released %d segments\n", (int) packet_data[4], (packet->len - 5) / 2);
  } else {
    bwprintf(COM2, "Unhandled packet type=%d\n", p->type);
  }


  return 0;
  #endif
  log_task("PutPacket len=%d type=%d", active_task->tid, packet->len, packet->type);
  int server_tid = uart2_tx_warehouse_tid;
  if (server_tid == -1) {
    KASSERT(false, "UART tx server not initialized");
    return -1;
  }
  KASSERT(packet->len < 2048, "Packet length was a bit large. Ensure it's okay, len=%d", packet->len);
  Send(server_tid, packet, packet->len + sizeof(uart_packet_t), NULL, 0);
  return 0;
}

int Logp(uart_packet_t *packet) {
  log_task("Logp str=%d", active_task->tid, packet.type);
  if (logging_warehouse_tid == -1) {
    KASSERT(false, "Logging relay not initialized");
    return -1;
  }

  KASSERT(packet->len < 2048, "Packet length was a bit large. Ensure it's okay, len=%d", packet->len);
  Send(logging_warehouse_tid, packet, packet->len + sizeof(uart_packet_t), NULL, 0);
  return 0;
}

int Logf(int type, char *fmt, ...) {
  char buf[512];
  va_list va;
  va_start(va,fmt);
  jformat(buf, 512, fmt, va);
  va_end(va);
  return Logs(type, buf);
}


int Logs(int type, const char *str) {
  #if defined(DEBUG_MODE)
  // Output to STDOUT for local
  bwputstr(COM2, str);
  bwputc(COM2, '\n');
  return 0;
  #endif

  if (type != IDLE_LOGGING && type != UPTIME_LOGGING) {
    RecordLog(str);
    RecordLog("\n\r");
  }

  log_task("Logp str=%d", active_task->tid, packet.type);
  if (logging_warehouse_tid == -1) {
    KASSERT(false, "Logging relay not initialized");
    return -1;
  }

  int slen = jstrlen(str);
  char message_buffer[1024] __attribute__ ((aligned (4)));;
  uart_packet_t * packet = (uart_packet_t *) message_buffer;
  char * packet_data = message_buffer + sizeof(uart_packet_t);
  packet->type = type;
  packet->len = slen;
  jmemcpy(packet_data, str, slen*sizeof(char));

  int size = sizeof(uart_packet_t) + slen;
  KASSERT(size <= 1024, "Message buffer overflow. Re-evaluate buffer sizes");
  Send(logging_warehouse_tid, message_buffer, size, NULL, 0);
  return 0;
}

void MoveTerminalCursor(unsigned int x, unsigned int y) {
  char buffer[12];
  jformatf(buffer, sizeof(buffer), "\e[%d;%dH", y, x);
  Putstr(COM2, buffer);
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
