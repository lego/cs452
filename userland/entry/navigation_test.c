#include <bwio.h>
#include <track/pathing.h>
#include <kernel.h>

void navigation_test() {
  InitPathing();

  path_t p;
  GetPath(&p, Name2Node("A4"), Name2Node("D5"));
  PrintPath(&p);

  int stopdist = 450;

  bwprintf(COM2, "Running with stopdist 250\n\r");

  // for (int i = p.len - 1; i > 0; i--) {
  for (int i = 1; i < p.len; i++) {
    track_node *curr_node = p.nodes[i];
    // Find first node where the dist - stopdist is too small
    for (int j = 0; j < i; j++) {
      if (curr_node->dist - p.nodes[j]->dist < stopdist) {
        // We need to find the sensor before this node for knowing when we need
        // to reserve and from what sensor
        for (int k = j - 1; k >= 0; k--) {
          if (k < 1) {
            bwprintf(COM2, "Resv at beginning on %4s: %4s.\n\r", p.src->name, curr_node->name);
            j = p.len; // break outer loop also
            break;
          }
          if (p.nodes[k]->type == NODE_SENSOR) {
            bwprintf(COM2, "Resv based on %4s: %4s. Resv %4dmm after %4s\n\r", p.nodes[k]->name, curr_node->name, curr_node->dist - p.nodes[k]->dist - stopdist, p.nodes[k]->name);
            j = p.len; // break outer loop also
            break;
          }
        }
      }
    }

  }

  ExitKernel();
}

void navigation_test_task() {
  Create(10, &navigation_test);
}
