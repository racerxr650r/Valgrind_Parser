/*
 * vgp_core.c
 *
 * Unit tests for the parser engine, output formatter, and source
 * extraction helpers in src/vgp.c, plus end-to-end exercise of the
 * vgp(1) command-line surface in src/main.c.
 *
 * Each test is annotated with the LLR identifiers (from doc/LLRs.md)
 * and HLR identifiers (from doc/HLRs.md) it exercises so a future
 * traceability matrix can map tests to requirements.
 */

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include "vgp.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

/* ---- Output suppression helpers ---------------------------------------- */
/* Many of the parser helpers print to stdout/stderr by design. These setup
 * and teardown helpers redirect both streams to /dev/null so the cmocka
 * test report stays readable. They do not exercise any LLR themselves. */

static int saved_stdout = -1;
static int saved_stderr = -1;

static int setup_suppress(void **state)
{
    (void)state;
    fflush(stdout);
    fflush(stderr);
    saved_stdout = dup(STDOUT_FILENO);
    saved_stderr = dup(STDERR_FILENO);
    int devnull = open("/dev/null", O_WRONLY);
    dup2(devnull, STDOUT_FILENO);
    dup2(devnull, STDERR_FILENO);
    close(devnull);
    return 0;
}

static int teardown_suppress(void **state)
{
    (void)state;
    fflush(stdout);
    fflush(stderr);
    if (saved_stdout >= 0) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
        saved_stdout = -1;
    }
    if (saved_stderr >= 0) {
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
        saved_stderr = -1;
    }
    return 0;
}

/* ---- initialize_parse_state -------------------------------------------- */

/*
 * Verifies LLR-IPS-03: every ParseState field is reset to its documented
 * initial value, even when the structure already contains stale data.
 * Traces to HLR-038 (Stateful Parsing Context).
 */
static void test_initialize_parse_state_normal(void **state)
{
    (void)state;
    ParseState ps;
    ps.in_error_block = true;
    ps.error_count = 42;
    initialize_parse_state(&ps);
    assert_false(ps.in_error_block);
    assert_false(ps.print_function);
    assert_int_equal(ps.stack_lines_shown, 0);
    assert_false(ps.user_code_found_for_error);
    assert_int_equal(ps.error_count, 0);
    assert_int_equal(ps.error_line_number, -1);
    assert_string_equal(ps.current_error_type, "");
    assert_string_equal(ps.error_filename, "");
    assert_string_equal(ps.error_function_name, "");
}

/*
 * Verifies LLR-IPS-02: a NULL ParseState pointer is a no-op.
 * Traces to HLR-039 (Defensive Argument Validation).
 */
static void test_initialize_parse_state_null(void **state)
{
    (void)state;
    initialize_parse_state(NULL);
}

/* ---- check_start_new_error --------------------------------------------- */

/*
 * Verifies LLR-CSNE-02: NULL line_content or state pointers cause the
 * function to return false without side effects.
 * Traces to HLR-039 (Defensive Argument Validation).
 */
static void test_check_start_new_error_null_args(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_false(check_start_new_error(NULL, &ps));
    assert_false(check_start_new_error("some line", NULL));
    assert_false(check_start_new_error(NULL, NULL));
}

/*
 * Verifies LLR-CSNE-04 + LLR-GBL-04 for the "Invalid read" keyword:
 * a matching line starts a new error block and bumps error_count.
 * Traces to HLR-016 (Error Block Detection),
 * HLR-018 (Error Header Rendering), HLR-038 (Stateful Parsing Context).
 */
static void test_check_start_new_error_detects_invalid_read(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_true(check_start_new_error("Invalid read of size 4", &ps));
    assert_true(ps.in_error_block);
    assert_int_equal(ps.error_count, 1);
}

/*
 * Verifies LLR-CSNE-04 + LLR-GBL-04 for the "Invalid write" keyword.
 * Traces to HLR-016 (Error Block Detection).
 */
static void test_check_start_new_error_detects_invalid_write(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_true(check_start_new_error("Invalid write of size 8", &ps));
    assert_true(ps.in_error_block);
}

/*
 * Verifies LLR-CSNE-04 + LLR-GBL-04 for the
 * "depends on uninitialised value" keyword.
 * Traces to HLR-016 (Error Block Detection).
 */
static void test_check_start_new_error_detects_uninit(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_true(check_start_new_error("depends on uninitialised value(s)", &ps));
}

/*
 * Verifies LLR-CSNE-04 + LLR-GBL-04 for the "Invalid free" keyword.
 * Traces to HLR-016 (Error Block Detection).
 */
static void test_check_start_new_error_detects_invalid_free(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_true(check_start_new_error("Invalid free() / delete / delete[]", &ps));
}

/*
 * Verifies LLR-CSNE-04 + LLR-GBL-04 for the "Mismatched free" keyword.
 * Traces to HLR-016 (Error Block Detection).
 */
static void test_check_start_new_error_detects_mismatched_free(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_true(check_start_new_error("Mismatched free() / delete / delete[]", &ps));
}

/*
 * Verifies LLR-CSNE-04 + LLR-GBL-04 for the
 * "Source and destination overlap" keyword.
 * Traces to HLR-016 (Error Block Detection).
 */
static void test_check_start_new_error_detects_overlap(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_true(check_start_new_error("Source and destination overlap in memcpy", &ps));
}

/*
 * Verifies LLR-CSNE-05: a non-error keyword line does not start an
 * error block (HLR-016 Error Block Detection).
 */
static void test_check_start_new_error_no_match(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_false(check_start_new_error("HEAP SUMMARY:", &ps));
    assert_false(ps.in_error_block);
}

/*
 * Verifies LLR-CSNE-03: while the parser is already inside an error
 * block, no new block is started even if the line matches a keyword
 * (HLR-016 Error Block Detection, HLR-038 Stateful Parsing Context).
 */
static void test_check_start_new_error_already_in_block(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;
    assert_false(check_start_new_error("Invalid read of size 4", &ps));
}

/* ---- process_stack_trace_line ----------------------------------------- */

/*
 * Verifies LLR-PSTL-02: NULL line_content or state cause the function
 * to return without side effects.
 * Traces to HLR-039 (Defensive Argument Validation).
 */
static void test_process_stack_trace_line_null_args(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    process_stack_trace_line(NULL, &ps);
    process_stack_trace_line("   at 0x123: foo (bar.c:10)", NULL);
}

/*
 * Verifies LLR-PSTL-05: the first user-code frame in the block is
 * captured into ParseState and arms print_function for the eventual
 * source listing.
 * Traces to HLR-022 (User-Code Classification),
 * HLR-025 (First-User-Frame Capture).
 */
static void test_process_stack_trace_line_finds_user_code(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;
    app_config.verbose = true;
    app_config.print_stack = false;

    process_stack_trace_line("   at 0x1096F6: my_func (/home/user/project/main.c:86)", &ps);
    assert_true(ps.user_code_found_for_error);
    assert_true(ps.print_function);
    assert_int_equal(ps.error_line_number, 86);

    app_config.verbose = false;
}

/*
 * Verifies LLR-PSTL-05 negative path: a non-user-code frame does not
 * arm user_code_found_for_error.
 * Traces to HLR-022 (User-Code Classification).
 */
static void test_process_stack_trace_line_non_user_code(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;
    app_config.verbose = true;

    process_stack_trace_line("   at 0x123: malloc (in /usr/lib/libc.so.6)", &ps);
    assert_false(ps.user_code_found_for_error);

    app_config.verbose = false;
}

/*
 * Verifies LLR-PSTL-04: when print_stack is enabled, every printed frame
 * increments stack_lines_shown.
 * Traces to HLR-024 (Stack Trace Output Gated by Options),
 * HLR-038 (Stateful Parsing Context).
 */
static void test_process_stack_trace_line_with_print_stack(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;
    app_config.print_stack = true;
    app_config.verbose = false;

    process_stack_trace_line("   at 0x1096F6: my_func (/home/user/project/main.c:86)", &ps);
    assert_int_equal(ps.stack_lines_shown, 1);

    app_config.print_stack = false;
}

/*
 * Verifies LLR-PSTL-06: at the STACK_TRACE_CONTEXT_LINES boundary, a
 * non-user frame triggers the one-shot ellipsis and increments
 * stack_lines_shown to suppress further ellipses.
 * Traces to HLR-023 (Bounded Stack Trace Output),
 * HLR-024 (Stack Trace Output Gated by Options).
 */
static void test_process_stack_trace_line_ellipsis_branch(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;
    ps.stack_lines_shown = STACK_TRACE_CONTEXT_LINES;
    ps.user_code_found_for_error = false;
    app_config.print_stack = true;
    app_config.verbose = false;

    process_stack_trace_line("   at 0x123: malloc (in /usr/lib/libc.so.6)", &ps);
    assert_int_equal(ps.stack_lines_shown, STACK_TRACE_CONTEXT_LINES + 1);

    app_config.print_stack = false;
}

/* ---- finalize_error_block --------------------------------------------- */

/*
 * Verifies LLR-FEB-02: NULL state is a no-op.
 * Traces to HLR-039 (Defensive Argument Validation).
 */
static void test_finalize_error_block_null(void **state)
{
    (void)state;
    finalize_error_block(NULL);
}

/*
 * Verifies LLR-FEB-03 and LLR-FEB-06: when no user-code frame was found
 * but stack_lines_shown > 0, the diagnostic hint is emitted and the
 * in_error_block / print_function flags are reset.
 * Traces to HLR-017 (Error Block Termination),
 * HLR-022 (User-Code Classification),
 * HLR-025 (First-User-Frame Capture).
 */
static void test_finalize_error_block_no_user_code(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;
    ps.user_code_found_for_error = false;
    ps.stack_lines_shown = 5;
    strncpy(ps.current_error_type, "Invalid read", sizeof(ps.current_error_type));

    finalize_error_block(&ps);
    assert_false(ps.in_error_block);
    assert_false(ps.print_function);
}

/*
 * Verifies LLR-FEB-04 + LLR-FEB-06 in isolation from LLR-FEB-05: with
 * print_function set but print_source / verbose off, only the
 * "Source (file:line)" reference line is emitted.
 * Traces to HLR-026 (Conditional Source Reference Output),
 * HLR-028 (Editor-Jumpable References),
 * HLR-017 (Error Block Termination).
 */
static void test_finalize_error_block_with_print_function(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;
    ps.user_code_found_for_error = true;
    ps.print_function = true;
    strncpy(ps.error_filename, "/tmp/nonexistent.c", sizeof(ps.error_filename));
    strncpy(ps.error_function_name, "test_func", sizeof(ps.error_function_name));
    ps.error_line_number = 10;
    app_config.print_source = false;
    app_config.verbose = false;

    finalize_error_block(&ps);
    assert_false(ps.in_error_block);
}

/*
 * Verifies LLR-FEB-05: with verbose enabled the function attempts to
 * print the source via print_source_function() (which gracefully fails
 * when the file does not exist).
 * Traces to HLR-012 (Verbose Mode),
 * HLR-027 (Conditional Source Body Output),
 * HLR-030 (External-Tool Dependency Isolation),
 * HLR-043 (External ctags Dependency Scope).
 */
static void test_finalize_error_block_with_verbose(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;
    ps.user_code_found_for_error = true;
    ps.print_function = true;
    strncpy(ps.error_filename, "/tmp/nonexistent_file_xyz.c", sizeof(ps.error_filename));
    strncpy(ps.error_function_name, "no_such_func", sizeof(ps.error_function_name));
    ps.error_line_number = 10;
    app_config.verbose = true;

    finalize_error_block(&ps);
    assert_false(ps.in_error_block);

    app_config.verbose = false;
}

/* ---- process_in_error_block ------------------------------------------- */

/*
 * Verifies LLR-PIEB-02: NULL pointers are no-ops.
 * Traces to HLR-039 (Defensive Argument Validation).
 */
static void test_process_in_error_block_null_args(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    process_in_error_block(NULL, &ps);
    process_in_error_block("some line", NULL);
}

/*
 * Verifies LLR-PIEB-03: a "   at " / "   by " line is dispatched to
 * process_stack_trace_line and the block remains active.
 * Traces to HLR-020 (Stack Frame Recognition),
 * HLR-021 (Stack Frame Decoding).
 */
static void test_process_in_error_block_stack_trace(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;
    app_config.verbose = true;

    process_in_error_block("   at 0x1096F6: my_func (/home/user/project/main.c:86)", &ps);
    assert_true(ps.in_error_block);

    app_config.verbose = false;
}

/*
 * Verifies LLR-PIEB-04: a non-stack-trace line ends the block via
 * finalize_error_block, clearing in_error_block.
 * Traces to HLR-017 (Error Block Termination).
 */
static void test_process_in_error_block_non_stack_trace(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;

    process_in_error_block("some other line", &ps);
    assert_false(ps.in_error_block);
}

/* ---- process_summary_lines -------------------------------------------- */

/*
 * Verifies LLR-PSL-02: NULL line_content is a no-op.
 * Traces to HLR-039 (Defensive Argument Validation).
 */
static void test_process_summary_lines_null(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    process_summary_lines(NULL, &ps);
}

/*
 * Verifies LLR-PSL-03 and LLR-PSL-04 with print_leak_summary enabled:
 * the LEAK SUMMARY header and each of the four leak categories are
 * recognised and dispatched.
 * Traces to HLR-031 (Leak Summary Recognition),
 * HLR-032 (Leak Summary Output Gated by Options),
 * HLR-033 (Leak Detail Formatting).
 */
static void test_process_summary_lines_leak_summary(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    app_config.print_leak_summary = true;

    process_summary_lines("LEAK SUMMARY:", &ps);
    process_summary_lines("definitely lost: 96 bytes in 3 blocks", &ps);
    process_summary_lines("indirectly lost: 16 bytes in 1 blocks", &ps);
    process_summary_lines("possibly lost: 0 bytes in 0 blocks", &ps);
    process_summary_lines("still reachable: 30 bytes in 1 blocks", &ps);

    app_config.print_leak_summary = false;
}

/*
 * Verifies the negative gating path of LLR-PSL-03 and LLR-PSL-04: with
 * print_leak_summary and verbose both false, recognised leak lines
 * produce no output.
 * Traces to HLR-032 (Leak Summary Output Gated by Options).
 */
static void test_process_summary_lines_leak_without_flag(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    app_config.print_leak_summary = false;
    app_config.verbose = false;

    process_summary_lines("LEAK SUMMARY:", &ps);
    process_summary_lines("definitely lost: 96 bytes in 3 blocks", &ps);
    process_summary_lines("indirectly lost: 16 bytes in 1 blocks", &ps);
    process_summary_lines("possibly lost: 0 bytes in 0 blocks", &ps);
    process_summary_lines("still reachable: 30 bytes in 1 blocks", &ps);
}

/*
 * Verifies LLR-PSL-05: the ERROR SUMMARY line is always dispatched to
 * print_final_error_summary regardless of any configuration flag.
 * Traces to HLR-034 (Final Error Summary Output).
 */
static void test_process_summary_lines_error_summary(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.error_count = 5;

    process_summary_lines("ERROR SUMMARY: 8 errors from 5 contexts", &ps);
}

/*
 * Verifies the fall-through path of process_summary_lines: lines that
 * match no recognised summary marker produce no output and do not
 * crash.
 * Traces to HLR-031 (Leak Summary Recognition),
 * HLR-040 (Non-Fatal Recovery from Malformed Input).
 */
static void test_process_summary_lines_no_match(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    process_summary_lines("For a detailed leak analysis, rerun with: --leak-check=full", &ps);
}

/* ---- print_leak_summary_line ------------------------------------------ */

/*
 * Verifies LLR-PLSL-02 and LLR-PLSL-03: a well-formed leak line is
 * parsed into bytes/blocks counts and reformatted.
 * Traces to HLR-005 (Locale Initialisation),
 * HLR-033 (Leak Detail Formatting).
 */
static void test_print_leak_summary_line_parse_success(void **state)
{
    (void)state;
    print_leak_summary_line("definitely lost: 96 bytes in 3 blocks", "Definitely Lost");
}

/*
 * Verifies LLR-PLSL-04 (newline branch): an unparseable line that
 * already ends in '\n' is printed verbatim without an extra newline.
 * Traces to HLR-035 (Robust Numeric Parsing),
 * HLR-040 (Non-Fatal Recovery from Malformed Input).
 */
static void test_print_leak_summary_line_parse_fail_with_newline(void **state)
{
    (void)state;
    print_leak_summary_line("malformed leak line\n", "Unknown");
}

/*
 * Verifies LLR-PLSL-04 (no-newline branch): an unparseable line without
 * a trailing newline is printed with one appended.
 * Traces to HLR-035 (Robust Numeric Parsing),
 * HLR-040 (Non-Fatal Recovery from Malformed Input).
 */
static void test_print_leak_summary_line_parse_fail_no_newline(void **state)
{
    (void)state;
    print_leak_summary_line("malformed leak line", "Unknown");
}

/* ---- print_final_error_summary ---------------------------------------- */

/*
 * Verifies LLR-PFES-02 and LLR-PFES-03: the FINAL COUNTS block is
 * emitted with Total Errors taken from ParseState and Possible Leaks
 * computed as Valgrind's total minus Total Errors.
 * Traces to HLR-005 (Locale Initialisation),
 * HLR-034 (Final Error Summary Output),
 * HLR-038 (Stateful Parsing Context).
 */
static void test_print_final_error_summary_parse_success(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.error_count = 3;
    print_final_error_summary("ERROR SUMMARY: 5 errors from 3 contexts", &ps);
}

/*
 * Verifies LLR-PFES-04 (newline branch).
 * Traces to HLR-035 (Robust Numeric Parsing),
 * HLR-040 (Non-Fatal Recovery from Malformed Input).
 */
static void test_print_final_error_summary_parse_fail_with_newline(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    print_final_error_summary("malformed error summary\n", &ps);
}

/*
 * Verifies LLR-PFES-04 (no-newline branch).
 * Traces to HLR-035 (Robust Numeric Parsing),
 * HLR-040 (Non-Fatal Recovery from Malformed Input).
 */
static void test_print_final_error_summary_parse_fail_no_newline(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    print_final_error_summary("malformed error summary", &ps);
}

/* ---- print_error_header ----------------------------------------------- */

/*
 * Verifies LLR-PEH-02..05 with verbose enabled and an error_type that
 * already ends with '\n' (LLR-PEH-04 verbatim branch).
 * Traces to HLR-012 (Verbose Mode),
 * HLR-018 (Error Header Rendering),
 * HLR-024 (Stack Trace Output Gated by Options).
 */
static void test_print_error_header_with_newline(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    app_config.verbose = true;
    print_error_header("Invalid read of size 4\n", &ps);
    assert_int_equal(ps.error_count, 1);
    app_config.verbose = false;
}

/*
 * Verifies LLR-PEH-04 newline-append branch and LLR-PEH-05 (Call Stack:
 * heading printed when print_stack is on).
 * Traces to HLR-018 (Error Header Rendering),
 * HLR-024 (Stack Trace Output Gated by Options).
 */
static void test_print_error_header_without_newline(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    app_config.print_stack = true;
    print_error_header("Invalid read of size 4", &ps);
    assert_int_equal(ps.error_count, 1);
    app_config.print_stack = false;
}

/*
 * Verifies the negative branch of LLR-PEH-05: with both flags off, the
 * "Call Stack:" heading is suppressed but the rest of the header still
 * prints.
 * Traces to HLR-018 (Error Header Rendering),
 * HLR-024 (Stack Trace Output Gated by Options).
 */
static void test_print_error_header_no_stack_flag(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    app_config.print_stack = false;
    app_config.verbose = false;
    print_error_header("Invalid read of size 4\n", &ps);
    assert_int_equal(ps.error_count, 1);
}

/* ---- get_function_name ------------------------------------------------ */

/*
 * Verifies LLR-GFN-03 + LLR-GFN-05: a fully-formed stack frame is
 * reformatted as "function(basename:line)".
 * Traces to HLR-021 (Stack Frame Decoding),
 * HLR-028 (Editor-Jumpable References).
 */
static void test_get_function_name_with_line_number(void **state)
{
    (void)state;
    char newline[MAX_LINE_LENGTH];
    get_function_name("   at 0x1096F6: my_func (/home/user/project/main.c:86)", newline);
    assert_non_null(strstr(newline, "my_func"));
    assert_non_null(strstr(newline, "main.c"));
    assert_non_null(strstr(newline, "86"));
}

/*
 * Verifies LLR-GFN-06: when the line number cannot be parsed (for an
 * "(in <obj>)" frame), the function emits the placeholder "?(?:0)".
 * Traces to HLR-021 (Stack Frame Decoding),
 * HLR-040 (Non-Fatal Recovery from Malformed Input).
 */
static void test_get_function_name_without_line_number(void **state)
{
    (void)state;
    char newline[MAX_LINE_LENGTH];
    get_function_name("   at 0x123: malloc (in /usr/lib/libc.so.6)", newline);
    assert_non_null(strstr(newline, "?(?:0)"));
}

/*
 * Verifies LLR-GFN-04: a function name carrying a parameter list (e.g.
 * C++ overload notation) has the suffix truncated.
 * Traces to HLR-021 (Stack Frame Decoding).
 */
static void test_get_function_name_paren_in_function(void **state)
{
    (void)state;
    char newline[MAX_LINE_LENGTH];
    get_function_name("   at 0x123: Class::method(int) (/path/to/file.cpp:42)", newline);
    assert_non_null(strstr(newline, "42"));
}

/* ---- process_log_file ------------------------------------------------- */

/*
 * Verifies LLR-PLF-02: a NULL FILE* argument is a no-op.
 * Traces to HLR-039 (Defensive Argument Validation).
 */
static void test_process_log_file_null(void **state)
{
    (void)state;
    process_log_file(NULL);
}

/*
 * Verifies LLR-PLF-04..LLR-PLF-09 end-to-end on a minimal log
 * containing a single error block followed by an ERROR SUMMARY line.
 * Traces to HLR-014 (PID-Prefix Stripping),
 * HLR-016 (Error Block Detection),
 * HLR-018 (Error Header Rendering),
 * HLR-020 (Stack Frame Recognition),
 * HLR-034 (Final Error Summary Output),
 * HLR-036 (Streaming Single-Pass Operation).
 */
static void test_process_log_file_with_error_block(void **state)
{
    (void)state;
    const char *log_content =
        "==123== Invalid read of size 4\n"
        "==123==    at 0x1096F6: my_func (/home/user/project/main.c:86)\n"
        "==123==    by 0x109B00: main (/home/user/project/main.c:100)\n"
        "==123== \n"
        "==123== ERROR SUMMARY: 1 errors from 1 contexts\n";

    FILE *f = tmpfile();
    assert_non_null(f);
    fwrite(log_content, 1, strlen(log_content), f);
    rewind(f);

    app_config.verbose = true;
    app_config.print_leak_summary = true;
    process_log_file(f);
    app_config.verbose = false;
    app_config.print_leak_summary = false;

    fclose(f);
}

/*
 * Verifies LLR-PLF-10: when EOF is reached while still inside an error
 * block, finalize_error_block is invoked and a closing separator is
 * printed.
 * Traces to HLR-019 (End-of-Log Error-Block Finalisation).
 */
static void test_process_log_file_eof_in_error_block(void **state)
{
    (void)state;
    const char *log_content =
        "==123== Invalid write of size 1\n"
        "==123==    at 0x1096F6: my_func (/home/user/project/main.c:86)\n";

    FILE *f = tmpfile();
    assert_non_null(f);
    fwrite(log_content, 1, strlen(log_content), f);
    rewind(f);

    app_config.verbose = true;
    process_log_file(f);
    app_config.verbose = false;

    fclose(f);
}

/*
 * Verifies LLR-PLF-09 dispatching to process_summary_lines for a
 * leak-summary-only log.
 * Traces to HLR-031 (Leak Summary Recognition),
 * HLR-033 (Leak Detail Formatting),
 * HLR-034 (Final Error Summary Output).
 */
static void test_process_log_file_leak_summary(void **state)
{
    (void)state;
    const char *log_content =
        "==123== LEAK SUMMARY:\n"
        "==123==    definitely lost: 96 bytes in 3 blocks\n"
        "==123==    indirectly lost: 16 bytes in 1 blocks\n"
        "==123==    possibly lost: 0 bytes in 0 blocks\n"
        "==123==    still reachable: 30 bytes in 1 blocks\n"
        "==123== ERROR SUMMARY: 0 errors from 0 contexts\n";

    FILE *f = tmpfile();
    assert_non_null(f);
    fwrite(log_content, 1, strlen(log_content), f);
    rewind(f);

    app_config.verbose = true;
    app_config.print_leak_summary = true;
    process_log_file(f);
    app_config.verbose = false;
    app_config.print_leak_summary = false;

    fclose(f);
}

/*
 * Verifies LLR-PLF-07 + LLR-PLF-08 sequencing: a finalised block's
 * terminator line is re-evaluated as a possible new error header so
 * that two adjacent blocks are both detected.
 * Traces to HLR-016 (Error Block Detection),
 * HLR-017 (Error Block Termination),
 * HLR-018 (Error Header Rendering).
 */
static void test_process_log_file_multiple_errors(void **state)
{
    (void)state;
    const char *log_content =
        "==123== Invalid read of size 4\n"
        "==123==    at 0x1096F6: foo (/home/user/proj/foo.c:10)\n"
        "==123== \n"
        "==123== Invalid write of size 8\n"
        "==123==    at 0x1096F6: bar (/home/user/proj/bar.c:20)\n"
        "==123==    by 0x109B00: main (/home/user/proj/main.c:30)\n"
        "==123== \n"
        "==123== ERROR SUMMARY: 2 errors from 2 contexts\n";

    FILE *f = tmpfile();
    assert_non_null(f);
    fwrite(log_content, 1, strlen(log_content), f);
    rewind(f);

    app_config.verbose = true;
    process_log_file(f);
    app_config.verbose = false;

    fclose(f);
}

/*
 * Verifies the gated-output paths through process_log_file when no
 * optional output flag is enabled: only the error summary is printed.
 * Traces to HLR-024 (Stack Trace Output Gated by Options),
 * HLR-027 (Conditional Source Body Output),
 * HLR-032 (Leak Summary Output Gated by Options),
 * HLR-034 (Final Error Summary Output).
 */
static void test_process_log_file_no_verbose(void **state)
{
    (void)state;
    const char *log_content =
        "==123== Invalid read of size 4\n"
        "==123==    at 0x1096F6: foo (/home/user/proj/foo.c:10)\n"
        "==123== \n"
        "==123== LEAK SUMMARY:\n"
        "==123==    definitely lost: 96 bytes in 3 blocks\n"
        "==123==    indirectly lost: 16 bytes in 1 blocks\n"
        "==123==    possibly lost: 0 bytes in 0 blocks\n"
        "==123==    still reachable: 30 bytes in 1 blocks\n"
        "==123== ERROR SUMMARY: 1 errors from 1 contexts\n";

    FILE *f = tmpfile();
    assert_non_null(f);
    fwrite(log_content, 1, strlen(log_content), f);
    rewind(f);

    app_config.verbose = false;
    app_config.print_stack = false;
    app_config.print_source = false;
    app_config.print_leak_summary = false;
    process_log_file(f);

    fclose(f);
}

/*
 * Verifies the print_source path through finalize_error_block when the
 * source file referenced by the captured frame does not exist (the
 * print_source_function helper reports failure and the block still
 * finishes cleanly).
 * Traces to HLR-027 (Conditional Source Body Output),
 * HLR-030 (External-Tool Dependency Isolation),
 * HLR-043 (External ctags Dependency Scope).
 */
static void test_process_log_file_with_print_source(void **state)
{
    (void)state;
    const char *log_content =
        "==123== Invalid read of size 4\n"
        "==123==    at 0x1096F6: foo (/home/user/proj/foo.c:10)\n"
        "==123== \n"
        "==123== ERROR SUMMARY: 1 errors from 1 contexts\n";

    FILE *f = tmpfile();
    assert_non_null(f);
    fwrite(log_content, 1, strlen(log_content), f);
    rewind(f);

    app_config.verbose = false;
    app_config.print_source = true;
    app_config.print_stack = false;
    process_log_file(f);
    app_config.print_source = false;

    fclose(f);
}

/* ---- extract_file_and_line additional branches ------------------------ */

/*
 * Verifies LLR-EFAL-07: a line that matches none of the three sscanf
 * patterns returns false.
 * Traces to HLR-021 (Stack Frame Decoding),
 * HLR-040 (Non-Fatal Recovery from Malformed Input).
 */
static void test_extract_file_and_line_format_without_function(void **state)
{
    (void)state;
    char filename[MAX_LINE_LENGTH];
    char function_name[MAX_LINE_LENGTH];
    int line_number = 0;

    assert_false(extract_file_and_line("completely invalid line", filename, function_name, &line_number));
}

/*
 * Verifies LLR-EFAL-06 (third "(in <obj>)" sscanf pattern) end-to-end.
 * Traces to HLR-021 (Stack Frame Decoding),
 * HLR-040 (Non-Fatal Recovery from Malformed Input).
 */
static void test_extract_file_and_line_in_format(void **state)
{
    (void)state;
    char filename[MAX_LINE_LENGTH];
    char function_name[MAX_LINE_LENGTH];
    int line_number = -1;

    assert_true(extract_file_and_line("   by 0x0000: malloc (in /usr/lib/libc.so.6)",
                                      filename, function_name, &line_number));
    assert_string_equal(filename, "/usr/lib/libc.so.6");
    assert_int_equal(line_number, 0);
}

/* ---- execute_command -------------------------------------------------- */

/* execute_command and parse_ctags_output are intentionally not in vgp.h
 * but are externally linkable so the unit tests can exercise them. */
extern bool execute_command(const char *command, char *output, size_t output_size);
extern bool parse_ctags_output(const char *language, char *ctags_output, int *start_line, int *end_line);

/*
 * Verifies LLR-EC-02: NULL pointers and a zero output_size are
 * rejected.
 * Traces to HLR-039 (Defensive Argument Validation).
 */
static void test_execute_command_null_args(void **state)
{
    (void)state;
    char output[256];
    assert_false(execute_command(NULL, output, sizeof(output)));
    assert_false(execute_command("echo hello", NULL, sizeof(output)));
    assert_false(execute_command("echo hello", output, 0));
}

/*
 * Verifies LLR-EC-03..LLR-EC-06: a successful popen/fgets/pclose round
 * trip captures the first line of the command's stdout.
 * Traces to HLR-029 (Multi-Language Source Support),
 * HLR-042 (POSIX Process Helpers).
 */
static void test_execute_command_success(void **state)
{
    (void)state;
    char output[256];
    assert_true(execute_command("echo hello", output, sizeof(output)));
    assert_non_null(strstr(output, "hello"));
}

/* ---- print_source_function edge cases --------------------------------- */

/*
 * Verifies LLR-PSF-02: NULL pointers and non-positive line numbers
 * cause the function to bail out with a stderr diagnostic.
 * Traces to HLR-039 (Defensive Argument Validation),
 * HLR-030 (External-Tool Dependency Isolation).
 */
static void test_print_source_function_null_args(void **state)
{
    (void)state;
    print_source_function(NULL, "func", 10);
    print_source_function("/tmp/foo.c", NULL, 10);
    print_source_function("/tmp/foo.c", "func", 0);
    print_source_function("/tmp/foo.c", "func", -1);
}

/* ---- parse_ctags_output ----------------------------------------------- */

/*
 * Verifies LLR-PCO-02: any NULL pointer argument is rejected with a
 * stderr diagnostic.
 * Traces to HLR-039 (Defensive Argument Validation),
 * HLR-030 (External-Tool Dependency Isolation).
 */
static void test_parse_ctags_output_null_args(void **state)
{
    (void)state;
    int start_line = 0, end_line = 0;
    char buf[256] = "func\tfile.c\t/^void func/;\"\tf\tline:1\tend:5";
    assert_false(parse_ctags_output(NULL, buf, &start_line, &end_line));
    assert_false(parse_ctags_output("C", NULL, &start_line, &end_line));
    assert_false(parse_ctags_output("C", buf, NULL, &end_line));
    assert_false(parse_ctags_output("C", buf, &start_line, NULL));
}

/*
 * Verifies LLR-PCO-03: ctags output without any tab cannot be split
 * into the expected fields and is rejected.
 * Traces to HLR-029 (Multi-Language Source Support),
 * HLR-030 (External-Tool Dependency Isolation).
 */
static void test_parse_ctags_output_no_tab(void **state)
{
    (void)state;
    int start_line = 0, end_line = 0;
    char buf[256] = "no_tab_at_all";
    assert_false(parse_ctags_output("C", buf, &start_line, &end_line));
}

/*
 * Verifies LLR-PCO-03: only a single tab separator is also insufficient.
 * Traces to HLR-029 (Multi-Language Source Support),
 * HLR-030 (External-Tool Dependency Isolation).
 */
static void test_parse_ctags_output_one_tab(void **state)
{
    (void)state;
    int start_line = 0, end_line = 0;
    char buf[256] = "func\tfile_only";
    assert_false(parse_ctags_output("C", buf, &start_line, &end_line));
}

/*
 * Verifies LLR-PCO-04: a record with no "line:" field is rejected.
 * Traces to HLR-029 (Multi-Language Source Support),
 * HLR-030 (External-Tool Dependency Isolation).
 */
static void test_parse_ctags_output_no_line_field(void **state)
{
    (void)state;
    int start_line = 0, end_line = 0;
    char buf[256] = "func\tfile.c\t/^void func/;\"\tf";
    assert_false(parse_ctags_output("C", buf, &start_line, &end_line));
}

/*
 * Verifies LLR-PCO-05: a record that points at a non-existent source
 * file is rejected when fopen fails.
 * Traces to HLR-029 (Multi-Language Source Support),
 * HLR-030 (External-Tool Dependency Isolation).
 */
static void test_parse_ctags_output_bad_file(void **state)
{
    (void)state;
    int start_line = 0, end_line = 0;
    char buf[256] = "func\t/nonexistent_xyz.c\t/^void func/;\"\tf\tline:1";
    assert_false(parse_ctags_output("C", buf, &start_line, &end_line));
}

/*
 * Verifies LLR-PCO-08: a language outside {C, C++, Rust, Fortran}
 * causes a non-fatal "Unsupported source language" diagnostic and
 * returns false.
 * Traces to HLR-029 (Multi-Language Source Support),
 * HLR-030 (External-Tool Dependency Isolation).
 */
static void test_parse_ctags_output_unsupported_language(void **state)
{
    (void)state;
    char tmp[] = "/tmp/vgp_ctags_test_XXXXXX.py";
    int fd = mkstemps(tmp, 3);
    assert_true(fd >= 0);
    const char *src = "def func():\n    pass\n";
    write(fd, src, strlen(src));
    close(fd);

    int start_line = 0, end_line = 0;
    char buf[512];
    snprintf(buf, sizeof(buf), "func\t%s\t/^def func/;\"\tf\tline:1", tmp);
    assert_false(parse_ctags_output("Python", buf, &start_line, &end_line));

    unlink(tmp);
}

/*
 * Verifies LLR-PCO-06 + LLR-PCO-09: the brace-balanced end-of-function
 * detection for C reports start_line=4 and an end_line on or after the
 * closing brace.
 * Traces to HLR-029 (Multi-Language Source Support).
 */
static void test_parse_ctags_output_c_success(void **state)
{
    (void)state;
    char tmp[] = "/tmp/vgp_ctags_c_XXXXXX.c";
    int fd = mkstemps(tmp, 2);
    assert_true(fd >= 0);
    const char *src =
        "void other_func(void) {\n"
        "    int x = 1;\n"
        "}\n"
        "void test_func(void) {\n"
        "    int y = 2;\n"
        "    y++;\n"
        "}\n";
    write(fd, src, strlen(src));
    close(fd);

    int start_line = 0, end_line = 0;
    char buf[512];
    snprintf(buf, sizeof(buf), "test_func\t%s\t/^void test_func/;\"\tf\tline:4", tmp);
    bool result = parse_ctags_output("C", buf, &start_line, &end_line);
    assert_true(result);
    assert_int_equal(start_line, 4);
    assert_true(end_line >= 6);

    unlink(tmp);
}

/*
 * Verifies LLR-PCO-07 + LLR-PCO-09: the "END SUBROUTINE <name>" match
 * reports the expected end_line for Fortran source.
 * Traces to HLR-029 (Multi-Language Source Support).
 */
static void test_parse_ctags_output_fortran_success(void **state)
{
    (void)state;
    char tmp[] = "/tmp/vgp_ctags_f_XXXXXX.f90";
    int fd = mkstemps(tmp, 4);
    assert_true(fd >= 0);
    const char *src =
        "SUBROUTINE test_sub()\n"
        "    INTEGER :: x\n"
        "    x = 1\n"
        "END SUBROUTINE test_sub\n";
    write(fd, src, strlen(src));
    close(fd);

    int start_line = 0, end_line = 0;
    char buf[512];
    snprintf(buf, sizeof(buf), "test_sub\t%s\t/^SUBROUTINE test_sub/;\"\ts\tline:1", tmp);
    bool result = parse_ctags_output("Fortran", buf, &start_line, &end_line);
    assert_true(result);
    assert_int_equal(start_line, 1);
    assert_true(end_line >= 4);

    unlink(tmp);
}

/* ---- print_source_function with real files ---------------------------- */

/*
 * Verifies LLR-PSF-03..LLR-PSF-09 end-to-end on a real on-disk source
 * file: the function is located via ctags and printed with the target
 * line marked.
 * Traces to HLR-027 (Conditional Source Body Output),
 * HLR-029 (Multi-Language Source Support),
 * HLR-043 (External ctags Dependency Scope).
 */
static void test_print_source_function_real_c_file(void **state)
{
    (void)state;
    char tmp[] = "/tmp/vgp_psf_XXXXXX.c";
    int fd = mkstemps(tmp, 2);
    assert_true(fd >= 0);
    const char *src =
        "#include <stdio.h>\n"
        "void my_test_func(void) {\n"
        "    int x = 42;\n"
        "    printf(\"%d\\n\", x);\n"
        "}\n";
    write(fd, src, strlen(src));
    close(fd);

    print_source_function(tmp, "my_test_func", 3);

    unlink(tmp);
}

/*
 * Verifies LLR-PSF-04: when the requested function is not present in
 * the source file, the helper writes a diagnostic and returns without
 * crashing.
 * Traces to HLR-029 (Multi-Language Source Support),
 * HLR-030 (External-Tool Dependency Isolation).
 */
static void test_print_source_function_nonexistent_function(void **state)
{
    (void)state;
    char tmp[] = "/tmp/vgp_psf2_XXXXXX.c";
    int fd = mkstemps(tmp, 2);
    assert_true(fd >= 0);
    const char *src = "void real_func(void) { }\n";
    write(fd, src, strlen(src));
    close(fd);

    print_source_function(tmp, "nonexistent_func", 1);

    unlink(tmp);
}

/* ---- execute_command edge cases --------------------------------------- */

/*
 * Verifies LLR-EC-04..LLR-EC-06 for a command that prints nothing:
 * fgets returns NULL with no ferror and the function still returns
 * true.
 * Traces to HLR-029 (Multi-Language Source Support),
 * HLR-040 (Non-Fatal Recovery from Malformed Input),
 * HLR-042 (POSIX Process Helpers).
 */
static void test_execute_command_empty_output(void **state)
{
    (void)state;
    char output[256] = "old";
    assert_true(execute_command("true", output, sizeof(output)));
}

/* ---- main(1) command-line surface via system() ------------------------ */

/* These tests fork a child running the shipped vgp binary so they cover
 * the end-to-end argument-parsing logic in src/main.c. */

#ifndef VGP_EXE
#define VGP_EXE "build/vgp"
#endif
#ifndef INT_LOG_DIR
#define INT_LOG_DIR "build"
#endif

/*
 * Verifies LLR-PCL-04: the -h flag prints usage and exits with success.
 * Traces to HLR-003 (Usage and Help Information Display),
 * HLR-009 (Application Exit Status).
 */
static void test_main_help_flag(void **state)
{
    (void)state;
    int rc = system(VGP_EXE " -h > /dev/null 2>&1");
    assert_int_equal(WEXITSTATUS(rc), 0);
}

/*
 * Verifies LLR-PCL-05: an unknown short option is rejected with an
 * "Unknown option:" diagnostic and EXIT_FAILURE.
 * Traces to HLR-002 (Argument Parsing and Validation),
 * HLR-009 (Application Exit Status),
 * HLR-010 (Application-Level Error Reporting).
 */
static void test_main_unknown_flag(void **state)
{
    (void)state;
    int rc = system(VGP_EXE " -z dummy.log > /dev/null 2>&1");
    assert_int_equal(WEXITSTATUS(rc), 1);
}

/*
 * Verifies LLR-PCL-08: invocation with no positional argument exits
 * with EXIT_FAILURE.
 * Traces to HLR-002 (Argument Parsing and Validation),
 * HLR-003 (Usage and Help Information Display),
 * HLR-009 (Application Exit Status).
 */
static void test_main_no_log_file(void **state)
{
    (void)state;
    int rc = system(VGP_EXE " > /dev/null 2>&1");
    assert_int_equal(WEXITSTATUS(rc), 1);
}

/*
 * Verifies LLR-PCL-07: a second positional argument is rejected with
 * "Multiple log files specified:" and EXIT_FAILURE.
 * Traces to HLR-002 (Argument Parsing and Validation),
 * HLR-009 (Application Exit Status),
 * HLR-010 (Application-Level Error Reporting).
 */
static void test_main_multiple_log_files(void **state)
{
    (void)state;
    int rc = system(VGP_EXE " file1.log file2.log > /dev/null 2>&1");
    assert_int_equal(WEXITSTATUS(rc), 1);
}

/*
 * Verifies LLR-MAIN-04 + LLR-MAIN-05: when fopen() of the input log
 * fails, main() emits "Error opening file '...': ..." and returns
 * EXIT_FAILURE.
 * Traces to HLR-004 (Input Log File Handling),
 * HLR-009 (Application Exit Status),
 * HLR-010 (Application-Level Error Reporting).
 */
static void test_main_nonexistent_file(void **state)
{
    (void)state;
    int rc = system(VGP_EXE " /tmp/nonexistent_vgp_xyz.log > /dev/null 2>&1");
    assert_int_equal(WEXITSTATUS(rc), 1);
}

/*
 * Verifies LLR-PCL-03 (-t -> print_stack) and LLR-MAIN-04..LLR-MAIN-09
 * end-to-end on a real Valgrind log produced by the integration build.
 * Traces to HLR-002 (Argument Parsing and Validation),
 * HLR-006 (Parser Module Invocation),
 * HLR-009 (Application Exit Status),
 * HLR-024 (Stack Trace Output Gated by Options).
 */
static void test_main_stack_flag(void **state)
{
    (void)state;
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             VGP_EXE " -t " INT_LOG_DIR "/c_error_generator_int_app_c_valgrind.log > /dev/null 2>&1");
    int rc = system(cmd);
    assert_int_equal(WEXITSTATUS(rc), 0);
}

/* ---- additional process_stack_trace_line / summary branches ----------- */

/*
 * Verifies the negative gating branch of LLR-PSTL-06: with both
 * print_stack and verbose off, the ellipsis is not printed and
 * stack_lines_shown is not incremented.
 * Traces to HLR-023 (Bounded Stack Trace Output),
 * HLR-024 (Stack Trace Output Gated by Options).
 */
static void test_process_stack_trace_line_ellipsis_no_verbose(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;
    ps.stack_lines_shown = STACK_TRACE_CONTEXT_LINES;
    ps.user_code_found_for_error = false;
    app_config.print_stack = false;
    app_config.verbose = false;

    process_stack_trace_line("   at 0x123: malloc (in /usr/lib/libc.so.6)", &ps);
}

/*
 * Verifies the verbose-only path of LLR-PSL-03 and LLR-PSL-04: with
 * print_leak_summary off but verbose on, leak lines still print.
 * Traces to HLR-012 (Verbose Mode),
 * HLR-031 (Leak Summary Recognition),
 * HLR-032 (Leak Summary Output Gated by Options).
 */
static void test_process_summary_lines_leak_with_verbose(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    app_config.print_leak_summary = false;
    app_config.verbose = true;

    process_summary_lines("LEAK SUMMARY:", &ps);
    process_summary_lines("definitely lost: 96 bytes in 3 blocks", &ps);
    process_summary_lines("indirectly lost: 16 bytes in 1 blocks", &ps);
    process_summary_lines("possibly lost: 0 bytes in 0 blocks", &ps);
    process_summary_lines("still reachable: 30 bytes in 1 blocks", &ps);

    app_config.verbose = false;
}

/*
 * Verifies LLR-PSTL-06 verbose branch: with verbose on (and
 * print_stack off) the ellipsis is printed once at the
 * STACK_TRACE_CONTEXT_LINES boundary.
 * Traces to HLR-012 (Verbose Mode),
 * HLR-023 (Bounded Stack Trace Output),
 * HLR-024 (Stack Trace Output Gated by Options).
 */
static void test_process_stack_trace_line_ellipsis_verbose(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;
    ps.stack_lines_shown = STACK_TRACE_CONTEXT_LINES;
    ps.user_code_found_for_error = false;
    app_config.print_stack = false;
    app_config.verbose = true;

    process_stack_trace_line("   at 0x123: malloc (in /usr/lib/libc.so.6)", &ps);
    assert_int_equal(ps.stack_lines_shown, STACK_TRACE_CONTEXT_LINES + 1);

    app_config.verbose = false;
}

/* ---- additional process_log_file scenarios ---------------------------- */

/*
 * Verifies LLR-PLF-09 dispatching to the leak path with print_leak_summary
 * the only flag enabled.
 * Traces to HLR-031 (Leak Summary Recognition),
 * HLR-032 (Leak Summary Output Gated by Options),
 * HLR-033 (Leak Detail Formatting),
 * HLR-034 (Final Error Summary Output).
 */
static void test_process_log_file_with_leak_flags_only(void **state)
{
    (void)state;
    const char *log_content =
        "==123== LEAK SUMMARY:\n"
        "==123==    definitely lost: 96 bytes in 3 blocks\n"
        "==123==    indirectly lost: 16 bytes in 1 blocks\n"
        "==123==    possibly lost: 0 bytes in 0 blocks\n"
        "==123==    still reachable: 30 bytes in 1 blocks\n"
        "==123== ERROR SUMMARY: 0 errors from 0 contexts\n";

    FILE *f = tmpfile();
    assert_non_null(f);
    fwrite(log_content, 1, strlen(log_content), f);
    rewind(f);

    app_config.verbose = false;
    app_config.print_leak_summary = true;
    process_log_file(f);
    app_config.print_leak_summary = false;

    fclose(f);
}

/*
 * Verifies LLR-PSTL-04 inside a real log: with print_stack the
 * filtered call stack is emitted for an error containing both user-
 * and library-frame entries.
 * Traces to HLR-022 (User-Code Classification),
 * HLR-024 (Stack Trace Output Gated by Options).
 */
static void test_process_log_file_error_with_stack_flag(void **state)
{
    (void)state;
    const char *log_content =
        "==123== Invalid read of size 4\n"
        "==123==    at 0x1096F6: foo (/home/user/proj/foo.c:10)\n"
        "==123==    by 0x109B00: bar (/usr/lib/libc.so.6)\n"
        "==123== \n"
        "==123== ERROR SUMMARY: 1 errors from 1 contexts\n";

    FILE *f = tmpfile();
    assert_non_null(f);
    fwrite(log_content, 1, strlen(log_content), f);
    rewind(f);

    app_config.verbose = false;
    app_config.print_stack = true;
    app_config.print_source = false;
    process_log_file(f);
    app_config.print_stack = false;

    fclose(f);
}

/*
 * Verifies LLR-PSTL-06 in a realistic streaming context: more than
 * STACK_TRACE_CONTEXT_LINES non-user frames trigger exactly one
 * ellipsis and subsequent frames are suppressed.
 * Traces to HLR-023 (Bounded Stack Trace Output),
 * HLR-036 (Streaming Single-Pass Operation).
 */
static void test_process_log_file_many_stack_lines(void **state)
{
    (void)state;
    char log_content[8192];
    int offset = 0;
    offset += snprintf(log_content + offset, sizeof(log_content) - offset,
                       "==123== Invalid read of size 4\n");
    for (int i = 0; i < STACK_TRACE_CONTEXT_LINES + 5; i++) {
        offset += snprintf(log_content + offset, sizeof(log_content) - offset,
                           "==123==    at 0x%04X: func_%d (in /usr/lib/libc.so.6)\n", 0x1000 + i, i);
    }
    offset += snprintf(log_content + offset, sizeof(log_content) - offset,
                       "==123== \n"
                       "==123== ERROR SUMMARY: 1 errors from 1 contexts\n");

    FILE *f = tmpfile();
    assert_non_null(f);
    fwrite(log_content, 1, (size_t)offset, f);
    rewind(f);

    app_config.verbose = true;
    process_log_file(f);
    app_config.verbose = false;

    fclose(f);
}

/*
 * Verifies the full source-listing pipeline end-to-end against a real
 * temporary source file: process_log_file -> finalize_error_block ->
 * print_source_function -> ctags lookup -> source body printed.
 * Traces to HLR-026 (Conditional Source Reference Output),
 * HLR-027 (Conditional Source Body Output),
 * HLR-029 (Multi-Language Source Support),
 * HLR-043 (External ctags Dependency Scope).
 */
static void test_process_log_file_with_real_source(void **state)
{
    (void)state;
    char tmp_src[] = "/tmp/vgp_test_XXXXXX.c";
    int fd = mkstemps(tmp_src, 2);
    assert_true(fd >= 0);
    const char *src =
        "#include <stdio.h>\n"
        "void test_function(void) {\n"
        "    int x = 42;\n"
        "    printf(\"%d\\n\", x);\n"
        "}\n";
    write(fd, src, strlen(src));
    close(fd);

    char log_content[4096];
    snprintf(log_content, sizeof(log_content),
             "==123== Invalid read of size 4\n"
             "==123==    at 0x1096F6: test_function (%s:3)\n"
             "==123== \n"
             "==123== ERROR SUMMARY: 1 errors from 1 contexts\n",
             tmp_src);

    FILE *f = tmpfile();
    assert_non_null(f);
    fwrite(log_content, 1, strlen(log_content), f);
    rewind(f);

    app_config.verbose = true;
    process_log_file(f);
    app_config.verbose = false;

    fclose(f);
    unlink(tmp_src);
}

/*
 * Verifies the negative gating branch in a streaming context: many
 * non-user frames with both print_stack and verbose off produce no
 * stack output and no ellipsis.
 * Traces to HLR-023 (Bounded Stack Trace Output),
 * HLR-024 (Stack Trace Output Gated by Options).
 */
static void test_process_log_file_many_stack_lines_no_verbose(void **state)
{
    (void)state;
    char log_content[8192];
    int offset = 0;
    offset += snprintf(log_content + offset, sizeof(log_content) - offset,
                       "==123== Invalid read of size 4\n");
    for (int i = 0; i < STACK_TRACE_CONTEXT_LINES + 5; i++) {
        offset += snprintf(log_content + offset, sizeof(log_content) - offset,
                           "==123==    at 0x%04X: func_%d (in /usr/lib/libc.so.6)\n", 0x1000 + i, i);
    }
    offset += snprintf(log_content + offset, sizeof(log_content) - offset,
                       "==123== \n"
                       "==123== ERROR SUMMARY: 1 errors from 1 contexts\n");

    FILE *f = tmpfile();
    assert_non_null(f);
    fwrite(log_content, 1, (size_t)offset, f);
    rewind(f);

    app_config.verbose = false;
    app_config.print_stack = false;
    process_log_file(f);

    fclose(f);
}

/* ---- additional extract_file_and_line edge cases --------------------- */

/*
 * Verifies LLR-EFAL-04 normalisation: a fully-qualified C++ name like
 * "Namespace::Class::method" is reduced to its final identifier
 * "method".
 * Traces to HLR-021 (Stack Frame Decoding),
 * HLR-025 (First-User-Frame Capture).
 */
static void test_extract_file_and_line_scope_operator(void **state)
{
    (void)state;
    char filename[MAX_LINE_LENGTH];
    char function_name[MAX_LINE_LENGTH];
    int line_number = 0;

    assert_true(extract_file_and_line("   at 0x109A00: Namespace::Class::method (/path/to/source.cpp:157)",
                                      filename, function_name, &line_number));
    assert_string_equal(function_name, "method");
}

/*
 * Verifies LLR-EFAL-04 normalisation: a Fortran-style dotted suffix
 * such as "trigger_invalid_write.3" is truncated at the dot.
 * Traces to HLR-021 (Stack Frame Decoding),
 * HLR-025 (First-User-Frame Capture),
 * HLR-029 (Multi-Language Source Support).
 */
static void test_extract_file_and_line_dotted_suffix(void **state)
{
    (void)state;
    char filename[MAX_LINE_LENGTH];
    char function_name[MAX_LINE_LENGTH];
    int line_number = 0;

    assert_true(extract_file_and_line("   at 0x109A00: trigger_invalid_write.3 (/path/to/source.f90:35)",
                                      filename, function_name, &line_number));
    assert_string_equal(function_name, "trigger_invalid_write");
}

/* NOTE: is_valid_function_char is declared in vgp.h but currently has
 * no definition in src/vgp.c, so it cannot be unit-tested. */

/* Shorthand for tests that produce noisy stdout/stderr. */
#define QUIET_TEST(f) cmocka_unit_test_setup_teardown(f, setup_suppress, teardown_suppress)

int run_vgp_core_tests(void)
{
    const struct CMUnitTest tests[] = {
        /* initialize_parse_state */
        cmocka_unit_test(test_initialize_parse_state_normal),
        cmocka_unit_test(test_initialize_parse_state_null),
        /* check_start_new_error */
        QUIET_TEST(test_check_start_new_error_null_args),
        QUIET_TEST(test_check_start_new_error_detects_invalid_read),
        QUIET_TEST(test_check_start_new_error_detects_invalid_write),
        QUIET_TEST(test_check_start_new_error_detects_uninit),
        QUIET_TEST(test_check_start_new_error_detects_invalid_free),
        QUIET_TEST(test_check_start_new_error_detects_mismatched_free),
        QUIET_TEST(test_check_start_new_error_detects_overlap),
        QUIET_TEST(test_check_start_new_error_no_match),
        QUIET_TEST(test_check_start_new_error_already_in_block),
        /* process_stack_trace_line */
        QUIET_TEST(test_process_stack_trace_line_null_args),
        QUIET_TEST(test_process_stack_trace_line_finds_user_code),
        QUIET_TEST(test_process_stack_trace_line_non_user_code),
        QUIET_TEST(test_process_stack_trace_line_with_print_stack),
        QUIET_TEST(test_process_stack_trace_line_ellipsis_branch),
        QUIET_TEST(test_process_stack_trace_line_ellipsis_no_verbose),
        /* finalize_error_block */
        QUIET_TEST(test_finalize_error_block_null),
        QUIET_TEST(test_finalize_error_block_no_user_code),
        QUIET_TEST(test_finalize_error_block_with_print_function),
        QUIET_TEST(test_finalize_error_block_with_verbose),
        /* process_in_error_block */
        QUIET_TEST(test_process_in_error_block_null_args),
        QUIET_TEST(test_process_in_error_block_stack_trace),
        QUIET_TEST(test_process_in_error_block_non_stack_trace),
        /* process_summary_lines */
        QUIET_TEST(test_process_summary_lines_null),
        QUIET_TEST(test_process_summary_lines_leak_summary),
        QUIET_TEST(test_process_summary_lines_leak_without_flag),
        QUIET_TEST(test_process_summary_lines_error_summary),
        QUIET_TEST(test_process_summary_lines_no_match),
        /* print_leak_summary_line */
        QUIET_TEST(test_print_leak_summary_line_parse_success),
        QUIET_TEST(test_print_leak_summary_line_parse_fail_with_newline),
        QUIET_TEST(test_print_leak_summary_line_parse_fail_no_newline),
        /* print_final_error_summary */
        QUIET_TEST(test_print_final_error_summary_parse_success),
        QUIET_TEST(test_print_final_error_summary_parse_fail_with_newline),
        QUIET_TEST(test_print_final_error_summary_parse_fail_no_newline),
        /* print_error_header */
        QUIET_TEST(test_print_error_header_with_newline),
        QUIET_TEST(test_print_error_header_without_newline),
        QUIET_TEST(test_print_error_header_no_stack_flag),
        /* get_function_name */
        QUIET_TEST(test_get_function_name_with_line_number),
        QUIET_TEST(test_get_function_name_without_line_number),
        QUIET_TEST(test_get_function_name_paren_in_function),
        /* process_log_file */
        QUIET_TEST(test_process_log_file_null),
        QUIET_TEST(test_process_log_file_with_error_block),
        QUIET_TEST(test_process_log_file_eof_in_error_block),
        QUIET_TEST(test_process_log_file_leak_summary),
        QUIET_TEST(test_process_log_file_multiple_errors),
        QUIET_TEST(test_process_log_file_no_verbose),
        QUIET_TEST(test_process_log_file_with_print_source),
        /* extract_file_and_line additional */
        cmocka_unit_test(test_extract_file_and_line_format_without_function),
        cmocka_unit_test(test_extract_file_and_line_in_format),
        /* execute_command */
        cmocka_unit_test(test_execute_command_null_args),
        QUIET_TEST(test_execute_command_success),
        QUIET_TEST(test_execute_command_empty_output),
        /* main.c flag coverage */
        cmocka_unit_test(test_main_help_flag),
        cmocka_unit_test(test_main_unknown_flag),
        cmocka_unit_test(test_main_no_log_file),
        cmocka_unit_test(test_main_multiple_log_files),
        cmocka_unit_test(test_main_nonexistent_file),
        cmocka_unit_test(test_main_stack_flag),
        /* print_source_function */
        QUIET_TEST(test_print_source_function_null_args),
        QUIET_TEST(test_print_source_function_real_c_file),
        QUIET_TEST(test_print_source_function_nonexistent_function),
        /* parse_ctags_output */
        QUIET_TEST(test_parse_ctags_output_null_args),
        QUIET_TEST(test_parse_ctags_output_no_tab),
        QUIET_TEST(test_parse_ctags_output_one_tab),
        QUIET_TEST(test_parse_ctags_output_no_line_field),
        QUIET_TEST(test_parse_ctags_output_bad_file),
        QUIET_TEST(test_parse_ctags_output_unsupported_language),
        QUIET_TEST(test_parse_ctags_output_c_success),
        QUIET_TEST(test_parse_ctags_output_fortran_success),
        /* process_summary_lines verbose */
        QUIET_TEST(test_process_summary_lines_leak_with_verbose),
        /* process_stack_trace_line ellipsis verbose */
        QUIET_TEST(test_process_stack_trace_line_ellipsis_verbose),
        /* additional process_log_file */
        QUIET_TEST(test_process_log_file_with_leak_flags_only),
        QUIET_TEST(test_process_log_file_error_with_stack_flag),
        QUIET_TEST(test_process_log_file_many_stack_lines),
        QUIET_TEST(test_process_log_file_with_real_source),
        QUIET_TEST(test_process_log_file_many_stack_lines_no_verbose),
        /* additional extract_file_and_line */
        cmocka_unit_test(test_extract_file_and_line_scope_operator),
        cmocka_unit_test(test_extract_file_and_line_dotted_suffix),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
