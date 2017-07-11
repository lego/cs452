#pragma once

#include <packet.h>

#define RESEVOIR_REQUEST_OK 1
#define RESEVOIR_REQUEST_ERROR 0

typedef struct {
  // For internal message passing
  packet_t packet;

  // Reasonable upper limit to request at one time
  int tracks[12];
  int len;
} segment_t;

void resevoir_task();

int RequestSegment(segment_t * segment);

void ReleaseSegment(segment_t * segment);
