#pragma once

#include <basic.h>
#include <track/track_node.h>
#include <track/track_data.h>

/**
 * Path and track graph related functions. This has functions involving:
 *   - generating a path
 *   - interacting with nodes (lookups, finding closest sensor)
 */

#define PATH_MAX 80

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
  int node_dist[PATH_MAX];
} path_t;

typedef struct {
  int node;
  int dist;
} node_dist_t;

// Initializes the track information
void InitPathing();

// Gets the distance between adjacent sensors. Precomputed for speed.
int adjSensorDist(int last, int current);

// Finds the next sensor or branch, returns -1 on error or exit node
node_dist_t findSensorOrBranch(int start);

// Same idea as findSensorOrBranch, but uses current switch state to bypass branches
node_dist_t nextSensor(int start);

void SetPathSwitches(path_t *path);

int StoppingDistance(int train, int speed);

// Generates a path from A to B
#define GetPath(p, src, dest) GetPathWithResv(p, src, dest, -1)

void GetPathWithResv(path_t *p, int src, int dest, int resv_owner);

void PrintPath(path_t *p);

// Navigates a train from A to B
void Navigate(int train, int speed, int src, int dest, bool include_stop);

/**
 * Translates a node name to the node number
 */
int Name2Node(char *name);

/**
 * Gets the reverse node ID for a given node.
 */
int GetReverseNode(int node);

void dijkstra(int src, int dest, int p_idx, int owner);

int get_path(int src, int dest, track_node **path, int path_buf_size, int p_idx);

void GetMultiDestinationPath(path_t *p, int src, int dest1, int dest2);
