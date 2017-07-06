#include <basic.h>
#include <kernel.h>
#include <warehouse.h>
#include <courier.h>

typedef struct {
  int forwardTo;
  int warehouseSize;
  int courierPriority;
  int structLength;
  int forwardToLength;
  courier_modify_fcn modifyFcn;
} warehouse_setup_t;

void warehouse() {
  int requester;
  warehouse_setup_t setup;

  ReceiveS(&requester, setup);
  ReplyN(requester);

  char data[setup.structLength];

  char queue[setup.warehouseSize][setup.structLength];
  int start = 0;
  int queueLength = 0;

  int courier_tid = createCourierAndModify(
      setup.courierPriority,
      setup.forwardTo, MyTid(),
      setup.structLength, setup.forwardToLength, setup.modifyFcn);
  bool courier_ready = false;

  while (true) {
    int i = (start+queueLength) % setup.warehouseSize;
    Receive(&requester, queue[i], setup.structLength);
    if (requester == courier_tid) {
      courier_ready = true;
    } else {
      queueLength += 1;
      ReplyN(requester);
    }

    if (courier_ready && queueLength > 0) {
      ReplyS(courier_tid, queue[start]);
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
  int forwardToLength,
  courier_modify_fcn modifyFcn
) {
  int tid = Create(priority, &warehouse);
  warehouse_setup_t setup;
  setup.forwardTo = forwardTo;
  setup.warehouseSize = warehouseSize;
  setup.courierPriority = courierPriority;
  setup.structLength = structLength;
  setup.forwardToLength = forwardToLength;
  setup.modifyFcn = modifyFcn;
  SendSN(tid, setup);
  return tid;
}
