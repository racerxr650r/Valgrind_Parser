/*
 * strip_valgrind_pid_prefix.c
 *
 * Unit tests for strip_valgrind_pid_prefix() in src/vgp.c.
 *
 * Function under test traces to:
 *   LLR-SVPP-01..06 (Sec. 5 of doc/LLRs.md)
 *   HLR-014 PID-Prefix Stripping
 *   HLR-039 Defensive Argument Validation
 *   HLR-040 Non-Fatal Recovery from Malformed Input
 */

#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "vgp.h"

/* Helper macro: assert that the returned pointer points at the expected
 * suffix of the input buffer (or to the buffer head when nothing was
 * stripped). */
#define TEST_STRIP(input_str, expected_suffix) \
    do { \
        char *original_line = strdup(input_str); \
        assert_non_null(original_line); \
        char *result = strip_valgrind_pid_prefix(original_line); \
        char *expected_ptr = original_line + (strlen(input_str) - strlen(expected_suffix)); \
        if (strcmp(expected_suffix, input_str) == 0) { \
             assert_ptr_equal(result, original_line); \
        } else { \
            assert_ptr_equal(result, expected_ptr); \
        } \
        assert_string_equal(result, expected_suffix); \
        free(original_line); \
    } while (0)

#define TEST_STRIP_EXPECT_ORIGINAL(input_str) \
    do { \
        char *original_line = strdup(input_str); \
        assert_non_null(original_line); \
        char *result = strip_valgrind_pid_prefix(original_line); \
        assert_ptr_equal(result, original_line); \
        assert_string_equal(result, input_str); \
        free(original_line); \
    } while (0)

/*
 * Verifies LLR-SVPP-03: when no "==" substring exists the original line
 * pointer is returned unchanged
 * (HLR-014 PID-Prefix Stripping, HLR-040 Non-Fatal Recovery).
 */
static void test_strip_no_prefix_at_all(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("Hello World");
}

/*
 * Verifies LLR-SVPP-02: NULL input returns NULL without dereferencing
 * (HLR-039 Defensive Argument Validation).
 */
static void test_strip_null_input(void **state) {
    (void)state;
    assert_null(strip_valgrind_pid_prefix(NULL));
}

/*
 * Verifies LLR-SVPP-03: empty input has no "==" prefix and is returned
 * unchanged (HLR-014, HLR-040).
 */
static void test_strip_empty_string(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("");
}

/*
 * Verifies LLR-SVPP-04: input is bare "==" with no following digits, so
 * the original pointer is returned (HLR-014, HLR-040).
 */
static void test_strip_only_double_equals(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("==");
}

/*
 * Verifies LLR-SVPP-04: "==" present but no digit follows the optional
 * whitespace, so input is returned unchanged
 * (HLR-014 PID-Prefix Stripping, HLR-040 Non-Fatal Recovery).
 */
static void test_strip_double_equals_no_pid_digits(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("== Hello");
    TEST_STRIP_EXPECT_ORIGINAL("==abc== Hello");
}

/*
 * Verifies LLR-SVPP-05: "==<digits>" present but no trailing "== "
 * sequence, so the original line is returned (HLR-014, HLR-040).
 */
static void test_strip_pid_no_trailing_marker(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("==123 Hello");
}

/*
 * Verifies LLR-SVPP-05: trailing "==" is missing the required following
 * space, so the prefix does not match (HLR-014, HLR-040).
 */
static void test_strip_pid_trailing_double_equals_no_space(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("==123==Hello");
}

/*
 * Verifies LLR-SVPP-06: full "==<pid>== " prefix is stripped and a
 * pointer to the message body is returned (HLR-014 PID-Prefix Stripping).
 */
static void test_strip_valid_prefix_simple(void **state) {
    (void)state;
    TEST_STRIP("==123== Hello World", "Hello World");
}

/*
 * Verifies LLR-SVPP-04 + LLR-SVPP-05 (whitespace tolerance around the
 * PID) and LLR-SVPP-06 (HLR-014 PID-Prefix Stripping).
 */
static void test_strip_valid_prefix_spaces_around_pid(void **state) {
    (void)state;
    TEST_STRIP("==  456  == Message", "Message");
    TEST_STRIP("==789== Message", "Message");
}

/*
 * Verifies LLR-SVPP-06: prefix is stripped even when the message body is
 * empty (HLR-014 PID-Prefix Stripping).
 */
static void test_strip_valid_prefix_no_message_after(void **state) {
    (void)state;
    TEST_STRIP("==789== ", "");
}

/*
 * Verifies LLR-SVPP-05: trailing "==" with no following space and no
 * body is rejected (HLR-014, HLR-040).
 */
static void test_strip_valid_prefix_incomplete_end_marker(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("==101112==");
}

/*
 * Verifies LLR-SVPP-04: leading "==" followed by whitespace and then "=="
 * is not a PID prefix because the inner field is not digits
 * (HLR-014, HLR-040).
 */
static void test_strip_prefix_like_too_short_for_pid(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("== == Message");
}

/*
 * Verifies LLR-SVPP-05: variants where the trailing "== " marker is
 * absent or incomplete cause the original line to be returned
 * (HLR-014, HLR-040).
 */
static void test_strip_prefix_like_too_short_for_end_marker_full(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("==123==");
    TEST_STRIP_EXPECT_ORIGINAL("==123==M");
}

/*
 * Verifies LLR-SVPP-06: a single-character message body is correctly
 * exposed after prefix stripping (HLR-014 PID-Prefix Stripping).
 */
static void test_strip_valid_prefix_short_end_marker(void **state) {
    (void)state;
    TEST_STRIP("==123== M", "M");
}

/*
 * Verifies LLR-SVPP-06: a line consisting solely of a valid prefix
 * yields an empty body (HLR-014 PID-Prefix Stripping).
 */
static void test_strip_line_is_exact_prefix(void **state) {
    (void)state;
    TEST_STRIP("==12345== ", "");
}

/*
 * Verifies LLR-SVPP-06: only the first valid prefix is stripped; any
 * subsequent "==N==" patterns inside the body are preserved verbatim
 * (HLR-014 PID-Prefix Stripping).
 */
static void test_strip_multiple_potential_prefixes(void **state) {
    (void)state;
    TEST_STRIP("==1== First ==2== Second", "First ==2== Second");
}

/*
 * Verifies LLR-SVPP-04: arbitrarily long PID digit runs are accepted
 * (HLR-014 PID-Prefix Stripping).
 */
static void test_strip_valid_prefix_long_pid(void **state) {
    (void)state;
    TEST_STRIP("==123456789012345== Long PID Message", "Long PID Message");
}

/*
 * Verifies LLR-SVPP-03 + LLR-SVPP-06: leading whitespace before the
 * "==" is tolerated because strstr() finds the first "==" regardless
 * (HLR-014 PID-Prefix Stripping).
 */
static void test_strip_leading_spaces_before_prefix(void **state) {
    (void)state;
    TEST_STRIP("  ==123== Message", "Message");
}

int run_strip_valgrind_pid_prefix_tests(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_strip_null_input),
        cmocka_unit_test(test_strip_empty_string),
        cmocka_unit_test(test_strip_no_prefix_at_all),
        cmocka_unit_test(test_strip_only_double_equals),
        cmocka_unit_test(test_strip_double_equals_no_pid_digits),
        cmocka_unit_test(test_strip_pid_no_trailing_marker),
        cmocka_unit_test(test_strip_pid_trailing_double_equals_no_space),
        cmocka_unit_test(test_strip_valid_prefix_simple),
        cmocka_unit_test(test_strip_valid_prefix_spaces_around_pid),
        cmocka_unit_test(test_strip_valid_prefix_no_message_after),
        cmocka_unit_test(test_strip_valid_prefix_incomplete_end_marker),
        cmocka_unit_test(test_strip_prefix_like_too_short_for_pid),
        cmocka_unit_test(test_strip_prefix_like_too_short_for_end_marker_full),
        cmocka_unit_test(test_strip_valid_prefix_short_end_marker),
        cmocka_unit_test(test_strip_line_is_exact_prefix),
        cmocka_unit_test(test_strip_multiple_potential_prefixes),
        cmocka_unit_test(test_strip_valid_prefix_long_pid),
        cmocka_unit_test(test_strip_leading_spaces_before_prefix),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
