#include <bwio.h>
#include <idle_task.h>
#include <priorities.h>
#include <track/pathing.h>
#include <trains/navigation.h>
#include <servers/nameserver.h>
#include <servers/clock_server.h>
#include <detective/sensor_detector_multiplexer.h>
#include <trains/train_controller.h>
#include <trains/route_executor.h>
#include <trains/reservoir.h>
#include <trains/sensor_collector.h>
#include <kernel.h>

void attrib_test_ProvideSensorTrigger(int sensor_no) {
  int tid = WhoIs(NS_SENSOR_ATTRIBUTER);
  sensor_data_t data;
  data.packet.type = SENSOR_DATA;
  data.sensor_no = sensor_no;
  data.timestamp = Time();
  SendSN(tid, data);
}

#define ProvideSensorTrigger attrib_test_ProvideSensorTrigger

void attribution_test_task() {
  InitPathing();
  InitNavigation();

  Create(PRIORITY_NAMESERVER, nameserver);
  Create(PRIORITY_IDLE_TASK, idle_task);
  Create(4, sensor_attributer);
  Create(4, reservoir_task);

  CreateTrainController(10);

  ProvideSensorTrigger(Name2Node("C15"));

  // ProvideSensorTrigger(Name2Node("D12"));
  // ProvideSensorTrigger(Name2Node("E11"));
  // // Should fail
  // ProvideSensorTrigger(Name2Node("D10"));

  ExitKernel();
}
