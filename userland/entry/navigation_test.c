#include <bwio.h>
#include <idle_task.h>
#include <priorities.h>
#include <track/pathing.h>
#include <trains/navigation.h>
#include <servers/nameserver.h>
#include <servers/clock_server.h>
#include <detective/sensor_detector_multiplexer.h>
#include <trains/route_executor.h>
#include <trains/reservoir.h>
#include <trains/sensor_collector.h>
#include <kernel.h>

void mock_executor() {
  RegisterAs(NS_EXECUTOR);
  char buffer[1024] __attribute__((aligned(4)));
  packet_t *packet = (packet_t *) buffer;
  route_failure_t *failure = (route_failure_t *) buffer;
  int sender;
  while (true) {
    ReceiveS(&sender, buffer);
    ReplyN(sender);
    switch (packet->type) {
      case ROUTE_FAILURE:
        bwprintf(COM2, "Executor got route failure from %d at %d\n", failure->train, failure->location);
        break;
    }
  }
}

void ProvideSensorTrigger(int sensor_no) {
  int tid = WhoIs(NS_SENSOR_DETECTOR_MULTIPLEXER);
  sensor_data_t data;
  data.packet.type = SENSOR_DATA;
  data.sensor_no = sensor_no;
  data.timestamp = Time();
  SendSN(tid, data);
}

void navigation_test_task() {
  InitPathing();
  InitNavigation();

  Create(PRIORITY_NAMESERVER, nameserver);
  Create(PRIORITY_IDLE_TASK, idle_task);
  Create(4, sensor_detector_multiplexer_task);
  Create(4, reservoir_task);
  Create(4, mock_executor);

  path_t train20_path;
  path_t train50_path;
  GetPath(&train20_path, Name2Node("C5"), Name2Node("C14"));
  GetPath(&train50_path, Name2Node("C13"), Name2Node("C6"));

  CreateRouteExecutor(10, 20, 10, ROUTE_EXECUTOR_NAVIGATE, &train20_path);
  CreateRouteExecutor(10, 50, 10, ROUTE_EXECUTOR_STOPFROM, &train50_path);

  ProvideSensorTrigger(Name2Node("C15"));
  ProvideSensorTrigger(Name2Node("D12"));
  ProvideSensorTrigger(Name2Node("E11"));
  // Should fail
  ProvideSensorTrigger(Name2Node("D10"));

  ExitKernel();
}
