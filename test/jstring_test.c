#include <check.h>

#include <assert.h>
#include <jstring.h>
#include <stdio.h>

START_TEST (jformatf_no_format)
{
  char str[1024];
  jformatf(str, 1024, "hello");
  ck_assert_str_eq(str, "hello");
}
END_TEST

START_TEST (jformatf_int)
{
  char str[1024];
  jformatf(str, 1024, "val=%d", 2);
  ck_assert_str_eq(str, "val=2");

  jformatf(str, 1024, "val=%d", 1337);
  ck_assert_str_eq(str, "val=1337");
}
END_TEST

START_TEST (jformatf_string)
{
  char str[1024];
  jformatf(str, 1024, "hello %s %s", "goodbye", "maybe");
  ck_assert_str_eq(str, "hello goodbye maybe");
}
END_TEST


START_TEST (jformatf_char)
{
  char str[1024];
  jformatf(str, 1024, "val=%c", '&');
  ck_assert_str_eq(str, "val=&");
}
END_TEST


START_TEST (jformatf_escaped_modulo)
{
  char str[1024];
  jformatf(str, 1024, "val=%%");
  ck_assert_str_eq(str, "val=%");
}
END_TEST


START_TEST (jformatf_hex)
{
  char str[1024];
  jformatf(str, 1024, "val=%x", 12345);
  ck_assert_str_eq(str, "val=3039");
}
END_TEST


START_TEST (jformatf_long)
{
  // FIXME: does nothing
}
END_TEST


START_TEST (jformatf_unsigned_int)
{
  // FIXME: does nothing
}
END_TEST


START_TEST (jformatf_padded_int)
{
  char str[1024];
  jformatf(str, 1024, "val=%12d", 2);
  ck_assert_str_eq(str, "val=           2");
}
END_TEST


START_TEST (jformatf_padded_int_with_zeros)
{
  char str[1024];
  jformatf(str, 1024, "val=%012d", 2);
  ck_assert_str_eq(str, "val=000000000002");
}
END_TEST


START_TEST (jformatf_trailing_padded_int)
{
  char str[1024];
  jformatf(str, 1024, "val=%-4d", 2);
  ck_assert_str_eq(str, "val=2   ");
}
END_TEST


START_TEST (jformatf_padded_string)
{
  char str[1024];
  jformatf(str, 1024, "val=%8s", "bye");
  ck_assert_str_eq(str, "val=     bye");
}
END_TEST

START_TEST (jformatf_trailing_padding)
{
  char str[1024];
  jformatf(str, 1024, "val=%-8s", "bye");
  ck_assert_str_eq(str, "val=bye     ");
}
END_TEST


START_TEST (jformatf_clears_buffer)
{
  char str[1024];
  strcpy(str, "no terminator");
  jformatf(str, 1024, "yep");
  ck_assert_str_eq(str, "yep");
}
END_TEST

START_TEST (jstrncpy_buffer_larger)
{
  char str[1024];
  strcpy(str, "no termination eh");
  jstrncpy(str, "yep", 1024);
  ck_assert_str_eq(str, "yep");
}
END_TEST

START_TEST (jstrncpy_buffer_smaller)
{
  char str[2];
  jstrncpy(str, "yep", 2);
  ck_assert_str_eq(str, "y");
}
END_TEST

START_TEST (jstrncpy_buffer_equal_size)
{
  char str[8];
  jstrncpy(str, "justright", 8);
  ck_assert_str_eq(str, "justrig");
}
END_TEST


int main(void)
{
  Suite *s1 = suite_create("jstring");
  TCase *tc;

  tc = tcase_create("jformatf");
  suite_add_tcase(s1, tc);
  tcase_add_test(tc, jformatf_no_format);
  tcase_add_test(tc, jformatf_int);
  tcase_add_test(tc, jformatf_string);
  tcase_add_test(tc, jformatf_char);
  tcase_add_test(tc, jformatf_escaped_modulo);
  tcase_add_test(tc, jformatf_clears_buffer);
  tcase_add_test(tc, jformatf_hex);
  tcase_add_test(tc, jformatf_padded_int);
  tcase_add_test(tc, jformatf_padded_int_with_zeros);
  tcase_add_test(tc, jformatf_padded_string);
  tcase_add_test(tc, jformatf_long);
  tcase_add_test(tc, jformatf_unsigned_int);
  tcase_add_test(tc, jformatf_trailing_padding);
  tcase_add_test(tc, jformatf_trailing_padded_int);

  tc = tcase_create("jstrncpy");
  suite_add_tcase(s1, tc);
  tcase_add_test(tc, jstrncpy_buffer_smaller);
  tcase_add_test(tc, jstrncpy_buffer_equal_size);
  tcase_add_test(tc, jstrncpy_buffer_larger);

  SRunner *sr = srunner_create(s1);
  srunner_set_fork_status(sr, CK_NOFORK);
  srunner_run_all(sr, CK_NORMAL);
  int nf = srunner_ntests_failed(sr);
  srunner_free(sr);

  return nf == 0 ? 0 : 1;
}
