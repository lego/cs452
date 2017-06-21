#ifndef __UART_TX_SERVER_H__
#define __UART_TX_SERVER_H__

void uart_tx();

int Putc(int channel, char c );
int Putstr(int channel, char *str );
int Putcs( int channel, char *c, int len );

#endif
