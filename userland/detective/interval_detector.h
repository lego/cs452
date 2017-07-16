#pragma once

#include <detective/detector.h>

/**
 * Interval Detector is a task you can kick off to send you a message every
 * interval amount of ticks. You can stop it via. Destroy
 *
 * See detective/detector.h for the message that this task sends
 */

/**
 * Begins an delay detector on an interval
 * @param name    of the detector
 * @param send_to to alert
 * @param ticks   to wait on each interval
 * @return        returns the unique identifier
 */
int StartIntervalDetector(const char *name, int send_to, int interval_ticks);
