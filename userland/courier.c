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
  int requester;
  courier_setup_t setup;

  ReceiveS(&requester, setup);
  ReplyN(requester);

  int len = setup.srcLen;
  if (setup.dstLen > len) {
    len = setup.dstLen;
  }
  char msgData[len];

  while (true) {
    Send(setup.src, NULL, 0, &msgData, setup.srcLen);
    if (setup.fcn != NULL) {
      setup.fcn(msgData);
    }
    Send(setup.dst, &msgData, setup.dstLen, NULL, 0);
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
