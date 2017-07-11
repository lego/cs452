#include <basic.h>
#include <kernel.h>
#include <trains/resevoir.h>

int resevoir_tid = -1;


bool all_segments_available(segment_t * request) {
  return true;
}

void set_segment_ownership(int owner, segment_t * request) {
  // KASSERT all segments are owned (do we need to?)

}

void release_segments(int owner, segment_t * request) {
  // KASSERT all segments are owned (do we need to?)

}

void resevoir_task() {
  int tid = MyTid();
  resevoir_tid = tid;
  segment_t request;
  int sender;
  while (true) {
    ReceiveS(&sender, request);
    switch (request.packet.type) {
    case RESEVOIR_REQUEST:
      if (all_segments_available(&request)) {
        set_segment_ownership(sender, &request);
        ReplyStatus(sender, RESEVOIR_REQUEST_OK);
      } else {
        ReplyStatus(sender, RESEVOIR_REQUEST_ERROR);
      }
      break;
    case RESEVOIR_RELEASE:
      release_segments(sender, &request);
      break;
    }
  }
}

int RequestSegment(segment_t *segment) {
  KASSERT(resevoir_tid >= 0, "Resevoir not started");
  segment->packet.type = RESEVOIR_REQUEST;
  int result;
  Send(resevoir_tid, segment, sizeof(segment_t), &result, sizeof(result));
  return result;
}

void ReleaseSegment(segment_t *segment) {
  KASSERT(resevoir_tid >= 0, "Resevoir not started");
  segment->packet.type = RESEVOIR_RELEASE;
  Send(resevoir_tid, segment, sizeof(segment_t), NULL, 0);
}
