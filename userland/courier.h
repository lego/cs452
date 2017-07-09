#pragma once

typedef void(*courier_modify_fcn)(void*);

int createCourierAndModify(int priority, int dst, int src, int srcLen, int dstLen, courier_modify_fcn fcn);
#define createCourier(priority, dst, src, msgLen) createCourierAndModify(priority, dst, src, msgLen, msgLen, NULL)
