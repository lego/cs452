/* THIS FILE IS GENERATED CODE -- DO NOT EDIT */

#include "track_data.h"

static void *memset(void *s, int c, unsigned int n) {
  unsigned char *p = s;
  while(n --> 0) { *p++ = (unsigned char)c; }
  return s;
}

void init_tracktest(track_node *track) {
  memset(track, 0, TRACK_MAX*sizeof(track_node));
  track[0].name = "MR";
  track[0].type = NODE_MERGE;
  track[0].num = 1;
  track[0].reverse = &track[1];
  track[0].edge[DIR_AHEAD].reverse = &track[6].edge[DIR_AHEAD];
  track[0].edge[DIR_AHEAD].src = &track[0];
  track[0].edge[DIR_AHEAD].dest = &track[7];
  track[0].edge[DIR_AHEAD].dist = 60;
  track[1].name = "BR";
  track[1].type = NODE_BRANCH;
  track[1].num = 1;
  track[1].reverse = &track[0];
  track[1].edge[DIR_STRAIGHT].reverse = &track[2].edge[DIR_AHEAD];
  track[1].edge[DIR_STRAIGHT].src = &track[1];
  track[1].edge[DIR_STRAIGHT].dest = &track[3];
  track[1].edge[DIR_STRAIGHT].dist = 110;
  track[1].edge[DIR_CURVED].reverse = &track[4].edge[DIR_AHEAD];
  track[1].edge[DIR_CURVED].src = &track[1];
  track[1].edge[DIR_CURVED].dest = &track[5];
  track[1].edge[DIR_CURVED].dist = 90;
  track[2].name = "EN1";
  track[2].type = NODE_ENTER;
  track[2].reverse = &track[3];
  track[2].edge[DIR_AHEAD].reverse = &track[1].edge[DIR_STRAIGHT];
  track[2].edge[DIR_AHEAD].src = &track[2];
  track[2].edge[DIR_AHEAD].dest = &track[0];
  track[2].edge[DIR_AHEAD].dist = 110;
  track[3].name = "EX1";
  track[3].type = NODE_EXIT;
  track[3].reverse = &track[2];
  track[4].name = "EN2";
  track[4].type = NODE_ENTER;
  track[4].reverse = &track[5];
  track[4].edge[DIR_AHEAD].reverse = &track[1].edge[DIR_CURVED];
  track[4].edge[DIR_AHEAD].src = &track[4];
  track[4].edge[DIR_AHEAD].dest = &track[0];
  track[4].edge[DIR_AHEAD].dist = 90;
  track[5].name = "EX2";
  track[5].type = NODE_EXIT;
  track[5].reverse = &track[4];
  track[6].name = "EN3";
  track[6].type = NODE_ENTER;
  track[6].reverse = &track[7];
  track[6].edge[DIR_AHEAD].reverse = &track[0].edge[DIR_AHEAD];
  track[6].edge[DIR_AHEAD].src = &track[6];
  track[6].edge[DIR_AHEAD].dest = &track[1];
  track[6].edge[DIR_AHEAD].dist = 60;
  track[7].name = "EX3";
  track[7].type = NODE_EXIT;
  track[7].reverse = &track[6];
}
