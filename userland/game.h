#pragma once

typedef struct {
  // A bit polymorphic (multi use)
  // type = GAME_ADD_TRAIN
  // type = GAME_STATION_ARRIVAL
  packet_t packet;
  int train;
  int station;
} station_arrival_t;

void game_task();
