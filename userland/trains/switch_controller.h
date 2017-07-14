#pragma once

enum {
  SWITCH_CURVED,
  SWITCH_STRAIGHT,
};

void switch_controller();

int SetSwitch(int sw, int state);
int GetSwitchState(int sw);
