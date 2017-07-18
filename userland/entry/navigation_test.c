#include <bwio.h>
#include <idle_task.h>
#include <priorities.h>
#include <track/pathing.h>
#include <trains/navigation.h>
#include <servers/nameserver.h>
#include <servers/clock_server.h>
#include <detective/sensor_detector_multiplexer.h>
#include <trains/reservoir.h>
#include <trains/sensor_collector.h>
#include <trains/train_controller.h>
#include <trains/switch_controller.h>
#include <kernel.h>
#include <terminal.h>

int ticker = 1;

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
        bwprintf(COM2, RED_BG "mock_executor" RESET_ATTRIBUTES ": got route failure from %d trying to get to %s\n", failure->train, track[failure->dest_id].name);
        break;
    }
  }
}

void mock_switch_controller() {
  RegisterAs(NS_SWITCH_CONTROLLER);
  int sender;
  char buffer[1024] __attribute__((aligned(4)));
  switch_control_request_t * request = (switch_control_request_t *)buffer;
  while (true) {
    ReceiveS(&sender, buffer);
    if (request->type == SWITCH_GET) {
      int state = SWITCH_STRAIGHT;
      ReplyS(sender, state);
    } else if (request->type == SWITCH_SET) {
      bwprintf(COM2, "mock_switch_controller: would have set switch %d to %s\n", request->index, request->value == SWITCH_CURVED ? "curved" : "straight");
      ReplyN(sender);
    }
  }
}

void ProvideSensorTrigger(int sensor_no) {
  int tid = WhoIs(NS_SENSOR_ATTRIBUTER);
  sensor_data_t data;
  data.packet.type = SENSOR_DATA;
  data.sensor_no = sensor_no;
  data.timestamp = ticker++;
  SendSN(tid, data);
}

void navigation_test_task() {
  InitPathing();
  InitNavigation();
  InitTrainControllers();

  Create(PRIORITY_NAMESERVER, nameserver);
  Create(PRIORITY_IDLE_TASK, idle_task);
  Create(4, sensor_attributer);
  Create(5, reservoir_task);
  Create(6, mock_executor);
  Create(7, mock_switch_controller);

  path_t train20_path;
  path_t train50_path;
  GetPath(&train20_path, Name2Node("C5"), Name2Node("C14"));
  GetPath(&train50_path, Name2Node("C13"), Name2Node("C6"));
  PrintPath(&train20_path);

  NavigateTrain(20, 10, &train20_path);

  // Train 20 sensors
  ProvideSensorTrigger(Name2Node("C15"));

  // Start train 50 (after 20 was attributed)
  NavigateTrain(50, 10, &train50_path);

  // Fails for train 20, keep moving it
  ProvideSensorTrigger(Name2Node("D12"));
  ProvideSensorTrigger(Name2Node("E11"));
  ProvideSensorTrigger(Name2Node("D10"));

  // Train 50 sensors
  ProvideSensorTrigger(Name2Node("E7"));
  ProvideSensorTrigger(Name2Node("D7"));
  ProvideSensorTrigger(Name2Node("MR9"));

  ExitKernel();
}
