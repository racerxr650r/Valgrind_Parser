#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdbool.h>

#include "vgp.h"

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

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_extract_file_and_line_rejects_null_args),
        cmocka_unit_test(test_extract_file_and_line_parses_stack_entry_with_line),
        cmocka_unit_test(test_extract_file_and_line_parses_entry_without_line_number),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
