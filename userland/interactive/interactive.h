#pragma once

#include <packet.h>

enum command_t {
  COMMAND_INVALID,
  COMMAND_QUIT,
  COMMAND_TRAIN_SPEED,
  COMMAND_TRAIN_REVERSE,
  COMMAND_SWITCH_TOGGLE,
  COMMAND_SWITCH_TOGGLE_ALL,
  COMMAND_CLEAR_SENSOR_SAMPLES,
  COMMAND_CLEAR_SENSOR_OFFSET,
  COMMAND_PRINT_SENSOR_SAMPLES,
  COMMAND_PRINT_SENSOR_MULTIPLIERS,
  COMMAND_HELP,
  COMMAND_STOP_FROM,
  COMMAND_SET_LOCATION,
  COMMAND_MANUAL_SENSE,
  COMMAND_PRINT_VELOCITY,
  COMMAND_SET_VELOCITY,
  COMMAND_STOPPING_DISTANCE_OFFSET,
  COMMAND_SET_STOPPING_DISTANCE,
  COMMAND_SET_STOPPING_DISTANCEN,
  COMMAND_UPTIME,
  COMMAND_STATUS,
  COMMAND_NAVIGATE,
  COMMAND_PATH,
}; typedef int command_t;

typedef struct {
  // INTERACTIVE_COMMAND
  packet_t packet;

  command_t command_type;
  int argc;
  char *cmd;
  char *arg1;
  char *arg2;
  char *arg3;
} interactive_command_t;

typedef struct {
  // INTERACTIVE_ECHO
  packet_t packet;

  char echo[4];
} interactive_echo_t;

typedef struct {
  // INTERACTIVE_TIME_UPDATE
  packet_t packet;
} interactive_time_update_t;
