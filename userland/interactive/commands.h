#pragma once

#include <packet.h>

/**
 * Defining a new command:
 *   - Add the command enum here
 *   - Add the command string into command_parser.c (e.g. reverse = "rv")
 *   - Add the case into command_interpreter, i.e. an enum, and a validation function
 *     these validation functions have helper macros for asserting correct inputs
 *     such as count, int type, etc.
 *   - Consume command type:
 *     interpreted commands are sent to interactive_tid for display purposes,
 *     and executor_tid for actions to execute (e.g. switches, trains)
 *     NOTE: SendToBoth is done in order to display in interactive and action
 */

typedef enum {
  COMMAND_INVALID,

  COMMAND_QUIT,
  COMMAND_TRAIN_CALIBRATE,
  COMMAND_TRAIN_SPEED,
  COMMAND_TRAIN_REVERSE,

  COMMAND_SWITCH_TOGGLE,
  COMMAND_SWITCH_TOGGLE_ALL,

  COMMAND_CLEAR_SENSOR_SAMPLES,
  COMMAND_CLEAR_SENSOR_OFFSET,
  COMMAND_PRINT_SENSOR_SAMPLES,
  COMMAND_PRINT_SENSOR_MULTIPLIERS,

  COMMAND_PRINT_VELOCITY,
  COMMAND_SET_VELOCITY,
  COMMAND_SET_LOCATION,
  COMMAND_STOPPING_DISTANCE_OFFSET,
  COMMAND_SET_STOPPING_DISTANCE,
  COMMAND_SET_STOPPING_DISTANCEN,

  COMMAND_STOP_FROM,
  COMMAND_NAVIGATE,
  COMMAND_NAVIGATE_RANDOMLY,
  COMMAND_PATH,

  COMMAND_MANUAL_SENSE,
} command_t;

typedef struct {
  packet_t packet;
  command_t type;
} cmd_t;

typedef struct {
  cmd_t base;
  char error[80];
} cmd_error_t;

typedef struct {
  cmd_t base;
  // Some of these arguments are used, some aren't, in commands

  // Target train of operation
  int train;
  // Desired speed of operation
  int speed;

  // Target switch of operation
  int switch_no;
  // Direction for switch
  int switch_dir;

  // Stopping dist, velo, other misc. args
  int extra_arg;

  // Source node
  int src_node;

  // Destination node
  int dest_node;

} cmd_data_t;
