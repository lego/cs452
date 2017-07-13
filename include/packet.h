#pragma once

typedef enum {
  /*
   * Sensor collector messages
   */
  SENSOR_DATA,

  /*
   * Detector messages
   */
  DELAY_DETECT,
  SENSOR_DETECT,
  SENSOR_TIMEOUT_DETECTIVE,

  // Messages from command_parser
  PARSED_COMMAND,
  // Messages from command_interpreter
  INTERPRETED_COMMAND,

  /*
   * Messages to Interactive task
   */
  INTERACTIVE_TIME_UPDATE,
  INTERACTIVE_ECHO,

  /**
   * Reservoir messages
   */
  RESERVOIR_REQUEST,
  RESERVOIR_RELEASE,
} packet_type_t;

typedef struct {
  // Left as int, otherwise switch cases must enumerate all cases
  int type;
  // length of data following this struct
  // int len;
} packet_t;


/**
 * Gets the data from a packet
 * NOTE: this could maybe assert sizeof(type) == len
 * @param  packet_ptr to the packet
 * @param  type       to typecast the data to
 * @return            a pointer
 */
#define GetPacketData(packet_ptr, type) (type *) (packet_ptr + sizeof(packet_t))
