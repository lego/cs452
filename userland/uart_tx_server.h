#ifndef __UART_TX_SERVER_H__
#define __UART_TX_SERVER_H__

void uart_tx_server();

int Putc(int channel, char c );
int Putstr(int channel, char *str );

#endif
