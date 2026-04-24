/*
 * is_user_code_stack_trace.c
 *
 * Unit tests for is_user_code_stack_trace() in src/vgp.c.
 *
 * Function under test traces to:
 *   LLR-IUCST-01..06 (Sec. 6 of doc/LLRs.md)
 *   HLR-020 Stack Frame Recognition
 *   HLR-022 User-Code Classification
 *   HLR-029 Multi-Language Source Support
 *   HLR-039 Defensive Argument Validation
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdbool.h>
#include <string.h>

#include "vgp.h"

/*
 * Verifies LLR-IUCST-03: lines lacking the "   at " / "   by " prefix are
 * rejected outright (HLR-020 Stack Frame Recognition,
 * HLR-022 User-Code Classification).
 */
static void test_is_user_code_no_prefix(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("Some random log line"));
    assert_false(is_user_code_stack_trace(""));
}

/*
 * Verifies LLR-IUCST-03 + LLR-IUCST-04: a "   at " frame whose path lacks
 * a recognised user-code extension is not user code
 * (HLR-022 User-Code Classification).
 */
static void test_is_user_code_at_prefix_no_extension(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   at 0x123: some_function (in /usr/lib/something.so)"));
}

/*
 * Verifies LLR-IUCST-03 + LLR-IUCST-04 for the "   by " prefix
 * (HLR-022 User-Code Classification).
 */
static void test_is_user_code_by_prefix_no_extension(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   by 0x456: another_function (some_other_lib)"));
}

/*
 * Verifies LLR-IUCST-03 + LLR-IUCST-04 + LLR-IUCST-05 + LLR-IUCST-06: a
 * "   at " frame with a .c extension and no IGNORE_PATHS hit returns true
 * (HLR-022 User-Code Classification, HLR-029 Multi-Language Source
 * Support).
 */
static void test_is_user_code_at_prefix_with_c_extension_no_ignore(void **state) {
    (void)state;
    assert_true(is_user_code_stack_trace("   at 0x1096F6: my_func (/home/user/project/main.c:86)"));
}

/*
 * Verifies LLR-IUCST-03 + LLR-IUCST-04 + LLR-IUCST-05 + LLR-IUCST-06 for
 * the "   by " prefix and a .cpp extension (HLR-022, HLR-029).
 */
static void test_is_user_code_by_prefix_with_cpp_extension_no_ignore(void **state) {
    (void)state;
    assert_true(is_user_code_stack_trace("   by 0x109A00: Class::method (/path/to/source.cpp:157)"));
}

/*
 * Verifies LLR-IUCST-04 + LLR-IUCST-06 for a .h extension
 * (HLR-022 User-Code Classification, HLR-029 Multi-Language Source
 * Support).
 */
static void test_is_user_code_at_prefix_with_h_extension_no_ignore(void **state) {
    (void)state;
    assert_true(is_user_code_stack_trace("   at 0xABC: inline_func (/project/include/utils.h:42)"));
}

/*
 * Verifies LLR-IUCST-04 + LLR-IUCST-06 for a .hpp extension
 * (HLR-022 User-Code Classification, HLR-029 Multi-Language Source
 * Support).
 */
static void test_is_user_code_by_prefix_with_hpp_extension_no_ignore(void **state) {
    (void)state;
    assert_true(is_user_code_stack_trace("   by 0xDEF: template_stuff (/project/header.hpp:99)"));
}

/*
 * Verifies LLR-IUCST-05 (IGNORE_PATHS deny-list): even when the extension
 * matches USER_CODE_EXTENSIONS, a path beginning with /usr/ is rejected
 * (HLR-022 User-Code Classification, LLR-GBL-03).
 */
static void test_is_user_code_at_prefix_with_c_extension_with_usr_include_ignore(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   at 0x123: func_in_sys_header (/usr/include/stdio.c:123)"));
    assert_false(is_user_code_stack_trace("   at 0x123: func_in_sys_header (/usr/include/sys/types.h:50)"));
}

/*
 * Verifies LLR-IUCST-05: a path containing /lib/ is rejected even when
 * the extension matches (HLR-022 User-Code Classification, LLR-GBL-03).
 */
static void test_is_user_code_by_prefix_with_cpp_extension_with_lib_ignore(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   by 0x456: some_lib_func (/lib/ld-linux-x86-64.so.2) (prog.cpp:0)"));
}

/*
 * Verifies LLR-IUCST-04: a frame whose path tokens do not contain any
 * recognised user-code extension (here "???:0") is rejected
 * (HLR-022 User-Code Classification).
 */
static void test_is_user_code_at_prefix_with_c_extension_with_question_marks_ignore(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   at 0x789: unknown_func (???:0)"));
}

/*
 * Verifies LLR-IUCST-04: a non-source-file extension (.txt) is not in
 * USER_CODE_EXTENSIONS and the frame is rejected
 * (HLR-022 User-Code Classification).
 */
static void test_is_user_code_at_prefix_no_user_extension(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   at 0xABC: some_func (in /some/path/file.txt)"));
}

/*
 * Verifies LLR-IUCST-02 (NULL-pointer guard) and HLR-039 (Defensive
 * Argument Validation).
 */
static void test_is_user_code_null_input(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace(NULL));
}

/*
 * Verifies LLR-IUCST-04: a bare "   at " / "   by " line with no path
 * after it cannot match any USER_CODE_EXTENSIONS entry
 * (HLR-022 User-Code Classification).
 */
static void test_is_user_code_just_prefix(void **state) {
    (void)state;
    assert_false(is_user_code_stack_trace("   at "));
    assert_false(is_user_code_stack_trace("   by "));
}

/*
 * Verifies LLR-IUCST-04 + LLR-GBL-02: uppercase ".C" appears in
 * USER_CODE_EXTENSIONS and is recognised as user code
 * (HLR-022 User-Code Classification, HLR-029 Multi-Language Source
 * Support).
 */
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
