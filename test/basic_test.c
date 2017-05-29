#include <assert.h>
#include <cbuffer.h>
#include <stdio.h>

#include <basic.h>

#define assert_ctz(e, v) assert(ctz(v) == e)

void test_ctz() {
  assert_ctz(0, 0b1);
  assert_ctz(0, 0b01);
  assert_ctz(1, 0b10);
  assert_ctz(2, 0b100);
  assert_ctz(3, 0b1000);
  assert_ctz(4, 0b10000);
  assert_ctz(5, 0b100000);
  assert_ctz(6, 0b1000000);
  assert_ctz(7, 0b10000000);
  assert_ctz(8, 0b100000000);
  assert_ctz(9, 0b1000000000);
  assert_ctz(10, 0b10000000000);
  assert_ctz(11, 0b100000000000);
  assert_ctz(12, 0b1000000000000);
  assert_ctz(13, 0b10000000000000);
  assert_ctz(14, 0b100000000000000);
  assert_ctz(15, 0b1000000000000000);
  assert_ctz(16, 0b10000000000000000);
  assert_ctz(17, 0b100000000000000000);
  assert_ctz(18, 0b1000000000000000000);
  assert_ctz(19, 0b10000000000000000000);
  assert_ctz(20, 0b100000000000000000000);
  assert_ctz(21, 0b1000000000000000000000);
  assert_ctz(22, 0b10000000000000000000000);
  assert_ctz(23, 0b100000000000000000000000);
  assert_ctz(24, 0b1000000000000000000000000);
  assert_ctz(25, 0b10000000000000000000000000);
  assert_ctz(26, 0b100000000000000000000000000);
  assert_ctz(27, 0b1000000000000000000000000000);
  assert_ctz(28, 0b10000000000000000000000000000);
  assert_ctz(29, 0b100000000000000000000000000000);
  assert_ctz(30, 0b1000000000000000000000000000000);
  assert_ctz(31, 0b10000000000000000000000000000000);
}

int main() {
  test_ctz();

  return 0;
}
