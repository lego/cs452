#pragma once

typedef struct {

} sensor_data_t;

/**
 * Task
 */
void sensor_server();

/**
 * Subscribes the current task to receive sensor data
 * When data is received it is _eventually_ delivered it via. a courier
 */
void SensorSubscribe();

/**
 * Unsubscribes this task from receiving data
 */
void SensorUnsubscribe();
