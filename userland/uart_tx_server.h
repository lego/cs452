#ifndef __UART_TX_SERVER_H__
#define __UART_TX_SERVER_H__

void uart_tx();

int Putc(int channel, const char c );
int Putstr(int channel, const char *str );
int Putcs( int channel, const char *c, int len );

#endif
