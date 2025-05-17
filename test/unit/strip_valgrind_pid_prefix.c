#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h> // For strdup, free

/*
 * Include the header file for the vgp module.
 * This should declare strip_valgrind_pid_prefix().
 */
#include "vgp.h" // Assuming vgp.h is in the include path

// Helper macro for string duplication and cleanup
#define TEST_STRIP(input_str, expected_suffix) \
    do { \
        char *original_line = strdup(input_str); \
        assert_non_null(original_line); \
        char *result = strip_valgrind_pid_prefix(original_line); \
        char *expected_ptr = original_line + (strlen(input_str) - strlen(expected_suffix)); \
        if (strcmp(expected_suffix, input_str) == 0) { /* Expecting original string */ \
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


/* LLR-SVPP03: Return original if "==" not found */
static void test_strip_no_prefix_at_all(void **state) {
    (void)state; // Unused
    TEST_STRIP_EXPECT_ORIGINAL("Hello World");
}

/* Test with NULL input (Defensive check, LLRs don't explicitly cover) */
static void test_strip_null_input(void **state) {
    (void)state;
    // Assuming the function should handle NULL gracefully, e.g., by returning NULL.
    // If it crashes, this test will fail.
    // The LLRs imply it returns the original pointer if criteria aren't met.
    assert_null(strip_valgrind_pid_prefix(NULL));
}

/* LLR-SVPP03: Return original for empty string */
static void test_strip_empty_string(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("");
}

/* LLR-SVPP03 / LLR-SVPP07: "==" found, but no subsequent PID digits */
static void test_strip_only_double_equals(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("==");
}

/* LLR-SVPP07: "==" found, then non-digit */
static void test_strip_double_equals_no_pid_digits(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("== Hello");
    TEST_STRIP_EXPECT_ORIGINAL("==abc== Hello"); // Non-digits where PID should be
}

/* LLR-SVPP11: "==" and PID found, but no trailing "== " */
static void test_strip_pid_no_trailing_marker(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("==123 Hello");
}

/* LLR-SVPP11: "==" and PID and "==" found, but no space after final "==" */
static void test_strip_pid_trailing_double_equals_no_space(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("==123==Hello");
}

/* LLR-SVPP13: Valid prefix, simple case */
static void test_strip_valid_prefix_simple(void **state) {
    (void)state;
    TEST_STRIP("==123== Hello World", "Hello World");
}

/* LLR-SVPP05, LLR-SVPP09, LLR-SVPP13: Valid prefix with spaces around PID */
static void test_strip_valid_prefix_spaces_around_pid(void **state) {
    (void)state;
    TEST_STRIP("==  456  == Message", "Message");
    TEST_STRIP("==789== Message", "Message"); // No spaces around PID, but valid
}

/* LLR-SVPP13: Valid prefix, no message content after */
static void test_strip_valid_prefix_no_message_after(void **state) {
    (void)state;
    TEST_STRIP("==789== ", "");
}

/* LLR-SVPP11: Valid start, PID, but incomplete end marker "==" (missing space) */
static void test_strip_valid_prefix_incomplete_end_marker(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("==101112==");
}

/* LLR-SVPP07: Prefix-like but too short for PID after initial "==" and spaces */
static void test_strip_prefix_like_too_short_for_pid(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("== == Message"); // No digits after initial "==" and space
}

/* LLR-SVPP11: Prefix-like but too short for full end marker "== " */
static void test_strip_prefix_like_too_short_for_end_marker_full(void **state) {
    (void)state;
    TEST_STRIP_EXPECT_ORIGINAL("==123==");   // Missing space
    TEST_STRIP_EXPECT_ORIGINAL("==123==M");  // Missing space and more chars
}

static void test_strip_valid_prefix_short_end_marker(void **state) {
    (void)state;
    TEST_STRIP("==123== M", "M");
}

/* LLR-SVPP13: Line content is exactly a valid prefix ending with a space */
static void test_strip_line_is_exact_prefix(void **state) {
    (void)state;
    TEST_STRIP("==12345== ", "");
}

/* Test that only the first valid prefix is stripped */
static void test_strip_multiple_potential_prefixes(void **state) {
    (void)state;
    TEST_STRIP("==1== First ==2== Second", "First ==2== Second");
}

/* Test with very long PID */
static void test_strip_valid_prefix_long_pid(void **state) {
    (void)state;
    TEST_STRIP("==123456789012345== Long PID Message", "Long PID Message");
}

/* Test with leading spaces before the "==" */
static void test_strip_leading_spaces_before_prefix(void **state) {
    (void)state;
    TEST_STRIP("  ==123== Message", "Message"); // `strstr` will find "==" at the beginning
}


int main(void) {
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