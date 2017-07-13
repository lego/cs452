#pragma once

#define BASIS_NODE "A4"
#define BASIS_NODE_NAME Name2Node(BASIS_NODE)
#define DECLARE_BASIS_NODE(name) int name = BASIS_NODE_NAME

void sensor_saver_task();
// for manually triggering in interactive via. command
void TriggerSensor(int sensor_num, int sensor_time);
