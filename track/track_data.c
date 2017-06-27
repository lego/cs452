/* THIS FILE IS GENERATED CODE -- DO NOT EDIT */

#include "track_data.h"
#include <null.h>
#include <debug.h>

static void *memset(void *s, int c, unsigned int n) {
  unsigned char *p = s;
  while(n --> 0) { *p++ = (unsigned char)c; }
  return s;
}

void init_tracktest(track_node *track) {
  memset(track, 0, TRACK_MAX*sizeof(track_node));
  track[0].name = "MR";
  track[0].id = 0;
  track[0].type = NODE_MERGE;
  track[0].num = 1;
  track[0].reverse = &track[1];
  track[0].edge[DIR_AHEAD].reverse = &track[6].edge[DIR_AHEAD];
  track[0].edge[DIR_AHEAD].src = &track[0];
  track[0].edge[DIR_AHEAD].dest = &track[7];
  track[0].edge[DIR_AHEAD].dist = 60;
  track[1].name = "BR";
  track[1].id = 1;
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
  track[2].id = 2;
  track[2].type = NODE_ENTER;
  track[2].reverse = &track[3];
  track[2].edge[DIR_AHEAD].reverse = &track[1].edge[DIR_STRAIGHT];
  track[2].edge[DIR_AHEAD].src = &track[2];
  track[2].edge[DIR_AHEAD].dest = &track[0];
  track[2].edge[DIR_AHEAD].dist = 110;
  track[3].name = "EX1";
  track[3].id = 3;
  track[3].type = NODE_EXIT;
  track[3].reverse = &track[2];
  track[4].name = "EN2";
  track[4].id = 4;
  track[4].type = NODE_ENTER;
  track[4].reverse = &track[5];
  track[4].edge[DIR_AHEAD].reverse = &track[1].edge[DIR_CURVED];
  track[4].edge[DIR_AHEAD].src = &track[4];
  track[4].edge[DIR_AHEAD].dest = &track[0];
  track[4].edge[DIR_AHEAD].dist = 90;
  track[5].name = "EX2";
  track[5].id = 5;
  track[5].type = NODE_EXIT;
  track[5].reverse = &track[4];
  track[6].name = "EN3";
  track[6].id = 6;
  track[6].type = NODE_ENTER;
  track[6].reverse = &track[7];
  track[6].edge[DIR_AHEAD].reverse = &track[0].edge[DIR_AHEAD];
  track[6].edge[DIR_AHEAD].src = &track[6];
  track[6].edge[DIR_AHEAD].dest = &track[1];
  track[6].edge[DIR_AHEAD].dist = 60;
  track[7].name = "EX3";
  track[7].id = 7;
  track[7].type = NODE_EXIT;
  track[7].reverse = &track[6];
}

void init_tracktest_route_table(track_node *track, track_edge *(*route_table)[TRACK_MAX][TRACK_MAX]) {

  // SECTION for MR
  (*route_table)[0][2] = &track[1].edge[DIR_STRAIGHT]; // MR to EN1
  (*route_table)[0][4] = &track[1].edge[DIR_CURVED]; // MR to EN2
  (*route_table)[0][6] = &track[0].edge[DIR_AHEAD]; // MR to EN3
  (*route_table)[0][5] = &track[1].edge[DIR_CURVED]; // MR to EX2
  (*route_table)[0][7] = &track[0].edge[DIR_AHEAD]; // MR to EX3
  (*route_table)[0][1] = NULL; // MR to BR
  (*route_table)[0][3] = &track[1].edge[DIR_STRAIGHT]; // MR to EX1

  // SECTION for BR
  (*route_table)[1][0] = NULL; // BR to MR
  (*route_table)[1][2] = &track[1].edge[DIR_STRAIGHT]; // BR to EN1
  (*route_table)[1][4] = &track[1].edge[DIR_CURVED]; // BR to EN2
  (*route_table)[1][6] = &track[0].edge[DIR_AHEAD]; // BR to EN3
  (*route_table)[1][5] = &track[1].edge[DIR_CURVED]; // BR to EX2
  (*route_table)[1][7] = &track[0].edge[DIR_AHEAD]; // BR to EX3
  (*route_table)[1][3] = &track[1].edge[DIR_STRAIGHT]; // BR to EX1

  // SECTION for EN1
  (*route_table)[2][3] = NULL; // EN1 to EX1
  (*route_table)[2][4] = &track[1].edge[DIR_CURVED]; // EN1 to EN2
  (*route_table)[2][6] = &track[0].edge[DIR_AHEAD]; // EN1 to EN3
  (*route_table)[2][5] = &track[1].edge[DIR_CURVED]; // EN1 to EX2
  (*route_table)[2][7] = &track[0].edge[DIR_AHEAD]; // EN1 to EX3
  (*route_table)[2][1] = &track[2].edge[DIR_AHEAD]; // EN1 to BR
  (*route_table)[2][0] = &track[2].edge[DIR_AHEAD]; // EN1 to MR

  // SECTION for EX1
  (*route_table)[3][2] = NULL; // EX1 to EN1
  (*route_table)[3][4] = &track[1].edge[DIR_CURVED]; // EX1 to EN2
  (*route_table)[3][6] = &track[0].edge[DIR_AHEAD]; // EX1 to EN3
  (*route_table)[3][5] = &track[1].edge[DIR_CURVED]; // EX1 to EX2
  (*route_table)[3][7] = &track[0].edge[DIR_AHEAD]; // EX1 to EX3
  (*route_table)[3][1] = &track[2].edge[DIR_AHEAD]; // EX1 to BR
  (*route_table)[3][0] = &track[2].edge[DIR_AHEAD]; // EX1 to MR

  // SECTION for EN2
  (*route_table)[4][0] = &track[4].edge[DIR_AHEAD]; // EN2 to MR
  (*route_table)[4][2] = &track[1].edge[DIR_STRAIGHT]; // EN2 to EN1
  (*route_table)[4][6] = &track[0].edge[DIR_AHEAD]; // EN2 to EN3
  (*route_table)[4][5] = NULL; // EN2 to EX2
  (*route_table)[4][7] = &track[0].edge[DIR_AHEAD]; // EN2 to EX3
  (*route_table)[4][1] = &track[4].edge[DIR_AHEAD]; // EN2 to BR
  (*route_table)[4][3] = &track[1].edge[DIR_STRAIGHT]; // EN2 to EX1

  // SECTION for EX2
  (*route_table)[5][0] = &track[4].edge[DIR_AHEAD]; // EX2 to MR
  (*route_table)[5][2] = &track[1].edge[DIR_STRAIGHT]; // EX2 to EN1
  (*route_table)[5][4] = NULL; // EX2 to EN2
  (*route_table)[5][6] = &track[0].edge[DIR_AHEAD]; // EX2 to EN3
  (*route_table)[5][7] = &track[0].edge[DIR_AHEAD]; // EX2 to EX3
  (*route_table)[5][1] = &track[4].edge[DIR_AHEAD]; // EX2 to BR
  (*route_table)[5][3] = &track[1].edge[DIR_STRAIGHT]; // EX2 to EX1

  // SECTION for EN3
  (*route_table)[6][0] = &track[6].edge[DIR_AHEAD]; // EN3 to MR
  (*route_table)[6][2] = &track[1].edge[DIR_STRAIGHT]; // EN3 to EN1
  (*route_table)[6][4] = &track[1].edge[DIR_CURVED]; // EN3 to EN2
  (*route_table)[6][5] = &track[1].edge[DIR_CURVED]; // EN3 to EX2
  (*route_table)[6][7] = NULL; // EN3 to EX3
  (*route_table)[6][1] = &track[6].edge[DIR_AHEAD]; // EN3 to BR
  (*route_table)[6][3] = &track[1].edge[DIR_STRAIGHT]; // EN3 to EX1

  // SECTION for EX3
  (*route_table)[7][0] = &track[6].edge[DIR_AHEAD]; // EX3 to MR
  (*route_table)[7][2] = &track[1].edge[DIR_STRAIGHT]; // EX3 to EN1
  (*route_table)[7][4] = &track[1].edge[DIR_CURVED]; // EX3 to EN2
  (*route_table)[7][6] = NULL; // EX3 to EN3
  (*route_table)[7][5] = &track[1].edge[DIR_CURVED]; // EX3 to EX2
  (*route_table)[7][1] = &track[6].edge[DIR_AHEAD]; // EX3 to BR
  (*route_table)[7][3] = &track[1].edge[DIR_STRAIGHT]; // EX3 to EX1
}

int init_tracktest_name_to_node(char *name) {
switch (name[0]) {
        case 'E':
      switch (name[1]) {
        case 'X':
      switch (name[2]) {
        case '1':
      switch (name[3]) {
        case '\0':
          return 3;
          break;
        default:
          return -1;
          break;
    }
      break;
case '3':
      switch (name[3]) {
        case '\0':
          return 7;
          break;
        default:
          return -1;
          break;
    }
      break;
case '2':
      switch (name[3]) {
        case '\0':
          return 5;
          break;
        default:
          return -1;
          break;
    }
      break;
        default:
          return -1;
          break;
    }
      break;
case 'N':
      switch (name[2]) {
        case '1':
      switch (name[3]) {
        case '\0':
          return 2;
          break;
        default:
          return -1;
          break;
    }
      break;
case '3':
      switch (name[3]) {
        case '\0':
          return 6;
          break;
        default:
          return -1;
          break;
    }
      break;
case '2':
      switch (name[3]) {
        case '\0':
          return 4;
          break;
        default:
          return -1;
          break;
    }
      break;
        default:
          return -1;
          break;
    }
      break;
        default:
          return -1;
          break;
    }
      break;
case 'B':
      switch (name[1]) {
        case 'R':
      switch (name[2]) {
        case '\0':
          return 1;
          break;
        default:
          return -1;
          break;
    }
      break;
        default:
          return -1;
          break;
    }
      break;
case 'M':
      switch (name[1]) {
        case 'R':
      switch (name[2]) {
        case '\0':
          return 0;
          break;
        default:
          return -1;
          break;
    }
      break;
        default:
          return -1;
          break;
    }
      break;
        default:
          return -1;
          break;
    }}
