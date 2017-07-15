#pragma once

void train_command_server();

int SetTrainSpeed(int train, int speed);
int MoveTrain(int train, int speed, int node_id);
int ReverseTrain(int train, int currentSpeed);
int InstantStop(int train);
