#include <basic.h>
#include <map.h>
#include <kernel.h>
#include <nameserver.h>

void nameserver() {
  int i;
  map_val_t map_buf[256];
  map_t map;
  map_init(&map, map_buf, 256);
  int status;

  // Used as pointer array for the map values
  // We have no other way to statically allocate this space.
  int tid_const_arr[MAX_TASKS];
  for (i = 0; i < MAX_TASKS; i++) {
    tid_const_arr[i] = i;
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
      status = map_insert(&map, req.name, &tid_const_arr[source_tid]);
      // FIXME: handle status
    } else if (req.call_type == WHOIS_CALL) {
      // If whois, get the value
      // Reply -1 if no tid found, else reply tid
      void *val = map_get(&map, req.name);
      if (val == NULL) {
        int tmp = -1;
        status = Reply(source_tid, &tmp, sizeof(int));
      } else {
        status = Reply(source_tid, val, sizeof(int));
      }

      // Reply failed, continue ?
      if (status <= 0) {
        // FIXME: handle status
      }
    } else {
      // FIXME: shoud not happen
    }
  }
}
