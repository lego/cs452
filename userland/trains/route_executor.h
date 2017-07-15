#pragma once

/**
 * Creates a route executor to execute a train's route
 * @param  priority to start the executor at
 * @param  train    running the path
 * @param  path     to run
 * @return          ??
 */
int CreateRouteExecutor(int priority, int train, path_t * path);
