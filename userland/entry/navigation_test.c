#include <bwio.h>
#include <priorities.h>
#include <track/pathing.h>
#include <trains/navigation.h>
#include <servers/nameserver.h>
#include <detective/sensor_detector_multiplexer.h>
#include <trains/route_executor.h>
#include <trains/reservoir.h>
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

void navigation_test_task() {
  InitPathing();
  InitNavigation();

  Create(PRIORITY_NAMESERVER, nameserver);
  Create(4, sensor_detector_multiplexer_task);
  Create(4, reservoir_task);
  Create(4, mock_executor);

  path_t train20_path;
  path_t train50_path;
  GetPath(&train20_path, Name2Node("C5"), Name2Node("C14"));
  GetPath(&train50_path, Name2Node("C13"), Name2Node("C6"));

  CreateRouteExecutor(10, 20, 10, &train20_path);
  CreateRouteExecutor(10, 50, 10, &train50_path);
}
