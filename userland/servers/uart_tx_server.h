#ifndef __UART_TX_SERVER_H__
#define __UART_TX_SERVER_H__

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
int Putp(int channel, uart_packet_t *packet);
int Logp(uart_packet_t *packet);
int Logs(int type, const char *str);
void MoveTerminalCursor(unsigned int x, unsigned int y);
int GetRxQueueLength(int channel);

// Terminal locations
#define SWITCH_LOCATION 3
#define SENSOR_HISTORY_LOCATION 10
#define COMMAND_LOCATION 23

#endif
