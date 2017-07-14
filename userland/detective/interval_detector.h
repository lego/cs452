#pragma once

/**
 * Begins an delay detector on an interval
 * @param name    of the detector
 * @param send_to to alert
 * @param ticks   to wait on each interval
 * @return        returns the unique identifier
 */
int StartIntervalDetector(const char * name, int send_to, int interval_ticks);
