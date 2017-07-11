#pragma once

#include <stdbool.h>

typedef enum {
  NODE_NONE,
  NODE_SENSOR,
  NODE_BRANCH,
  NODE_MERGE,
  NODE_ENTER,
  NODE_EXIT,
} node_type;

#define DIR_AHEAD 0
#define DIR_STRAIGHT 0
#define DIR_CURVED 1

struct track_node;
typedef struct track_node track_node;
typedef struct track_edge track_edge;

struct track_edge {
  track_edge *reverse;
  track_node *src, *dest;
  int dist;             /* in millimetres */

  // Resevoir data
  int owner;
};

struct track_node {
  const char *name;
  node_type type;
  int id;
  int num;              /* sensor or switch number */
  track_node *reverse;  /* same location, but opposite direction */
  track_edge edge[2];

  // Resevoir data
  int owner;

  // Pathing data, reset on each use of pathing.
  // NOTE: this is not mutually exclusive, and will fail if used concurrently
  int dist;
  int prev;
  bool visited;

  // For displaying the actual vs. expected sensor stuff
  int actual_sensor_trip;
  int expected_time;
};
