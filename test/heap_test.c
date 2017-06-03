#include <check.h>

#include <assert.h>
#include <heap.h>
#include <stdio.h>

START_TEST (test_heap_create)
{
  heap_t heap = heap_create(NULL, 0);
  ck_assert_int_eq(heap.size, 0);
}
END_TEST


START_TEST (test_heap_push_and_pop)
{
  heapnode_t nodes[2];
  heap_t heap = heap_create(nodes, 2);
  int result = 0;
  int value = 0;

  result = heap_push(&heap, 1, (void *) 1);
  ck_assert_msg(result == 0, "Error inserting into heap");

  value = (int) heap_pop(&heap);
  ck_assert_msg(value == 1, "Incorrect value after popping");

  ck_assert_int_eq(heap_size(&heap), 0);
}
END_TEST

START_TEST (test_heap_multiple_push_and_pops)
{
  heapnode_t nodes[10];
  heap_t heap = heap_create(nodes, 10);
  int result = 0;
  int value = 0;

  result = heap_push(&heap, 2, (void *) 2);
  ck_assert_msg(result == 0, "Error inserting into heap");

  result = heap_push(&heap, 1, (void *) 10);
  ck_assert_msg(result == 0, "Error inserting into heap");

  result = heap_push(&heap, -1, (void *) -1);
  ck_assert_msg(result == 0, "Error inserting into heap");

  result = heap_push(&heap, 5, (void *) 5);
  ck_assert_msg(result == 0, "Error inserting into heap");

  value = (int) heap_pop(&heap);
  ck_assert_msg(value == -1, "Incorrect value after popping");

  value = (int) heap_peek_priority(&heap);
  ck_assert_msg(value == 1, "Incorrect value after peeking at priority");

  value = (int) heap_peek(&heap);
  ck_assert_msg(value == 10, "Incorrect value after peeking at value");

  value = (int) heap_pop(&heap);
  ck_assert_msg(value == 10, "Incorrect value after popping");

  value = (int) heap_pop(&heap);
  ck_assert_msg(value == 2, "Incorrect value after popping");

  value = (int) heap_pop(&heap);
  ck_assert_msg(value == 5, "Incorrect value after popping");

  ck_assert_int_eq(heap_size(&heap), 0);
}
END_TEST


START_TEST (test_heap_push_duplicate_priorities)
{
  heapnode_t nodes[10];
  heap_t heap = heap_create(nodes, 10);
  int result = 0;
  int value = 0;

  result = heap_push(&heap, 2, (void *) 2);
  ck_assert_msg(result == 0, "Error inserting into heap");

  result = heap_push(&heap, 1, (void *) 1);
  ck_assert_msg(result == 0, "Error inserting into heap");

  result = heap_push(&heap, -1, (void *) -1);
  ck_assert_msg(result == 0, "Error inserting into heap");

  result = heap_push(&heap, 1, (void *) 1);
  ck_assert_msg(result == 0, "Error inserting into heap");

  value = (int) heap_pop(&heap);
  ck_assert_msg(value == -1, "Incorrect value after popping");

  value = (int) heap_pop(&heap);
  ck_assert_msg(value == 1, "Incorrect value after popping");

  value = (int) heap_pop(&heap);
  ck_assert_msg(value == 1, "Incorrect value after popping");

  value = (int) heap_pop(&heap);
  ck_assert_msg(value == 2, "Incorrect value after popping");

  ck_assert_int_eq(heap_size(&heap), 0);
}
END_TEST


int main(void)
{
  Suite *s1 = suite_create("Core");
  TCase *tc = tcase_create("Core");
  SRunner *sr = srunner_create(s1);
  int nf;

  suite_add_tcase(s1, tc);
  tcase_add_test(tc, test_heap_create);
  tcase_add_test(tc, test_heap_push_and_pop);
  tcase_add_test(tc, test_heap_multiple_push_and_pops);
  tcase_add_test(tc, test_heap_push_duplicate_priorities);

  srunner_run_all(sr, CK_NORMAL);
  nf = srunner_ntests_failed(sr);
  srunner_free(sr);

  return nf == 0 ? 0 : 1;
}
