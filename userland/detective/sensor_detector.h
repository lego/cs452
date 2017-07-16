#pragma once

#include <detective/detector.h>

/**
 * Sensor Detector is a task you can kick off which will send you a message
 * when a specific sensor has been hit.
 *
 * See detective/detector.h for the message that this task sends
 */

/**
 * Begins a sensor detector
 * @param  name      of the detector
 * @param  send_to   the tid when detected
 * @param  sensor_no to wait on
 * @return           returns the unique identifier (for sensor detectors)
 */
int StartSensorDetector(const char * name, int send_to, int sensor_no);
