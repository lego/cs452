/* THIS FILE IS GENERATED CODE -- DO NOT EDIT */
#pragma once


#include "track_node.h"

// The track initialization functions expect an array of this size.
#define TRACK_MAX 8

void init_tracktest(track_node *track);
void init_tracktest_route_table(track_node *track, track_edge *(*route_table)[TRACK_MAX][TRACK_MAX]);
