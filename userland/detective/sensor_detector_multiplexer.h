#pragma once

/**
 * Subscribes to the Sensor Collector for all sensor data, receives
 * Sensor Detectors and multiplexes the incoming sensor data to all detectors
 *
 * Receives:
 *  SENSOR_DATA from Sensor Collector and
 *  SENSOR_DETECTOR_REQUEST from Sensor Detector
 * Sends:
 *  SENSOR_DATA to Sensor Detector
 *
 */
void sensor_detector_multiplexer_task();
