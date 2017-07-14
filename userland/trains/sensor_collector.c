#include <basic.h>
#include <bwio.h>
#include <servers/nameserver.h>
#include <servers/clock_server.h>
#include <servers/uart_rx_server.h>
#include <servers/uart_tx_server.h>
#include <trains/sensor_collector.h>

void sensor_collector_task() {
  int tid = MyTid();
  int parent = MyParentTid();
  int sensor_saver_tid = WhoIsEnsured(SENSOR_SAVER);
  sensor_data_t req;
  req.packet.type = SENSOR_DATA;
  log_task("sensor_reader initialized parent=%d", tid, parent);
  int oldSensors[5];
  int sensors[5];
  Delay(50); // Wait half a second for old COM1 input to be read
  while (true) {
    log_task("sensor_reader sleeping", tid);
    ClearRx(COM1);
    Putc(COM1, 0x85);
    int queueLength = 0;
    const int maxTries = 100;
    int i;
    for (i = 0; i < maxTries && queueLength < 10; i++) {
      Delay(1);
      queueLength = GetRxQueueLength(COM1);
    }
    // We tried to read but failed to get the correct number of bytes, ABORT
    if (i == maxTries) {
      continue;
    }
    log_task("sensor_reader reading", tid);
    for (int i = 0; i < 5; i++) {
      char high = Getc(COM1);
      char low = Getc(COM1);
      sensors[i] = (high << 8) | low;
    }
    log_task("sensor_reader read", tid);
    for (int i = 0; i < 5; i++) {
      if (sensors[i] != oldSensors[i]) {
        for (int j = 0; j < 16; j++) {
          if ((sensors[i] & (1 << j)) & ~(oldSensors[i] & (1 << j))) {
            req.sensor_no = i*16+(15-j);


            // Send to all sensor subscribers
            // right now that's just the sensor saver
            SendSN(sensor_saver_tid, req);
          }
        }
      }
      oldSensors[i] = sensors[i];
    }
  }
}
