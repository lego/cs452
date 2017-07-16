#pragma once

#include <packet.h>
#include <track/pathing.h>

typedef enum {
  TRAIN_CONTROLLER_SET_SPEED,
} train_command_t;

typedef struct {
  // type = TRAIN_CONTROLLER_COMMAND
  packet_t packet;

  train_command_t type;
  int speed;
} train_command_msg_t;

typedef struct {
  // type = TRAIN_NAVIGATE_COMMAND
  packet_t packet;

  path_t path;
  int speed; // Initially for testing at a consistent speed
} train_navigate_t;


int CreateTrainController(int train);


/**
 * Commanding a train controller to take a basic action
 * @param train to command
 * @param type  of action:
 *                TRAIN_CONTROLLER_SET_SPEED
 *                ...
 * @param speed (additional parameter)
 */
void TellTrainController(int train, int type, int speed);
