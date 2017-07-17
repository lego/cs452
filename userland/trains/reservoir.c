#include <basic.h>
#include <kernel.h>
#include <util.h>
#include <trains/reservoir.h>
#include <track/pathing.h>
#include <servers/uart_tx_server.h>
#include <servers/clock_server.h>

int reservoir_tid = -1;

typedef struct {
  // type = RESERVOIR_PATHING_REQUEST
  packet_t packet;
  int train;
  int src_node;
  int dest_node;
} pathing_request_t;

static inline int get_segment_owner(segment_t * segment) {
  if (track[segment->track_node].edge[segment->dir].owner != -1)
    return track[segment->track_node].edge[segment->dir].owner;
  else if (track[segment->track_node].edge[segment->dir].reverse->owner != -1)
    return track[segment->track_node].edge[segment->dir].reverse->owner;
  else
    return -1;
}

bool all_segments_available(reservoir_segments_t * request, int owner) {
  for (int i = 0; i < request->len; i++) {
    int segment_owner = get_segment_owner(&request->segments[i]);
    if (segment_owner != -1 && segment_owner != owner) return false;
  }
  return true;
}

void set_segment_owner(segment_t * segment, int owner) {
  track[segment->track_node].edge[segment->dir].owner = owner;
}

void put_resevoir_packet(int type, reservoir_segments_t * request, int owner) {
  int time = Time();
  char message_buffer[32] __attribute__ ((aligned (4)));;
  uart_packet_t * packet = (uart_packet_t *) message_buffer;
  char * packet_data = message_buffer + sizeof(uart_packet_t);
  packet->type = type;
  packet->len = 5 + request->len*2;
  jmemcpy(&packet_data[0], &time, sizeof(int));
  packet_data[4] = owner;
  for (int i = 0; i < request->len; i++) {
    packet_data[5+2*i] = (char)request->segments[i].track_node;
    packet_data[5+2*i+1] = (char)track[request->segments[i].track_node].edge[request->segments[i].dir].dest->id;
  }
  PutPacket(packet);
}

void set_segment_ownership(reservoir_segments_t * request, int owner) {
  for (int i = 0; i < request->len; i++) {
    set_segment_owner(&request->segments[i], owner);
  }
  put_resevoir_packet(PACKET_RESEVOIR_SET_DATA, request, owner);
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
  put_resevoir_packet(PACKET_RESEVOIR_UNSET_DATA, request, owner);
}

static void process_pathing_request(path_t * path, pathing_request_t * request, int sender) {
  // FIXME: take into account train ownership
  GetPath(path, request->src_node, request->dest_node);
}

void reservoir_task() {
  int tid = MyTid();
  reservoir_tid = tid;

  char request_buffer[256] __attribute__ ((aligned (4)));
  KASSERT(sizeof(request_buffer) >= sizeof(reservoir_segments_t), "Buffer isn't large enough.");
  KASSERT(sizeof(request_buffer) >= sizeof(pathing_request_t), "Buffer isn't large enough.");
  packet_t * packet = (packet_t *) request_buffer;
  reservoir_segments_t * resv_request = (reservoir_segments_t *) request_buffer;
  pathing_request_t * path_request = (pathing_request_t *) request_buffer;
  path_t result_path;

  int sender;
  while (true) {
    ReceiveS(&sender, request_buffer);
    switch (packet->type) {
    case RESERVOIR_REQUEST:
      if (all_segments_available(resv_request, resv_request->owner)) {
        set_segment_ownership(resv_request, resv_request->owner);
        ReplyStatus(sender, RESERVOIR_REQUEST_OK);
      } else {
        ReplyStatus(sender, RESERVOIR_REQUEST_ERROR);
      }
      break;
    case RESERVOIR_RELEASE:
      release_segments(resv_request, resv_request->owner);
      ReplyN(sender);
      break;
    case RESERVOIR_PATHING_REQUEST:
      process_pathing_request(&result_path, path_request, resv_request->owner);
      // FIXME: if no path found due to ownership, ReplyN
      ReplyS(sender, result_path);
      break;
    }
  }
}

int RequestSegment(reservoir_segments_t *segment) {
  KASSERT(reservoir_tid >= 0, "Reservoir not started");
  KASSERT(segment->owner >= 0 && segment->owner < 80, "Segment owner isn't valid train. Got owner=%d", segment->owner);
  KASSERT(segment->len >= 0 && segment->len < RESERVING_LIMIT, "Segment overflowed. size=%d max=%d", segment->len, RESERVING_LIMIT);
  // TODO: kassert valid segments and directions
  segment->packet.type = RESERVOIR_REQUEST;
  int result;
  Send(reservoir_tid, segment, sizeof(reservoir_segments_t), &result, sizeof(result));
  return result;
}

void edge_segments_to_segments(reservoir_segment_edges_t *segment, reservoir_segments_t *actual_segment) {
  for (int i = 0; i < segment->len; i++) {
    actual_segment->segments[i].track_node = segment->edges[i]->src->id;
    actual_segment->segments[i].dir = (&segment->edges[i]->src->edge[DIR_AHEAD] == segment->edges[i]) ? DIR_AHEAD : DIR_CURVED;
  }

  actual_segment->len = segment->len;
  actual_segment->owner = segment->owner;
}

int RequestSegmentEdges(reservoir_segment_edges_t *segment) {
  KASSERT(reservoir_tid >= 0, "Reservoir not started");
  KASSERT(segment->owner >= 0 && segment->owner < 80, "Segment owner isn't valid train. Got owner=%d", segment->owner);
  KASSERT(segment->len >= 0 && segment->len < RESERVING_LIMIT, "Segment overflowed. size=%d max=%d", segment->len, RESERVING_LIMIT);
  // TODO: kassert valid segments and directions

  reservoir_segments_t actual_segment;
  edge_segments_to_segments(segment, &actual_segment);

  actual_segment.packet.type = RESERVOIR_REQUEST;
  int result;
  SendS(reservoir_tid, actual_segment, result);
  return result;
}

void ReleaseSegment(reservoir_segments_t *segment) {
  KASSERT(reservoir_tid >= 0, "Reservoir not started");
  KASSERT(segment->len >= 0 && segment->len < RESERVING_LIMIT, "Segment overflowed. size=%d max=%d", segment->len, RESERVING_LIMIT);
  KASSERT(segment->owner >= 0 && segment->owner < 80, "Segment owner isn't valid train. Got owner=%d", segment->owner);
  // TODO: kassert valid segments and directions
  segment->packet.type = RESERVOIR_RELEASE;
  Send(reservoir_tid, segment, sizeof(reservoir_segments_t), NULL, 0);
}

void ReleaseSegmentEdges(reservoir_segment_edges_t *segment) {
  KASSERT(reservoir_tid >= 0, "Reservoir not started");
  KASSERT(segment->len >= 0 && segment->len < RESERVING_LIMIT, "Segment overflowed. size=%d max=%d", segment->len, RESERVING_LIMIT);
  KASSERT(segment->owner >= 0 && segment->owner < 80, "Segment owner isn't valid train. Got owner=%d", segment->owner);

  reservoir_segments_t actual_segment;
  edge_segments_to_segments(segment, &actual_segment);

  actual_segment.packet.type = RESERVOIR_RELEASE;
  Send(reservoir_tid, &actual_segment, sizeof(reservoir_segments_t), NULL, 0);
}


int RequestPath(path_t * output, int train, int src_node, int dest_node) {
  KASSERT(reservoir_tid >= 0, "Reservoir not started");
  pathing_request_t request;
  request.packet.type = RESERVOIR_PATHING_REQUEST;
  request.train = train;
  request.src_node = src_node;
  request.dest_node = dest_node;
  Send(reservoir_tid, &request, sizeof(reservoir_segments_t), output, sizeof(path_t));
  // FIXME: get pathing result
  // maybe if 0 bytes written we've failed?
  return 0;
}

int WhoOwnsSegment(int node) {
  // Because we're not doing much in this, we shouldn't need to go do a Send
  // TODO: KASSERT for the node?
  segment_t segment;
  segment.track_node = node;
  segment.dir = DIR_AHEAD;
  // FIXME: a bit of an assumption here: segments are edges, so technically
  // there could be two different edge owners
  return get_segment_owner(&segment);
}
