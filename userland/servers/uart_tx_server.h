#pragma once

#define RESPONSE_BUFFER_SIZE 16

#define PACKET_SENSOR_DATA 10
#define PACKET_SWITCH_DATA 11

#define PACKET_LOG_INFO 90

typedef struct {
  int len;
  int type;
} uart_packet_t;

typedef struct {
  int len;
  int type;
  char data[12];
} uart_packet_fixed_size_t;

void uart_tx();

int Putc(int channel, const char c );
int Putstr(int channel, const char *str);
int Putcs(int channel, const char *c, int len);
int Puti(int channel, int i);
int Putf(int channel, char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
int PutPacket(uart_packet_t *packet);

#define PutFixedPacket(packet) do { \
  KASSERT((packet)->len <= RESPONSE_BUFFER_SIZE, "UART fixed packet overflown. Adjust size or use the variable sized one."); \
  PutPacket((uart_packet_t *) packet); \
} while (0)

int Logp(uart_packet_t *packet);
int Logs(int type, const char *str);
int Logf(int type, char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
void MoveTerminalCursor(unsigned int x, unsigned int y);
int GetRxQueueLength(int channel);

// Log types
#define EXECUTOR_LOGGING 103
