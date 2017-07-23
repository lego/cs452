#pragma once

#include <packet.h>
#include <track/pathing.h>

typedef struct {
  // type = ROUTE_FAILURE
  packet_t packet;

  int train;
  int dest_id;
} route_failure_t;

typedef enum {
  TRAIN_CONTROLLER_SET_SPEED,
  TRAIN_CONTROLLER_REVERSE,
  TRAIN_CONTROLLER_CALIBRATE,
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

typedef enum {
  OPERATION_NAVIGATE,
  OPERATION_NAVIGATE_RANDOMLY,
  OPERATION_STOPFROM,
} pathing_operation_t;

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
 * Tell a train to navigate to a random location, then keep re-navigating
 * @param train to navigate
 * @param speed to travel at (temp for diagnostics)
 * @param path  to travel
 */
void NavigateTrainRandomly(int train, int speed, path_t * path);

/**
 * Tell a train to navigate to a location
 * @param train to navigate
 * @param speed to travel at (temp for diagnostics)
 * @param path  to travel
 */
void NavigateTrain(int train, int speed, path_t * path);


/**
 * Tell a train to stop from a location
 * @param train to navigate
 * @param speed to travel at (temp for diagnostics)
 * @param path  to stopping locaiton
 */
void StopTrainAt(int train, int speed, path_t * path);
