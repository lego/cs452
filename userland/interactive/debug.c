#include <basic.h>
#include <kernel.h>
#include <priorities.h>
#include <interactive/debug.h>
#include <servers/clock_server.h>
#include <servers/uart_rx_server.h>
#include <servers/uart_tx_server.h>

void uart_rx_monitor() {
  int channel;
  int sender;
  ReceiveS(&sender, channel);
  ReplyN(sender);

  // whois

  while (true) {
    Delay(1);
    int queue_length = GetRxQueueLength(channel);
    SendSN(sender, queue_length);
  }
}

void uart_tx_monitor() {
  int channel;
  int sender;
  ReceiveS(&sender, channel);
  ReplyN(sender);
}

void diagnostic_printer_task() {
  int parent = MyParentTid();

  int buffers[4];
  while (true) {
    Delay(100);
    Send(parent, NULL, 0, buffers, sizeof(buffers));
  }
}

void interactive_diagnostic_task() {
  int channel;
  int uart1_rx_monitor = CreateWithName(PRIORITY_INTERACTIVE_DIAGNOSTICS + 1, uart_rx_monitor, "UART1 Rx Monitor");
  channel = COM1;
  SendSN(uart1_rx_monitor, channel);

  int uart1_tx_monitor = CreateWithName(PRIORITY_INTERACTIVE_DIAGNOSTICS + 1, uart_tx_monitor, "UART1 Tx Monitor");
  channel = COM1;
  SendSN(uart1_tx_monitor, channel);

  int diagnostic_printer = Create(PRIORITY_INTERACTIVE_DIAGNOSTICS + 1, diagnostic_printer_task);

  int tmp_buf;
  int sender;
  int buffer_sizes[4];
  while (true) {
    ReceiveS(&sender, tmp_buf);
    if (sender == uart1_rx_monitor) {
      buffer_sizes[0] = tmp_buf;
    } else if (sender == uart1_tx_monitor) {
      buffer_sizes[1] = tmp_buf;
    } else if (sender == diagnostic_printer) {
    } else {
      KASSERT(false, "Unexpected sender.");
    }
  }
}

void PrintDiagnostics() {

}
