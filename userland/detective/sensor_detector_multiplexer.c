#include <kernel.h>
#include <servers/clock_server.h>
#include <servers/nameserver.h>
#include <track/pathing.h>
#include <trains/sensor_collector.h>
#include <cbuffer.h>
#include <packet.h>

#define BUFFER_SIZE 50

void sensor_detector_multiplexer_task() {
  int tid = MyTid();
  RegisterAs(NS_SENSOR_DETECTOR_MULTIPLEXER);

  void *sensor_detectors_buffer[BUFFER_SIZE];
  cbuffer_t sensor_detectors;
  cbuffer_init(&sensor_detectors, sensor_detectors_buffer, BUFFER_SIZE);

  int sender;

  char request_buffer[128] __attribute__((aligned(4)));
  packet_t *packet = (packet_t *)request_buffer;
  sensor_data_t *data = (sensor_data_t *)request_buffer;

  while (true) {
    ReceiveS(&sender, request_buffer);
    switch (packet->type) {
    case SENSOR_DETECTOR_REQUEST:
      cbuffer_add(&sensor_detectors, (void *)sender);
      RecordLogf("Multiplexer got detector=%d\n\r", sender);
      break;
    case SENSOR_DATA:
      // Forward sensor data to all detectors!
      ReplyN(sender);
      RecordLogf("Multiplexer got sensor=%s at uptime=%d\n\r", track[data->sensor_no].name, Time());
      while (cbuffer_size(&sensor_detectors) > 0) {
        int detector = (int)cbuffer_pop(&sensor_detectors, NULL); // Oops, ignore the error, surely fine
        Reply(detector, data, sizeof(sensor_data_t));             // pretty illegal send size, don't do this at home kids
      }
      break;
    }
  }
}
