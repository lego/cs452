#pragma once

#include <courier.h>

int createWarehouseWithModifier(
  int priority,
  int forwardTo,
  int warehouseSize,
  int courierPriority,
  int structLength,
  int forwardToLength,
  courier_modify_fcn modifyFcn
);

#define createWarehouse(priority, forwardTo, warehouseSize, courierPriority, structLength) createWarehouseWithModifier(priority, forwardTo, warehouseSize, courierPriority, structLength, structLength, NULL)
