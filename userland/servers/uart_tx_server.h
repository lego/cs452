#pragma once

#define RESPONSE_BUFFER_SIZE 32

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
int Putp(int channel, uart_packet_t *packet);
int Logp(uart_packet_t *packet);
int Logs(int type, const char *str);
void MoveTerminalCursor(unsigned int x, unsigned int y);
int GetRxQueueLength(int channel);

// Log types
#define EXECUTOR_LOGGING 103
