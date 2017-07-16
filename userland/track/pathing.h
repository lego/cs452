#pragma once

#include <basic.h>
#include <track/track_data.h>
#include <track/track_node.h>

/**
 * Path and track graph related functions. This has functions involving:
 *   - generating a path
 *   - interacting with nodes (lookups, finding closest sensor)
 */

#define PATH_MAX 80

#define VELOCITY_SAMPLES_MAX 5

// used in interactive and trains/executor for the old stopping and navigation
// TODO: remove all usage
#define BASIS_NODE "A4"
#define BASIS_NODE_NAME Name2Node(BASIS_NODE)
#define DECLARE_BASIS_NODE(name) int name = BASIS_NODE_NAME

extern track_node track[TRACK_MAX];

typedef struct Path {
  int dist; /* in millimetres */
  int len;
  track_node *src;
  track_node *dest;
  track_node *nodes[PATH_MAX];
} path_t;

// Initializes the track information
void InitPathing();

// Finds the next sensor or branch, returns -1 on error or exit node
int findSensorOrBranch(int start);

void SetPathSwitches(path_t *path);

int StoppingDistance(int train, int speed);

// Generates a path from A to B
void GetPath(path_t *p, int src, int dest);

void PrintPath(path_t *p);

// Navigates a train from A to B
void Navigate(int train, int speed, int src, int dest, bool include_stop);

int Name2Node(char *name);

void dijkstra(int src, int dest);

int get_path(int src, int dest, track_node **path, int path_buf_size);

void GetMultiDestinationPath(path_t *p, int src, int dest1, int dest2);
