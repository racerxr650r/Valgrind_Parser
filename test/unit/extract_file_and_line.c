/*
 * extract_file_and_line.c
 *
 * Unit tests for extract_file_and_line() in src/vgp.c.
 *
 * Function under test traces to:
 *   LLR-EFAL-01..07 (Sec. 8 of doc/LLRs.md)
 *   HLR-021 Stack Frame Decoding
 *   HLR-025 First-User-Frame Capture
 *   HLR-039 Defensive Argument Validation
 *   HLR-040 Non-Fatal Recovery from Malformed Input
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdbool.h>

#include "vgp.h"

/*
 * Verifies LLR-EFAL-02 (NULL-pointer guards): the function returns false
 * when any of its four output/input pointers is NULL.
 * Traces to HLR-039 (Defensive Argument Validation).
 */
static void test_extract_file_and_line_rejects_null_args(void **state)
{
    (void)state;
    char filename[MAX_LINE_LENGTH];
    char function_name[MAX_LINE_LENGTH];
    int line_number = 0;

    assert_false(extract_file_and_line(NULL, filename, function_name, &line_number));
    assert_false(extract_file_and_line("line", NULL, function_name, &line_number));
    assert_false(extract_file_and_line("line", filename, NULL, &line_number));
    assert_false(extract_file_and_line("line", filename, function_name, NULL));
}

/*
 * Verifies LLR-EFAL-03, LLR-EFAL-04 (primary three-field sscanf path) on a
 * Valgrind frame of the form "   at 0x...: function (file:line)".
 * Also confirms namespace stripping done by normalize_function_name().
 * Traces to HLR-021 (Stack Frame Decoding) and HLR-025 (First-User-Frame
 * Capture).
 */
static void test_extract_file_and_line_parses_stack_entry_with_line(void **state)
{
    (void)state;
    char filename[MAX_LINE_LENGTH];
    char function_name[MAX_LINE_LENGTH];
    int line_number = 0;

    assert_true(extract_file_and_line("   at 0x109A00: Class::method (/path/to/source.cpp:157)",
                                      filename, function_name, &line_number));
    assert_string_equal(filename, "/path/to/source.cpp");
    assert_string_equal(function_name, "method");
    assert_int_equal(line_number, 157);
}

/*
 * Verifies LLR-EFAL-06 (third "(in <obj>)" sscanf path): a frame with no
 * source line number falls through to the "in <obj>" pattern, sets
 * *line_number to 0, and still returns the function name and object path.
 * Traces to HLR-021 (Stack Frame Decoding) and HLR-040 (Non-Fatal
 * Recovery from Malformed Input).
 */
static void test_extract_file_and_line_parses_entry_without_line_number(void **state)
{
    (void)state;
    char filename[MAX_LINE_LENGTH];
    char function_name[MAX_LINE_LENGTH];
    int line_number = -1;

    assert_true(extract_file_and_line("   by 0x0000: malloc (in /usr/lib/libc.so.6)",
                                      filename, function_name, &line_number));
    assert_string_equal(filename, "/usr/lib/libc.so.6");
    assert_string_equal(function_name, "malloc");
    assert_int_equal(line_number, 0);
}

int run_extract_file_and_line_tests(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_extract_file_and_line_rejects_null_args),
        cmocka_unit_test(test_extract_file_and_line_parses_stack_entry_with_line),
        cmocka_unit_test(test_extract_file_and_line_parses_entry_without_line_number),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
