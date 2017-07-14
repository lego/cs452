#include <trains/reservoir.h>
#include <trains/track_node.h>
#include <trains/navigation.h>
#include <kernel.h>
#include <bwio.h>

void reservoir_test_task() {
  int result;
  InitNavigation();
  Create(0, reservoir_task);
  reservoir_segments_t req;
  req.segments[0].track_node = 0;
  req.segments[0].dir = DIR_AHEAD;
  req.len = 1;
  result = RequestSegment(&req);
  bwprintf(COM2, "Reservation attempt=%d\n\r", result);

  req.segments[1].track_node = 1;
  req.segments[1].dir = DIR_AHEAD;
  req.len = 2;
  result = RequestSegment(&req);
  bwprintf(COM2, "Reservation w/2 but overlapping attempt=%d\n\r", result);

  req.len = 1;
  bwprintf(COM2, "Releasing 2 node\n\r");
  ReleaseSegment(&req);

  req.len = 2;
  result = RequestSegment(&req);
  bwprintf(COM2, "Reservation w/2 after release attempt=%d\n\r", result);

  bwprintf(COM2, "Releasing 2 nodes\n\r");
  ReleaseSegment(&req);

  reservoir_segments_t reverse_req;
  reverse_req.segments[0].track_node = track[0].edge[DIR_AHEAD].reverse->src->id;
  reverse_req.segments[0].dir = DIR_AHEAD;
  reverse_req.len = 1;
  result = RequestSegment(&reverse_req);
  bwprintf(COM2, "Reservation w/reverse node0 attempt=%d\n\r", result);


  result = RequestSegment(&req);
  bwprintf(COM2, "Reservation w/ attempt=%d\n\r", result);

  ExitKernel();
}
