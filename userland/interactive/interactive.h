#pragma once

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

enum interactive_req_type_t {
  INT_REQ_SENSOR_UPDATE,
  INT_REQ_COMMAND,
  INT_REQ_TIME,
}; typedef int interactive_req_type_t;

typedef struct {
  interactive_req_type_t type;

  // FIXME: figure out a better way to not put all params in one struct
  // but use different structs

  // INT_REQ_COMMAND data
  command_t command_type;
  int argc;
  char *cmd;
  char *arg1;
  char *arg2;
  char *arg3;
} interactive_req_t;
