#include <basic.h>
#include <kernel.h>
#include <nameserver.h>

static int nameserver_tid = -1;

void nameserver() {
  int tid = MyTid();
  nameserver_tid = tid;

  int i;
  int status;

  int mapping[NUM_TASK_NAMES];
  for (i = 0; i < NUM_TASK_NAMES; i++) {
    mapping[i] = -1;
  }

  // Always serve requests
  while (true) {
    nameserver_request_t req;
    int source_tid;

    // Get a nameserver request
    status = Receive(&source_tid, &req, sizeof(req));
    if (status <= 0) {
      // Receive failed, continue ?
      // FIXME: handle status
    }

    if (req.call_type == REGISTER_CALL) {
      // If register call, just add to the hashmap
      log_nameserver("nameserver registered name=%d tid=%d", req.name, source_tid);
      mapping[req.name] = source_tid;
      // FIXME: handle status
      status = Reply(source_tid, NULL, 0);
    } else if (req.call_type == WHOIS_CALL) {
      // If whois, get the value
      // Reply -1 if no tid found, else reply tid
      int val = mapping[req.name];
      log_nameserver("nameserver resp name=%d tid=%d", req.name, val);
      status = Reply(source_tid, &val, sizeof(int));

      // Reply failed, continue ?
      if (status <= 0) {
        // FIXME: handle status
      }
    } else {
      // FIXME: shoud not happen
    }
  }
}

int RegisterAs( task_name_t name ) {
  log_task("RegisterAs name=%d", active_task->tid, name);
  if (nameserver_tid == -1){
    // Don't make data syscall, but still reschedule
    Pass();
    return -1;
  }

  nameserver_request_t req;
  req.call_type = REGISTER_CALL;
  req.name = name;

  Send(nameserver_tid, &req, sizeof(nameserver_request_t), NULL, 0);
  return 0;
}

int WhoIs( task_name_t name ) {
  log_task("WhoIs name=%d", active_task->tid, name);
  if (nameserver_tid == -1) {
    // Don't make data syscall, but still reschedule
    Pass();
    return -1;
  }

  nameserver_request_t req;
  req.call_type = WHOIS_CALL;
  req.name = name;

  int recv_tid;
  Send(nameserver_tid, &req, sizeof(nameserver_request_t), &recv_tid, sizeof(recv_tid));

  return recv_tid;
}
