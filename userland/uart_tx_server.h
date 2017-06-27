#ifndef __UART_TX_SERVER_H__
#define __UART_TX_SERVER_H__

void uart_tx();

int Putc(int channel, const char c );
int Putstr(int channel, const char *str );
int Putcs( int channel, const char *c, int len );
void move_cursor(unsigned int x, unsigned int y);
int Puti(int channel, int i);


// Terminal locations
#define SWITCH_LOCATION 3
#define SENSOR_HISTORY_LOCATION 10
#define COMMAND_LOCATION 23

#endif
