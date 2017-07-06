#include <check.h>

#include <debug.h>
#include <assert.h>
#include <alloc.h>
#include <stdio.h>

START_TEST (alloc_allocates_realloctable_memory)
{
  allocator_init();
  ck_assert_int_eq(disbursed_size, 0);
  ck_assert_int_eq(reallocatable_size, 0);
  alloc(20);
  ck_assert_int_ge(disbursed_size, 20);
  ck_assert_int_ge(reallocatable_size, 20);
}
END_TEST

START_TEST (alloc_allocates_nonrealloctable_memory)
{
  allocator_init();
  ck_assert_int_eq(disbursed_size, 0);
  ck_assert_int_eq(reallocatable_size, 0);
  alloc(2000);
  ck_assert_int_ge(disbursed_size, 2000);
  ck_assert_int_eq(reallocatable_size, 0);
}
END_TEST

START_TEST (alloc_fails_out_of_memory)
{
  // FIXME: do test
}
END_TEST

START_TEST (free_loses_nonrealloctable_memory)
{
  allocator_init();
  ck_assert_int_eq(disbursed_size, 0);
  ck_assert_int_eq(reallocatable_size, 0);
  void *data = alloc(MAX_REALLOCATION_SIZE + 1000);
  ck_assert_int_ge(disbursed_size, MAX_REALLOCATION_SIZE + 1000);
  ck_assert_int_eq(reallocatable_size, 0);
  int result = jfree(data);
  ck_assert_int_eq(1, result);
  ck_assert_int_ge(lost_memory_size, MAX_REALLOCATION_SIZE + 1000);
  ck_assert_int_eq(lost_memory_count, 1);
}
END_TEST

START_TEST (free_recovers_realloctable_memory)
{
  allocator_init();
  ck_assert_int_eq(disbursed_size, 0);
  ck_assert_int_eq(reallocatable_size, 0);
  void *data = alloc(20);
  ck_assert_int_ge(disbursed_size, 20);
  ck_assert_int_ge(reallocatable_size, 20);
  int result = jfree(data);
  ck_assert_int_eq(0, result);
  ck_assert_int_eq(lost_memory_size, 0);
  ck_assert_int_eq(lost_memory_count, 0);
  ck_assert_int_ge(memory_returned_size, 20);
  ck_assert_int_eq(memory_returned_count, 1);
}
END_TEST

/**
 * Tests that a series of allocs and frees will continue to re-use memory
 * alloc -> block leased
 * free  -> block released
 * alloc -> block reused
 * free  -> block released, again
 * so forth
 */
START_TEST (block_reuse)
{
  void *data;
  int result;
  int amount_previously_reallocatable;

  // init
  allocator_init();
  ck_assert_int_eq(disbursed_size, 0);
  ck_assert_int_eq(reallocatable_size, 0);
  ck_assert_int_eq(reallocatable_count, 0);
  ck_assert_int_eq(reused_size, 0);

  // allocate first block
  data = alloc(20);
  ck_assert_int_ge(disbursed_size, 20);
  ck_assert_int_ge(reallocatable_size, 20);
  ck_assert_int_eq(reallocatable_count, 1);
  ck_assert_int_eq(reused_size, 0);
  ck_assert_int_eq(reused_count, 0);

  // free first block
  result = jfree(data);
  ck_assert_int_eq(0, result);
  ck_assert_int_eq(lost_memory_size, 0);
  ck_assert_int_eq(lost_memory_count, 0);
  ck_assert_int_ge(memory_returned_size, 20);
  ck_assert_int_eq(memory_returned_count, 1);

  // allocate second block
  amount_previously_reallocatable = reallocatable_size;
  data = alloc(20);
  ck_assert_int_eq(amount_previously_reallocatable, reallocatable_size);
  ck_assert_int_eq(reallocatable_count, 1);
  ck_assert_int_ge(reused_size, 20);
  ck_assert_int_eq(reused_count, 1);

  // free second block
  result = jfree(data);
  ck_assert_int_eq(0, result);
  ck_assert_int_eq(lost_memory_size, 0);
  ck_assert_int_eq(lost_memory_count, 0);
  ck_assert_int_ge(memory_returned_size, 40);
  ck_assert_int_eq(memory_returned_count, 2);

  // allocate third block
  amount_previously_reallocatable = reallocatable_size;
  data = alloc(20);
  ck_assert_int_eq(amount_previously_reallocatable, reallocatable_size);
  ck_assert_int_eq(reallocatable_count, 1);
  ck_assert_int_ge(reused_size, 40);
  ck_assert_int_eq(reused_count, 2);

  // free third block
  result = jfree(data);
  ck_assert_int_eq(0, result);
  ck_assert_int_eq(lost_memory_size, 0);
  ck_assert_int_eq(lost_memory_count, 0);
  ck_assert_int_ge(memory_returned_size, 60);
  ck_assert_int_eq(memory_returned_count, 3);

  // allocate fourth block
  amount_previously_reallocatable = reallocatable_size;
  data = alloc(20);
  ck_assert_int_eq(reallocatable_count, 1);
  ck_assert_int_eq(amount_previously_reallocatable, reallocatable_size);
  ck_assert_int_ge(reused_size, 60);
  ck_assert_int_eq(reused_count, 3);

  // free fourth block
  result = jfree(data);
  ck_assert_int_eq(0, result);
  ck_assert_int_eq(lost_memory_size, 0);
  ck_assert_int_eq(lost_memory_count, 0);
  ck_assert_int_ge(memory_returned_size, 80);
  ck_assert_int_eq(memory_returned_count, 4);
}
END_TEST


/**
 * Tests that parallel allocs and frees will re-use, and allocate new blocks
 * alloc  -> block1 leased
 * alloc  -> block2 leased
 * free   -> block1 released
 * alloc  -> block1 leased
 * free 2 -> block2 released
 * alloc  -> block2 leased
 */
START_TEST (block_reuse_and_allocation)
{
  void *block1;
  void *block2;
  void *data;
  int result;
  int amount_previously_reallocatable;

  // init
  allocator_init();
  ck_assert_int_eq(disbursed_size, 0);
  ck_assert_int_eq(reallocatable_size, 0);
  ck_assert_int_eq(reallocatable_count, 0);
  ck_assert_int_eq(reused_size, 0);

  // allocate first block
  block1 = alloc(20);
  ck_assert_int_ge(disbursed_size, 20);
  ck_assert_int_ge(reallocatable_size, 20);
  ck_assert_int_eq(reallocatable_count, 1);
  ck_assert_int_eq(reused_size, 0);
  ck_assert_int_eq(reused_count, 0);

  // allocate second block
  block2 = alloc(20);
  ck_assert_int_ge(disbursed_size, 40);
  ck_assert_int_ge(reallocatable_size, 40);
  ck_assert_int_eq(reallocatable_count, 2);
  ck_assert_int_ge(reused_size, 0);
  ck_assert_int_eq(reused_count, 0);

  // free first block
  result = jfree(block1);
  ck_assert_int_eq(0, result);
  ck_assert_int_eq(lost_memory_size, 0);
  ck_assert_int_eq(lost_memory_count, 0);
  ck_assert_int_ge(memory_returned_size, 20);
  ck_assert_int_eq(memory_returned_count, 1);

  // re-allocate first block
  amount_previously_reallocatable = reallocatable_size;
  data = alloc(20);
  ck_assert_ptr_eq(block1, data);
  ck_assert_int_eq(amount_previously_reallocatable, reallocatable_size);
  ck_assert_int_eq(reallocatable_count, 2);
  ck_assert_int_ge(reused_size, 20);
  ck_assert_int_eq(reused_count, 1);

  // free second block
  result = jfree(block2);
  ck_assert_int_eq(0, result);
  ck_assert_int_eq(lost_memory_size, 0);
  ck_assert_int_eq(lost_memory_count, 0);
  ck_assert_int_ge(memory_returned_size, 40);
  ck_assert_int_eq(memory_returned_count, 2);

  // re-allocate second block
  amount_previously_reallocatable = reallocatable_size;
  data = alloc(20);
  ck_assert_ptr_eq(block2, data);
  ck_assert_int_eq(amount_previously_reallocatable, reallocatable_size);
  ck_assert_int_eq(reallocatable_count, 2);
  ck_assert_int_ge(reused_size, 40);
  ck_assert_int_eq(reused_count, 2);
}
END_TEST

/**
 * Tests that do parallel allocs and re-use of different size blocks
 * alloc  20  -> block1 leased
 * alloc 140  -> block2 leased
 * free blk1  -> block1 released
 * free blk2  -> block2 released
 * alloc 120  -> block2 leased
 * alloc  20  -> block1 leased
 */
START_TEST (different_size_block_reuse)
{
  void *block1;
  void *block2;
  void *data;
  int result;
  int amount_previously_reallocatable;

  // init
  allocator_init();
  ck_assert_int_eq(disbursed_size, 0);
  ck_assert_int_eq(reallocatable_size, 0);
  ck_assert_int_eq(reallocatable_count, 0);
  ck_assert_int_eq(reused_size, 0);

  // allocate first block
  block1 = alloc(20);
  ck_assert_int_ge(disbursed_size, 20);
  ck_assert_int_ge(reallocatable_size, 20);
  ck_assert_int_eq(reallocatable_count, 1);
  ck_assert_int_eq(reused_size, 0);
  ck_assert_int_eq(reused_count, 0);

  // allocate second block
  block2 = alloc(120);
  ck_assert_int_ge(disbursed_size, 140);
  ck_assert_int_ge(reallocatable_size, 140);
  ck_assert_int_eq(reallocatable_count, 2);
  ck_assert_int_ge(reused_size, 0);
  ck_assert_int_eq(reused_count, 0);

  // free first block
  result = jfree(block1);
  ck_assert_int_eq(0, result);
  ck_assert_int_eq(lost_memory_size, 0);
  ck_assert_int_eq(lost_memory_count, 0);
  ck_assert_int_ge(memory_returned_size, 20);
  ck_assert_int_eq(memory_returned_count, 1);

  // free second block
  result = jfree(block2);
  ck_assert_int_eq(0, result);
  ck_assert_int_eq(lost_memory_size, 0);
  ck_assert_int_eq(lost_memory_count, 0);
  ck_assert_int_ge(memory_returned_size, 140);
  ck_assert_int_eq(memory_returned_count, 2);

  // re-allocate second block
  amount_previously_reallocatable = reallocatable_size;
  data = alloc(120);
  ck_assert_ptr_eq(block2, data);
  ck_assert_int_eq(amount_previously_reallocatable, reallocatable_size);
  ck_assert_int_eq(reallocatable_count, 2);
  ck_assert_int_ge(reused_size, 120);
  ck_assert_int_eq(reused_count, 1);

  // re-allocate first block
  amount_previously_reallocatable = reallocatable_size;
  data = alloc(20);
  ck_assert_ptr_eq(block1, data);
  ck_assert_int_eq(amount_previously_reallocatable, reallocatable_size);
  ck_assert_int_eq(reallocatable_count, 2);
  ck_assert_int_ge(reused_size, 140);
  ck_assert_int_eq(reused_count, 2);
}
END_TEST


int main(void)
{
  Suite *s1 = suite_create("alloc");

  TCase *tc = tcase_create("alloc");
  suite_add_tcase(s1, tc);
  tcase_add_test(tc, alloc_allocates_realloctable_memory);
  tcase_add_test(tc, alloc_allocates_nonrealloctable_memory);
  tcase_add_test(tc, alloc_fails_out_of_memory);
  tcase_add_test(tc, free_loses_nonrealloctable_memory);
  tcase_add_test(tc, free_recovers_realloctable_memory);
  tcase_add_test(tc, block_reuse);
  tcase_add_test(tc, block_reuse_and_allocation);
  tcase_add_test(tc, different_size_block_reuse);

  SRunner *sr = srunner_create(s1);
  srunner_set_fork_status(sr, CK_NOFORK);
  srunner_run_all(sr, CK_NORMAL);
  int nf = srunner_ntests_failed(sr);
  srunner_free(sr);

  return nf == 0 ? 0 : 1;
}
