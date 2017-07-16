#pragma once

#include <detective/detector.h>

/**
 * Begins a delay detector
 * @param name    of the detector
 * @param send_to to alert when done
 * @param ticks   to wait on
 * @return        returns the unique identifier (for delay detectors)
 */
int StartDelayDetector(const char * name, int send_to, int ticks);
