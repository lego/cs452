#pragma once

#include <packet.h>

/**
 * Generic detector message, sent from detectors to the task asking for the
 * result.
 */
typedef struct {
  // Various types:
  // type = DELAY_DETECT
  // type = SENSOR_DETECT
  // type = INTERVAL_DETECT
  packet_t packet;
  // if DELAY_DETECT or INTERVAL_DETECT, ticks waited
  // if SENSOR_DETECT, sensor detected
  int details;
  // Unique identifier for the detector type, returned on creation.
  // Helps identify what you might have detected (e.g. delays)
  // Unique over (detector_type, identifier)
  int identifier;
} detector_message_t;
