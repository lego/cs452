#pragma once

/**
 * Begins a sensor detector
 * @param  name      of the detector
 * @param  send_to   [description]
 * @param  sensor_no to wait on
 * @return           returns the unique identifier (for sensor detectors)
 */
int StartSensorDetector(const char * name, int send_to, int sensor_no);
