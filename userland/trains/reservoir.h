#pragma once

/**
 * The Reservoir is for reserving segments of track.
 * This is used by train controlers in order to know where
 * trains can traverse
 */

#include <packet.h>

#define RESERVOIR_REQUEST_OK 1
#define RESERVOIR_REQUEST_ERROR 0

typedef struct {
  // For internal message passing
  packet_t packet;

  // Reasonable upper limit to request at one time
  int tracks[12];
  int len;
} segment_t;

void reservoir_task();

int RequestSegment(segment_t * segment);

void ReleaseSegment(segment_t * segment);
