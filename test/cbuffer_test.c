/*
 * iotest.c
 */

#include <assert.h>
#include <cbuffer.h>
#include <stdio.h>

int main() {
  // void **c_buf[80];
  // void **test_buf[80];
  // int result;
  // void *c;
  // int i;
  //
  // // create a buffer
  // cbuffer buf = create_cbuffer(c_buf, 80);
  //
  // // check the buffer is empty
  // assert(cbuffer_empty(&buf));
  // assert(!cbuffer_full(&buf));
  //
  // // add an item
  // char a = 'a';
  // result = cbuffer_add(&buf, &a);
  // assert(result == 0);
  // assert(!cbuffer_empty(&buf));
  //
  // // pop the item, and test for it
  // c = cbuffer_pop(&buf, &result);
  // assert(result == 0);
  // assert(c == &a);
  // assert(cbuffer_empty(&buf));
  //
  // // fill the buffer
  // for (i = 0; i < 80; i++) {
  //   result = cbuffer_add(&buf, i);
  //   assert(result == 0);
  // }
  //
  // // check the buffer is full
  // assert(cbuffer_full(&buf));
  //
  // // check the next add fails
  // result = cbuffer_add(&buf, a);
  // assert(result == -1);
  //
  // // empty the buffer
  // for (i = 0; i < 80; i++) {
  //   c = cbuffer_pop(&buf, &result);
  //   assert(result == 0);
  //   assert(c == i);
  // }
  //
  // // check that the next pop fails
  // cbuffer_pop(&buf, &result);
  // assert(result == -1);
  //
  // // cause buffer to wrap multiple times
  // for (i = 0; i < 70; i++) {
  //   result = cbuffer_add(&buf, i);
  //   assert(result == 0);
  // }
  // for (i = 0; i < 70; i++) {
  //   c = cbuffer_pop(&buf, &result);
  //   assert(result == 0);
  //   assert(c == i);
  // }
  // for (i = 0; i < 70; i++) {
  //   result = cbuffer_add(&buf, i);
  //   assert(result == 0);
  // }
  // for (i = 0; i < 70; i++) {
  //   c = cbuffer_pop(&buf, &result);
  //   assert(result == 0);
  //   assert(c == i);
  // }
  //
  // // Test add twice, pop, then unpop
  // result = cbuffer_add(&buf, 1);
  // assert(result == 0);
  // result = cbuffer_add(&buf, 2);
  // assert(result == 0);
  // c = cbuffer_pop(&buf, &result);
  // assert(result == 0);
  // assert(c == 1);
  // result = cbuffer_unpop(&buf, c);
  // assert(result == 0);
  // c = cbuffer_pop(&buf, &result);
  // assert(result == 0);
  // assert(c == 1);
  //
  // // Unpop and add again, this is an edgecase
  // // Of not increasing size again
  // c = cbuffer_pop(&buf, &result);
  // assert(result == 0);
  // assert(c == 2);
  // result = cbuffer_unpop(&buf, c);
  // assert(result == 0);
  // c = cbuffer_pop(&buf, &result);
  // assert(result == 0);
  // assert(c == 2);

  return 0;
}
