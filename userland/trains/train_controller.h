#pragma once

#include <packet.h>
#include <track/pathing.h>

typedef enum {
  TRAIN_CONTROLLER_SET_SPEED,
  TRAIN_CONTROLLER_REVERSE,
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

/**
 * Initializes internal state of the train controllers
 */
void InitTrainControllers();

int CreateTrainController(int train);


/**
 * Alert a train controller about a sensor trigger attributed to it
 * @param train     attributed to trigger
 * @param sensor_no triggered
 * @param timestamp of triggering
 */
void AlertTrainController(int train, int sensor_no, int timestamp);


/**
 * Commanding a train controller to take a basic action
 * @param train to command
 * @param type  of action:
 *                TRAIN_CONTROLLER_SET_SPEED
 *                ...
 * @param speed (additional parameter)
 */
void TellTrainController(int train, int type, int speed);


/**
 * Tell a train to navigate to a location
 * @param train to navigate
 * @param speed to travel at (temp for diagnostics)
 * @param path  to travel
 */
void NavigateTrain(int train, int speed, path_t * path);
