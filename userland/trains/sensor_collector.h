#pragma once

#include <packet.h>

typedef struct {
  // type = SENSOR_DATA
  packet_t packet;
  int sensor_no;
  int timestamp;
} sensor_data_t;

void sensor_attributer();
void sensor_collector_task();

int RegisterTrain(int train);
int RegisterTrainReverse(int train, int lastSensor);
