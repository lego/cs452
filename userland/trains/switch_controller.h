#pragma once

enum {
  SWITCH_STRAIGHT,
  SWITCH_CURVED,
};

#define NUM_SWITCHES 22

enum {
  SWITCH_SET,
  SWITCH_GET
};

typedef struct {
  int type;
  int index;
  int value;
} switch_control_request_t;

void switch_controller();

int SetSwitch(int sw, int state);
int GetSwitchState(int sw);
