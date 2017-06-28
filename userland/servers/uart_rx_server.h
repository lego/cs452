#ifndef __UART_RX_SERVER_H__
#define __UART_RX_SERVER_H__

void uart_rx_server();
void uart_rx();

char Getc(int channel);
int ClearRx(int channel);
int GetRxQueueLength(int channel);

#endif
