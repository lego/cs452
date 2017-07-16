#pragma once

enum {
  SWITCH_STRAIGHT,
  SWITCH_CURVED,
};

#define NUM_SWITCHES 22

void switch_controller();

int SetSwitch(int sw, int state);
int GetSwitchState(int sw);
