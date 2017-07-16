#pragma once

#define RESPONSE_BUFFER_SIZE 32

#define PACKET_SENSOR_DATA 10
#define PACKET_SWITCH_DATA 11

#define PACKET_LOG_INFO 90

typedef struct {
  int len;
  int type;
  char data[RESPONSE_BUFFER_SIZE];
} uart_packet_t;

void uart_tx();

int Putc(int channel, const char c );
int Putstr(int channel, const char *str);
int Putcs(int channel, const char *c, int len);
int Puti(int channel, int i);
int Putf(int channel, char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
int PutPacket(uart_packet_t *packet);
int Logp(uart_packet_t *packet);
int Logf(int type, char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
int Logs(int type, const char *str);
int Logf(int type, char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
void MoveTerminalCursor(unsigned int x, unsigned int y);
int GetRxQueueLength(int channel);

// Log types
#define EXECUTOR_LOGGING 150
