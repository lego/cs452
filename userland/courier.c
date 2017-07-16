#include <basic.h>
#include <kernel.h>
#include <courier.h>

#define DEFAULT_BUFFER_SIZE 1024

typedef struct {
  int dst;
  int src;
  int srcLen;
  int dstLen;
  courier_modify_fcn fcn;
} courier_setup_t;

void courier() {
  int tid = MyTid();
  int requester;
  int result;
  courier_setup_t setup;

  ReceiveS(&requester, setup);
  ReplyN(requester);

  int len = setup.srcLen;
  if (setup.dstLen > len) {
    len = setup.dstLen;
  }

  // Default buffer size, for unprovided amounts
  bool using_default_size = (len == 0);
  if (using_default_size) {
    len = DEFAULT_BUFFER_SIZE;
    setup.srcLen = len;
    setup.dstLen = len;
  }

  char msgData[len] __attribute__((aligned(4)));

  while (true) {
    result = Send(setup.src, NULL, 0, &msgData, setup.srcLen);
    KASSERT(!using_default_size || result < DEFAULT_BUFFER_SIZE, "Default courier buffer. Please re-evaluate buffer sizes.");
    if (result < 0)
      Destroy(tid);
    if (setup.fcn != NULL) {
      setup.fcn(msgData);
    }
    result = Send(setup.dst, &msgData, result, NULL, 0);
    KASSERT(!using_default_size || result < DEFAULT_BUFFER_SIZE, "Default courier buffer overflowed. Please re-evaluate buffer sizes.");
    if (result < 0)
      Destroy(tid);
  }
}

int createCourierAndModify(int priority, int dst, int src, int srcLen, int dstLen, char *name, courier_modify_fcn fcn) {
  int tid = CreateWithName(priority, &courier, name);
  courier_setup_t setup;
  setup.dst = dst;
  setup.src = src;
  setup.srcLen = srcLen;
  setup.dstLen = dstLen;
  setup.fcn = fcn;
  SendSN(tid, setup);
  return tid;
}
