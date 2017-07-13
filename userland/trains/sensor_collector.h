#pragma once

#include <packet.h>

typedef struct {
  packet_t packet;
  int sensor_no;
} sensor_data_t;

void sensor_collector_task();
