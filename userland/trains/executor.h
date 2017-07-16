#pragma once

/**
 * Executor accepts incoming commands and executes them
 * This task is responsible for high level command execution, from user input
 * to train re-pathing
 *
 * Recevies:
 *  INTERPRETED_COMMAND from Command Interpreter
 *  PATHING_WORKER_RESULT from an internal Pathing worker (blocks on RequestPath)
 *
 * Starts:
 *  Pathing worker, blocking on Reservoir.RequestPath
 *  Route Executors, executing work on behalf of a route
 *
 */

void executor_task();
