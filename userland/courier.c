#include <basic.h>
#include <kernel.h>
#include <courier.h>

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
  char msgData[len];

  while (true) {
    result = Send(setup.src, NULL, 0, &msgData, setup.srcLen);
    if (result < 0) Destroy(tid);
    if (setup.fcn != NULL) {
      setup.fcn(msgData);
    }
    result = Send(setup.dst, &msgData, setup.dstLen, NULL, 0);
    if (result < 0) Destroy(tid);
  }
}

int createCourierAndModify(int priority, int dst, int src, int srcLen, int dstLen, char * name, courier_modify_fcn fcn) {
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
