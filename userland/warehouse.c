#include <basic.h>
#include <kernel.h>
#include <jstring.h>
#include <warehouse.h>
#include <courier.h>

#define DEFAULT_BUFFER_SIZE 256

typedef struct {
  int forwardTo;
  int warehouseSize;
  int courierPriority;
  int structLength;
  int forwardToLength;
  courier_modify_fcn modifyFcn;
} warehouse_setup_t;

void warehouse() {
  int tid = MyTid();
  int requester;
  warehouse_setup_t setup;

  ReceiveS(&requester, setup);
  ReplyN(requester);

  bool using_default_size = (setup.structLength == 0);
  if (using_default_size) {
    setup.structLength = DEFAULT_BUFFER_SIZE;
  }

  char data[setup.structLength];

  char queue[setup.warehouseSize][setup.structLength]  __attribute__((aligned (4)));;
  // need to account for this in variable size warehouses
  int element_size[setup.warehouseSize];

  int start = 0;
  int queueLength = 0;

  char courier_name[100];
  jstrappend(MyTaskName(), " - courier", courier_name);

  int courier_tid = createCourierAndModify(
      setup.courierPriority,
      setup.forwardTo, MyTid(),
      using_default_size ? 0 : setup.structLength, using_default_size ? 0 : setup.structLength, courier_name, setup.modifyFcn);
  bool courier_ready = false;

  int status;

  while (true) {
    int end = (start+queueLength) % setup.warehouseSize;
    status = Receive(&requester, queue[end], setup.structLength);
    // If we didn't receive anything, just continue
    if (status < 0) continue;
    // Store the size of the data
    element_size[end] = status;
    if (requester == courier_tid) {
      courier_ready = true;
    } else {
      queueLength += 1;
      ReplyN(requester);
    }

    if (courier_ready && queueLength > 0) {
      int status = Reply(courier_tid, queue[start], element_size[start]);
      // If our courier failed, then we have nothing else to do
      if (status < 0) Destroy(tid);
      start = (start+1) % setup.warehouseSize;
      queueLength -= 1;
      courier_ready = false;
    }
  }
}

int createWarehouseWithModifier(
  int priority,
  int forwardTo,
  int warehouseSize,
  int courierPriority,
  int structLength,
  char * name,
  courier_modify_fcn modifyFcn
) {
  int tid = CreateWithName(priority, &warehouse, name);
  warehouse_setup_t setup;
  setup.forwardTo = forwardTo;
  setup.warehouseSize = warehouseSize;
  setup.courierPriority = courierPriority;
  setup.structLength = structLength;
  setup.modifyFcn = modifyFcn;
  SendSN(tid, setup);
  return tid;
}
