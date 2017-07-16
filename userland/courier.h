#pragma once

typedef void (*courier_modify_fcn)(void *);

int createCourierAndModify(int priority, int dst, int src, int srcLen, int dstLen, char *name, courier_modify_fcn fcn);
#define createCourier(priority, dst, src, msgLen, name) createCourierAndModify(priority, dst, src, msgLen, msgLen, name, NULL)
