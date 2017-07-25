#pragma once

#include <detective/detector.h>

/**
 * Delay Detector is a task you can kick off which will send you a message
 * after a certain delay, without the original task having to block
 * (besides creating and initializing this task).
 *
 * See detective/detector.h for the message that this task sends
 */

/**
 * Begins a delay detector
 * @param name    of the detector
 * @param send_to to alert when done
 * @param ticks   to wait on
 * @return        returns the unique identifier (for delay detectors)
 */
int StartRecyclableDelayDetector(const char * name, int send_to, int ticks);
int StartDelayDetector(const char * name, int send_to, int ticks);
