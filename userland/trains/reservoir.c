#include <basic.h>
#include <kernel.h>
#include <trains/reservoir.h>

int reservoir_tid = -1;


bool all_segments_available(segment_t * request) {
  return true;
}

void set_segment_ownership(int owner, segment_t * request) {
  // KASSERT all segments are owned (do we need to?)

}

void release_segments(int owner, segment_t * request) {
  // KASSERT all segments are owned (do we need to?)

}

void reservoir_task() {
  int tid = MyTid();
  reservoir_tid = tid;
  segment_t request;
  int sender;
  while (true) {
    ReceiveS(&sender, request);
    switch (request.packet.type) {
    case RESERVOIR_REQUEST:
      if (all_segments_available(&request)) {
        set_segment_ownership(sender, &request);
        ReplyStatus(sender, RESERVOIR_REQUEST_OK);
      } else {
        ReplyStatus(sender, RESERVOIR_REQUEST_ERROR);
      }
      break;
    case RESERVOIR_RELEASE:
      release_segments(sender, &request);
      break;
    }
  }
}

int RequestSegment(segment_t *segment) {
  KASSERT(reservoir_tid >= 0, "Reservoir not started");
  segment->packet.type = RESERVOIR_REQUEST;
  int result;
  Send(reservoir_tid, segment, sizeof(segment_t), &result, sizeof(result));
  return result;
}

void ReleaseSegment(segment_t *segment) {
  KASSERT(reservoir_tid >= 0, "Reservoir not started");
  segment->packet.type = RESERVOIR_RELEASE;
  Send(reservoir_tid, segment, sizeof(segment_t), NULL, 0);
}
