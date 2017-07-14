#include <basic.h>
#include <kernel.h>
#include <trains/reservoir.h>
#include <track/pathing.h>

int reservoir_tid = -1;

static inline int get_segment_owner(segment_t * segment) {
  if (track[segment->track_node].edge[segment->dir].owner != -1)
    return track[segment->track_node].edge[segment->dir].owner;
  else if (track[segment->track_node].edge[segment->dir].reverse->owner != -1)
    return track[segment->track_node].edge[segment->dir].reverse->owner;
  else
    return -1;
}

bool all_segments_available(reservoir_segments_t * request) {
  for (int i = 0; i < request->len; i++) {
    if (get_segment_owner(&request->segments[i]) != -1) return false;
  }
  return true;
}

void set_segment_owner(segment_t * segment, int owner) {
  track[segment->track_node].edge[segment->dir].owner = owner;
}

void set_segment_ownership(reservoir_segments_t * request, int owner) {
  for (int i = 0; i < request->len; i++) {
    set_segment_owner(&request->segments[i], owner);
  }
}

void release_segments(reservoir_segments_t * request, int owner) {
  for (int i = 0; i < request->len; i++) {
    KASSERT(get_segment_owner(&request->segments[i]) == owner, "Attempted to release segment not owned. node=%d dir=%d name=%s owner=%d actual_owner=%d",
      request->segments[i].track_node,
      request->segments[i].dir,
      track[request->segments[i].track_node].name,
      owner,
      get_segment_owner(&request->segments[i]));
    set_segment_owner(&request->segments[i], -1);
  }
}

void reservoir_task() {
  int tid = MyTid();
  reservoir_tid = tid;
  reservoir_segments_t request;
  int sender;
  while (true) {
    ReceiveS(&sender, request);
    switch (request.packet.type) {
    case RESERVOIR_REQUEST:
      if (all_segments_available(&request)) {
        set_segment_ownership(&request, sender);
        ReplyStatus(sender, RESERVOIR_REQUEST_OK);
      } else {
        ReplyStatus(sender, RESERVOIR_REQUEST_ERROR);
      }
      break;
    case RESERVOIR_RELEASE:
      release_segments(&request, sender);
      ReplyN(sender);
      break;
    }
  }
}


int RequestSegment(reservoir_segments_t *segment) {
  KASSERT(reservoir_tid >= 0, "Reservoir not started");
  KASSERT(segment->len > 0 && segment->len < 12, "Segment overflowed");
  // TODO: kassert valid segments and directions
  segment->packet.type = RESERVOIR_REQUEST;
  int result;
  Send(reservoir_tid, segment, sizeof(reservoir_segments_t), &result, sizeof(result));
  return result;
}

void ReleaseSegment(reservoir_segments_t *segment) {
  KASSERT(reservoir_tid >= 0, "Reservoir not started");
  KASSERT(segment->len > 0 && segment->len < 12, "Segment overflowed");
  // TODO: kassert valid segments and directions
  segment->packet.type = RESERVOIR_RELEASE;
  Send(reservoir_tid, segment, sizeof(reservoir_segments_t), NULL, 0);
}
