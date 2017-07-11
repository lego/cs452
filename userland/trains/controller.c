#include <basic.h>
#include <packet.h>
#include <trains/controller.h>

void reserve_more_track() {

}

void train_controller() {
  char buffer[1024];
  packet_t * packet = (packet_t *) buffer;

  int sender;

  while (true) {
    int bytes = Receive(&sender, buffer, 1024);
    KASSERT(bytes < 1024, "Buffer overflow sender=%d packet_type=%d", sender, packet->type);
    switch (packet->type) {
      case SENSOR_DATA:
        reserve_more_track();
        break;
      default:
        KASSERT(false, "Received unhandled data packet_type=%d", packet->type);
        break;
    }
  }
}
