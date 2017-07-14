#pragma once

#include <packet.h>
#include <interactive/command_parser.h>

typedef struct {
  // INTERACTIVE_ECHO
  packet_t packet;

  char echo[4];
} interactive_echo_t;

typedef struct {
  // INTERACTIVE_TIME_UPDATE
  packet_t packet;
} interactive_time_update_t;
