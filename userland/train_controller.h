#ifndef __TRAIN_CONTROLLER_H__
#define __TRAIN_CONTROLLER_H__

enum {
  SWITCH_CURVED,
  SWITCH_STRAIGHT,
};

void train_controller_server();

int SetTrainSpeed(int train, int speed);
int ReverseTrain(int train, int currentSpeed);
int SetSwitch(int sw, int state);

#endif
