#pragma once

void uart_rx_server();
void uart_rx();

char Getc(int channel);
int ClearRx(int channel);
int GetRxQueueLength(int channel);
