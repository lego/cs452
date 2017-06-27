#include <alloc.h>

static char memory[1000000];
static unsigned int current_location;

void *alloc(unsigned int size) {
  // FIXME: align memory
  current_location += size;
  return (void *) memory + current_location - size;
}

int jfree(void *ptr) {
  // Herp derp, do nothing
  return 0;
}
