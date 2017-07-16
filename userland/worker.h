#pragma once

/**
 * Creates an abstract worker to execute a function
 * Useful for one off couriers to do a blocking action, and to return results
 * via. a message to the parent
 * @param  priority    to run the worker (WARNING: this blocks the parent)
 * @param  worker_func to execute
 * @param  data        to pass the worker
 * @return             the worker tid
 */
#define CreateWorker(priority, worker_func, data) _CreateWorker(priority, worker_func, &data, sizeof(data));

int _CreateWorker(int priority, void (*worker_func)(int, void *), void *data, int data_len);
