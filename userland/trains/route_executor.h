#pragma once

#include <packet.h>
#include <track/pathing.h>

typedef struct {
  // type = ROUTE_FAILURE
  packet_t packet;

  int train;
  int location;
} route_failure_t;

/**
 * The Route Executor tracks information for a particular route that is
 * curently being run by a train.
 *
 * In particular, it currently takes the responsibilty of reserving and releasing
 * segments of owned track during the traversal of a route.
 *
 * It does this using sensor detectors to detect along the route, so it's not
 * very robust.
 */

/**
 * Creates a route executor to execute a train's route
 * @param  priority to start the executor at
 * @param  train    running the path
 * @param  path     to run
 * @return          ??
 */
int CreateRouteExecutor(int priority, int train, path_t * path);
