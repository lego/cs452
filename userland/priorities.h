#pragma once

// Cool! We can also identify the hierarchy via. indentation

#define PRIORITY_ENTRY_TASK 1

#define PRIORITY_NAMESERVER 1

#define PRIORITY_CLOCK_SERVER 2
  #define PRIORITY_CLOCK_NOTIFIER 1

#define PRIORITY_UART1_TX_SERVER 2
  #define PRIORITY_UART1_TX_NOTIFIER 1

#define PRIORITY_UART1_RX_SERVER 2
  #define PRIORITY_UART1_RX_NOTIFIER 1

#define PRIORITY_UART2_TX_SERVER 4
  #define PRIORITY_UART2_TX_NOTIFIER 3

#define PRIORITY_UART2_RX_SERVER 4
  #define PRIORITY_UART2_RX_NOTIFIER 3

#define PRIORITY_TRAIN_CONTROLLER_SERVER 6
  #define PRIORITY_TRAIN_CONTROLLER_TASK 5

#define PRIORITY_IDLE_TASK 31
