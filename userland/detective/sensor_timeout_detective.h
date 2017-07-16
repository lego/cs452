#pragma once

#include <packet.h>

#define DETECTIVE_TIMEOUT 0
#define DETECTIVE_SENSOR 1

/**
 * Sensor Timeout detective combines the two results of the
 * delay detector and sensor detector.
 * i.e. it just provides a nicer way to call both (maybe excessive, hah, one to one for class spec)
 */

/**
 * Message sent from the Sensor Timeout Detective, informing the desired task
 * whether a timeout occured, or a sensor was detected
 */

typedef struct {
  // types: SENSOR_TIMEOUT_DETECTIVE
  packet_t packet;
  // if the delay happens first, we get DETECTIVE_TIMEOUT
  // if the sensor happens first, we get DETECTIVE_SENSOR
  int action;

  // timeout requested for detective
  int timeout;
  // sensor requested for detective
  int sensor_no;

  // Unique identifier for the detective, returned on creation.
  // Helps identify what you might have detected
  int identifier;
} sensor_timeout_message_t;

/**
 * Begins a sensor detector
 * @param  name      of the detector
 * @param  send_to   when finished
 * @param  timeout   ticks to timeout on
 * @param  sensor_no to wait for
 * @return           returns the unique identifier (for sensor detectors)
 */
int StartSensorTimeoutDetective(const char * name, int send_to, int timeout, int sensor_no);
