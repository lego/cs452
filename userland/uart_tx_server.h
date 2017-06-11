#ifndef __UART_TX_SERVER_H__
#define __UART_TX_SERVER_H__

void uart_tx_server();

int Putcs( int tid, int channel, char *c, int len );
int Putc( int tid, int channel, char c );
int Putstr( int tid, int channel, char *str );

#endif
