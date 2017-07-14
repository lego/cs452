#pragma once

/**
 * The Reservoir is for reserving segments of track.
 * This is used by train controlers in order to know where
 * trains can traverse
 */

#include <packet.h>

#define RESERVOIR_REQUEST_OK 0
#define RESERVOIR_REQUEST_ERROR 1

/**
 * segment_t specifies a track segment, i.e. edge, from a node and direction
 */
typedef struct {
  // a track node
  int track_node;
  // either DIR_AHEAD
  // or DIR_CURVED and DIR_STRAIGHT
  int dir;
} segment_t;

typedef struct {
  // NOTE: for internal message passing, either
  // RESERVOIR_REQUEST or RESERVOIR_RELEASE
  packet_t packet;

  // Reasonable upper limit to request or release at one time
  segment_t segments[12];
  int len;
} reservoir_segments_t;

void reservoir_task();

int RequestSegment(reservoir_segments_t * segment);

void ReleaseSegment(reservoir_segments_t * segment);
