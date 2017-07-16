#pragma once

/**
 * Subscribes to the Sensor Collector for all sensor data.
 * Receives requests from Sensor Detectors and multiplexes all
 * incoming sensor data to all detectors currently waiting
 *
 * Receives:
 *  SENSOR_DATA from Sensor Collector and
 *  SENSOR_DETECTOR_REQUEST from Sensor Detector
 * Sends:
 *  SENSOR_DATA to Sensor Detector
 *
 */
void sensor_detector_multiplexer_task();
