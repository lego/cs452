#pragma once

enum {
  SWITCH_STRAIGHT,
  SWITCH_CURVED,
};

void switch_controller();

int SetSwitch(int sw, int state);
int GetSwitchState(int sw);
