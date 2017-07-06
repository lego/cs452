#include <kernel.h>
#include <basic.h>
#include <kern/typed_messaging.h>

typedef struct {
  int a;
} test_t;
DeclMessageType(test_t);

typedef struct {
  int a;
  int b;
} other_test_t;
DeclMessageType(other_test_t);

DefMessageType(test_t);
DefMessageType(other_test_t);

void typed_message_test_task() {
  test_t data;
  data.a = 2;

  other_test_t other_data;
  other_data.a = 4;
  other_data.b = 1;
  SendTyped(1, other_test_t, other_data);

  int sender;
  typed_msg_t msg = RecvTyped(&sender);

  KASSERT(IsMessageType(msg, other_test_t), "Should not happen typeval=%d expected typeval=%d", msg.type, MessageType(other_test_t));
  other_test_t *dater = GetMessageData(msg, other_test_t);

  // KASSERTs here due to bad type fetching
  test_t *dater2 = GetMessageData(msg, test_t);

  ExitKernel();
}
