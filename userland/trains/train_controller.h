#pragma once

#include <packet.h>

typedef enum {
  TRAIN_CONTROLLER_SET_SPEED,
} train_command_t;

typedef struct {
  // type = TRAIN_CONTROLLER_COMMAND
  packet_t packet;
  train_command_t type;
  int speed;
} train_command_msg_t;

int CreateTrainController(int train);
