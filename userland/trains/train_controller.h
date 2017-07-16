#pragma once

enum {
  TRAIN_CONTROLLER_SENSOR,
  TRAIN_CONTROLLER_COMMAND,
};

enum {
  TRAIN_CONTROLLER_SET_SPEED,
};

typedef struct {
  int type;

  union {
    struct {
      int sensor;
      int time;
    } sensor;

    struct {
      int type;
      int speed;
    } command;
  };
} train_controller_msg_t;

int CreateTrainController(int train);
