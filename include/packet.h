#pragma once

typedef enum {
  SENSOR_DATA,
  DELAY_DETECT,
  SENSOR_DETECT,
} packet_type_t;

typedef struct {
  packet_type_t type;
  // length of data following this struct
  int len;
} packet_t;


/**
 * Gets the data from a packet
 * NOTE: this could maybe assert sizeof(type) == len
 * @param  packet_ptr to the packet
 * @param  type       to typecast the data to
 * @return            a pointer
 */
#define GetPacketData(packet_ptr, type) (type *) (packet_ptr + sizeof(packet_t))
