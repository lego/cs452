#ifndef __TRAIN_CONTROLLER_H__
#define __TRAIN_CONTROLLER_H__

enum {
  SWITCH_CURVED,
  SWITCH_STRAIGHT,
};

void train_controller_server();

int SetTrainSpeed(int train, int speed);
int MoveTrain(int train, int speed, int node_id);
int ReverseTrain(int train, int currentSpeed);
int InstantStop(int train);
int SetSwitch(int sw, int state);

#endif
