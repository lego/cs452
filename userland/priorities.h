#ifndef __PRIORITIES_H__
#define __PRIORITIES_H__

#define PRIORITY_NAMESERVER 1
#define PRIORITY_CLOCK_SERVER 2

#define PRIORITY_RX_SERVER 2
  #define PRIORITY_RX_NOTIFIER (PRIORITY_RX_SERVER-1)

#define PRIORITY_TX_SERVER 4
  #define PRIORITY_TX_NOTIFIER (PRIORITY_TX_SERVER-1)

#define PRIORITY_TRAIN_CONTROLLER_SERVER 6
  #define PRIORITY_TRAIN_CONTROLLER_TASK (PRIORITY_TRAIN_CONTROLLER_SERVER-1)

#define PRIORITY_IDLE_TASK 31

#endif
