#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdbool.h>
#include <string.h>

/*
 * Include the header file for the vgp module.
 * This should declare is_user_code_stack_trace() and extern declarations
 * for USER_CODE_EXTENSIONS and IGNORE_PATHS if they are global.
 */
#include "vgp.h" // Assuming vgp.h is in the include path

// Test cases

/* LLR-IUCST04: Return false if no valid prefix. */
static void test_is_user_code_no_prefix(void **state) {
    (void)state; // Unused
    assert_false(is_user_code_stack_trace("Some random log line"));
    assert_false(is_user_code_stack_trace("")); // Empty string
}

/* LLR-IUCST02, LLR-IUCST08: Line with " at " but no user extension. */
static void test_is_user_code_at_prefix_no_extension(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   at 0x123: some_function (in /usr/lib/something.so)"));
}

/* LLR-IUCST03, LLR-IUCST08: Line with " by " but no user extension. */
static void test_is_user_code_by_prefix_no_extension(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   by 0x456: another_function (some_other_lib)"));
}

/* LLR-IUCST02, LLR-IUCST05-07, LLR-IUCST09, LLR-IUCST12: Valid "at" prefix, .c extension, no ignore path. */
static void test_is_user_code_at_prefix_with_c_extension_no_ignore(void **state) {
    (void)state;
    assert_true(is_user_code_stack_trace("   at 0x1096F6: my_func (/home/user/project/main.c:86)"));
}

/* LLR-IUCST03, LLR-IUCST05-07, LLR-IUCST09, LLR-IUCST12: Valid "by" prefix, .cpp extension, no ignore path. */
static void test_is_user_code_by_prefix_with_cpp_extension_no_ignore(void **state) {
    (void)state;
    assert_true(is_user_code_stack_trace("   by 0x109A00: Class::method (/path/to/source.cpp:157)"));
}

/* LLR-IUCST02, LLR-IUCST05-07, LLR-IUCST09, LLR-IUCST12: Valid "at" prefix, .h extension, no ignore path. */
static void test_is_user_code_at_prefix_with_h_extension_no_ignore(void **state) {
    (void)state;
    assert_true(is_user_code_stack_trace("   at 0xABC: inline_func (/project/include/utils.h:42)"));
}

/* LLR-IUCST03, LLR-IUCST05-07, LLR-IUCST09, LLR-IUCST12: Valid "by" prefix, .hpp extension, no ignore path. */
static void test_is_user_code_by_prefix_with_hpp_extension_no_ignore(void **state) {
    (void)state;
    assert_true(is_user_code_stack_trace("   by 0xDEF: template_stuff (/project/header.hpp:99)"));
}

/* LLR-IUCST02, LLR-IUCST05-07, LLR-IUCST09-11: Valid "at" prefix, .c extension, but with /usr/include/ ignore path. */
static void test_is_user_code_at_prefix_with_c_extension_with_usr_include_ignore(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   at 0x123: func_in_sys_header (/usr/include/stdio.c:123)")); // .c in /usr/include
    assert_false(is_user_code_stack_trace("   at 0x123: func_in_sys_header (/usr/include/sys/types.h:50)"));
}

/* LLR-IUCST03, LLR-IUCST05-07, LLR-IUCST09-11: Valid "by" prefix, .cpp extension, but with /lib/ ignore path. */
static void test_is_user_code_by_prefix_with_cpp_extension_with_lib_ignore(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   by 0x456: some_lib_func (/lib/ld-linux-x86-64.so.2) (prog.cpp:0)")); // Path contains /lib/
}

/* LLR-IUCST02, LLR-IUCST05-07, LLR-IUCST09-11: Valid "at" prefix, .c extension, but with "???" ignore path. */
static void test_is_user_code_at_prefix_with_c_extension_with_question_marks_ignore(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   at 0x789: unknown_func (???:0)")); // Path contains ???
}

/* LLR-IUCST02, LLR-IUCST08: Valid "at" prefix, but no recognized user extension. */
static void test_is_user_code_at_prefix_no_user_extension(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   at 0xABC: some_func (in /some/path/file.txt)"));
}

/* Additional Test: Test when line is NULL */
static void test_is_user_code_null_input(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace(NULL));
}

/* Additional Test: Test when line is just the prefix */
static void test_is_user_code_just_prefix(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   at "));
    assert_false(is_user_code_stack_trace("   by "));
}

/* Additional Test: Test with mixed case extension */
static void test_is_user_code_mixed_case_extension(void **state) {
    (void)state;
    assert_true(is_user_code_stack_trace("   at 0x123: some_func (main.C:10)"));
}

int run_is_user_code_stack_trace_tests(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_is_user_code_no_prefix),
        cmocka_unit_test(test_is_user_code_at_prefix_no_extension),
        cmocka_unit_test(test_is_user_code_by_prefix_no_extension),
        cmocka_unit_test(test_is_user_code_at_prefix_with_c_extension_no_ignore),
        cmocka_unit_test(test_is_user_code_by_prefix_with_cpp_extension_no_ignore),
        cmocka_unit_test(test_is_user_code_at_prefix_with_h_extension_no_ignore),
        cmocka_unit_test(test_is_user_code_by_prefix_with_hpp_extension_no_ignore),
        cmocka_unit_test(test_is_user_code_at_prefix_with_c_extension_with_usr_include_ignore),
        cmocka_unit_test(test_is_user_code_by_prefix_with_cpp_extension_with_lib_ignore),
        cmocka_unit_test(test_is_user_code_at_prefix_with_c_extension_with_question_marks_ignore),
        cmocka_unit_test(test_is_user_code_at_prefix_no_user_extension),
        cmocka_unit_test(test_is_user_code_null_input),
        cmocka_unit_test(test_is_user_code_just_prefix),
        cmocka_unit_test(test_is_user_code_mixed_case_extension),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}