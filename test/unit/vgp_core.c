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

/* Helpers to suppress stdout/stderr during noisy tests */
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

/* ---- Tests for initialize_parse_state ---- */

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

static void test_initialize_parse_state_null(void **state)
{
    (void)state;
    /* Should not crash */
    initialize_parse_state(NULL);
}

/* ---- Tests for check_start_new_error ---- */

static void test_check_start_new_error_null_args(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_false(check_start_new_error(NULL, &ps));
    assert_false(check_start_new_error("some line", NULL));
    assert_false(check_start_new_error(NULL, NULL));
}

static void test_check_start_new_error_detects_invalid_read(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_true(check_start_new_error("Invalid read of size 4", &ps));
    assert_true(ps.in_error_block);
    assert_int_equal(ps.error_count, 1);
}

static void test_check_start_new_error_detects_invalid_write(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_true(check_start_new_error("Invalid write of size 8", &ps));
    assert_true(ps.in_error_block);
}

static void test_check_start_new_error_detects_uninit(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_true(check_start_new_error("depends on uninitialised value(s)", &ps));
}

static void test_check_start_new_error_detects_invalid_free(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_true(check_start_new_error("Invalid free() / delete / delete[]", &ps));
}

static void test_check_start_new_error_detects_mismatched_free(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_true(check_start_new_error("Mismatched free() / delete / delete[]", &ps));
}

static void test_check_start_new_error_detects_overlap(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_true(check_start_new_error("Source and destination overlap in memcpy", &ps));
}

static void test_check_start_new_error_no_match(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    assert_false(check_start_new_error("HEAP SUMMARY:", &ps));
    assert_false(ps.in_error_block);
}

static void test_check_start_new_error_already_in_block(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;
    /* Should not start a new error when already in a block */
    assert_false(check_start_new_error("Invalid read of size 4", &ps));
}

/* ---- Tests for process_stack_trace_line ---- */

static void test_process_stack_trace_line_null_args(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    /* Should not crash */
    process_stack_trace_line(NULL, &ps);
    process_stack_trace_line("   at 0x123: foo (bar.c:10)", NULL);
}

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

    /* This line is not user code and we've reached the context limit */
    process_stack_trace_line("   at 0x123: malloc (in /usr/lib/libc.so.6)", &ps);
    assert_int_equal(ps.stack_lines_shown, STACK_TRACE_CONTEXT_LINES + 1);

    app_config.print_stack = false;
}

/* ---- Tests for finalize_error_block ---- */

static void test_finalize_error_block_null(void **state)
{
    (void)state;
    /* Should not crash */
    finalize_error_block(NULL);
}

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

    /* Will try to call print_source_function which will fail on ctags, but won't crash */
    finalize_error_block(&ps);
    assert_false(ps.in_error_block);

    app_config.verbose = false;
}

/* ---- Tests for process_in_error_block ---- */

static void test_process_in_error_block_null_args(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    /* Should not crash */
    process_in_error_block(NULL, &ps);
    process_in_error_block("some line", NULL);
}

static void test_process_in_error_block_stack_trace(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;
    app_config.verbose = true;

    process_in_error_block("   at 0x1096F6: my_func (/home/user/project/main.c:86)", &ps);
    assert_true(ps.in_error_block); /* Still in block */

    app_config.verbose = false;
}

static void test_process_in_error_block_non_stack_trace(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.in_error_block = true;

    process_in_error_block("some other line", &ps);
    assert_false(ps.in_error_block); /* Block finalized */
}

/* ---- Tests for process_summary_lines ---- */

static void test_process_summary_lines_null(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    /* Should not crash */
    process_summary_lines(NULL, &ps);
}

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

static void test_process_summary_lines_leak_without_flag(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    app_config.print_leak_summary = false;
    app_config.verbose = false;

    /* These should not print anything (exercising the false branch of the flag check) */
    process_summary_lines("LEAK SUMMARY:", &ps);
    process_summary_lines("definitely lost: 96 bytes in 3 blocks", &ps);
    process_summary_lines("indirectly lost: 16 bytes in 1 blocks", &ps);
    process_summary_lines("possibly lost: 0 bytes in 0 blocks", &ps);
    process_summary_lines("still reachable: 30 bytes in 1 blocks", &ps);
}

static void test_process_summary_lines_error_summary(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.error_count = 5;

    process_summary_lines("ERROR SUMMARY: 8 errors from 5 contexts", &ps);
}

static void test_process_summary_lines_no_match(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    /* Line that doesn't match any summary pattern */
    process_summary_lines("For a detailed leak analysis, rerun with: --leak-check=full", &ps);
}

/* ---- Tests for print_leak_summary_line ---- */

static void test_print_leak_summary_line_parse_success(void **state)
{
    (void)state;
    /* Exercises the sscanf success branch */
    print_leak_summary_line("definitely lost: 96 bytes in 3 blocks", "Definitely Lost");
}

static void test_print_leak_summary_line_parse_fail_with_newline(void **state)
{
    (void)state;
    /* Exercises the sscanf failure branch with newline */
    print_leak_summary_line("malformed leak line\n", "Unknown");
}

static void test_print_leak_summary_line_parse_fail_no_newline(void **state)
{
    (void)state;
    /* Exercises the sscanf failure branch without newline */
    print_leak_summary_line("malformed leak line", "Unknown");
}

/* ---- Tests for print_final_error_summary ---- */

static void test_print_final_error_summary_parse_success(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    ps.error_count = 3;
    print_final_error_summary("ERROR SUMMARY: 5 errors from 3 contexts", &ps);
}

static void test_print_final_error_summary_parse_fail_with_newline(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    print_final_error_summary("malformed error summary\n", &ps);
}

static void test_print_final_error_summary_parse_fail_no_newline(void **state)
{
    (void)state;
    ParseState ps;
    initialize_parse_state(&ps);
    print_final_error_summary("malformed error summary", &ps);
}

/* ---- Tests for print_error_header ---- */

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

/* ---- Tests for get_function_name ---- */

static void test_get_function_name_with_line_number(void **state)
{
    (void)state;
    char newline[MAX_LINE_LENGTH];
    get_function_name("   at 0x1096F6: my_func (/home/user/project/main.c:86)", newline);
    assert_non_null(strstr(newline, "my_func"));
    assert_non_null(strstr(newline, "main.c"));
    assert_non_null(strstr(newline, "86"));
}

static void test_get_function_name_without_line_number(void **state)
{
    (void)state;
    char newline[MAX_LINE_LENGTH];
    get_function_name("   at 0x123: malloc (in /usr/lib/libc.so.6)", newline);
    assert_non_null(strstr(newline, "?(?:0)"));
}

static void test_get_function_name_paren_in_function(void **state)
{
    (void)state;
    char newline[MAX_LINE_LENGTH];
    /* Function name with parenthesis should be stripped */
    get_function_name("   at 0x123: Class::method(int) (/path/to/file.cpp:42)", newline);
    assert_non_null(strstr(newline, "42"));
}

/* ---- Tests for process_log_file ---- */

static void test_process_log_file_null(void **state)
{
    (void)state;
    /* Should not crash */
    process_log_file(NULL);
}

static void test_process_log_file_with_error_block(void **state)
{
    (void)state;
    /* Create a temporary file with a short valgrind-like log */
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

static void test_process_log_file_eof_in_error_block(void **state)
{
    (void)state;
    /* File ends while still inside an error block (no empty line terminator) */
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

static void test_process_log_file_no_verbose(void **state)
{
    (void)state;
    /* Exercise the non-verbose code paths */
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

/* ---- Tests for extract_file_and_line additional branches ---- */

static void test_extract_file_and_line_format_without_function(void **state)
{
    (void)state;
    char filename[MAX_LINE_LENGTH];
    char function_name[MAX_LINE_LENGTH];
    int line_number = 0;

    /* Format: "   at 0x0000: filename:line" - the second sscanf branch */
    /* This is hard to trigger since the first sscanf is greedy, test the no-match path */
    assert_false(extract_file_and_line("completely invalid line", filename, function_name, &line_number));
}

static void test_extract_file_and_line_in_format(void **state)
{
    (void)state;
    char filename[MAX_LINE_LENGTH];
    char function_name[MAX_LINE_LENGTH];
    int line_number = -1;

    /* "in /path/to/lib" format */
    assert_true(extract_file_and_line("   by 0x0000: malloc (in /usr/lib/libc.so.6)",
                                      filename, function_name, &line_number));
    assert_string_equal(filename, "/usr/lib/libc.so.6");
    assert_int_equal(line_number, 0);
}

/* ---- Tests for execute_command ---- */
/* NOTE: execute_command is not declared in vgp.h, so we declare it here */
extern bool execute_command(const char *command, char *output, size_t output_size);
extern bool parse_ctags_output(const char *language, char *ctags_output, int *start_line, int *end_line);

static void test_execute_command_null_args(void **state)
{
    (void)state;
    char output[256];
    assert_false(execute_command(NULL, output, sizeof(output)));
    assert_false(execute_command("echo hello", NULL, sizeof(output)));
    assert_false(execute_command("echo hello", output, 0));
}

static void test_execute_command_success(void **state)
{
    (void)state;
    char output[256];
    assert_true(execute_command("echo hello", output, sizeof(output)));
    assert_non_null(strstr(output, "hello"));
}

/* ---- Tests for print_source_function edge cases ---- */

static void test_print_source_function_null_args(void **state)
{
    (void)state;
    /* Should not crash, just print error */
    print_source_function(NULL, "func", 10);
    print_source_function("/tmp/foo.c", NULL, 10);
    print_source_function("/tmp/foo.c", "func", 0);
    print_source_function("/tmp/foo.c", "func", -1);
}

/* ---- Tests for parse_ctags_output ---- */

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

static void test_parse_ctags_output_no_tab(void **state)
{
    (void)state;
    int start_line = 0, end_line = 0;
    char buf[256] = "no_tab_at_all";
    assert_false(parse_ctags_output("C", buf, &start_line, &end_line));
}

static void test_parse_ctags_output_one_tab(void **state)
{
    (void)state;
    int start_line = 0, end_line = 0;
    char buf[256] = "func\tfile_only";
    assert_false(parse_ctags_output("C", buf, &start_line, &end_line));
}

static void test_parse_ctags_output_no_line_field(void **state)
{
    (void)state;
    int start_line = 0, end_line = 0;
    char buf[256] = "func\tfile.c\t/^void func/;\"\tf";
    assert_false(parse_ctags_output("C", buf, &start_line, &end_line));
}

static void test_parse_ctags_output_bad_file(void **state)
{
    (void)state;
    int start_line = 0, end_line = 0;
    char buf[256] = "func\t/nonexistent_xyz.c\t/^void func/;\"\tf\tline:1";
    assert_false(parse_ctags_output("C", buf, &start_line, &end_line));
}

static void test_parse_ctags_output_unsupported_language(void **state)
{
    (void)state;
    /* Create a temp file to pass the fopen check */
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

static void test_parse_ctags_output_c_success(void **state)
{
    (void)state;
    /* Create a temp C file */
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
    /* Should succeed: start_line=4, end_line=7 */
    assert_true(result);
    assert_int_equal(start_line, 4);
    assert_true(end_line >= 6);

    unlink(tmp);
}

static void test_parse_ctags_output_fortran_success(void **state)
{
    (void)state;
    /* Create a temp Fortran file */
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

/* ---- Tests for print_source_function with real files ---- */

static void test_print_source_function_real_c_file(void **state)
{
    (void)state;
    /* Create a temp C file with a known function */
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

    /* This should exercise the full print_source_function path */
    print_source_function(tmp, "my_test_func", 3);

    unlink(tmp);
}

static void test_print_source_function_nonexistent_function(void **state)
{
    (void)state;
    /* Create a temp C file */
    char tmp[] = "/tmp/vgp_psf2_XXXXXX.c";
    int fd = mkstemps(tmp, 2);
    assert_true(fd >= 0);
    const char *src = "void real_func(void) { }\n";
    write(fd, src, strlen(src));
    close(fd);

    /* Function doesn't exist in the file — ctags grep will fail */
    print_source_function(tmp, "nonexistent_func", 1);

    unlink(tmp);
}

/* ---- Tests for execute_command edge cases ---- */

static void test_execute_command_empty_output(void **state)
{
    (void)state;
    char output[256] = "old";
    /* Command that produces no output */
    assert_true(execute_command("true", output, sizeof(output)));
}

/* ---- Tests for main.c flag coverage via system() ---- */

#ifndef VGP_EXE
#define VGP_EXE "build/vgp"
#endif
#ifndef INT_LOG_DIR
#define INT_LOG_DIR "build"
#endif

static void test_main_help_flag(void **state)
{
    (void)state;
    int rc = system(VGP_EXE " -h > /dev/null 2>&1");
    assert_int_equal(WEXITSTATUS(rc), 0);
}

static void test_main_unknown_flag(void **state)
{
    (void)state;
    int rc = system(VGP_EXE " -z dummy.log > /dev/null 2>&1");
    assert_int_equal(WEXITSTATUS(rc), 1);
}

static void test_main_no_log_file(void **state)
{
    (void)state;
    int rc = system(VGP_EXE " > /dev/null 2>&1");
    assert_int_equal(WEXITSTATUS(rc), 1);
}

static void test_main_multiple_log_files(void **state)
{
    (void)state;
    int rc = system(VGP_EXE " file1.log file2.log > /dev/null 2>&1");
    assert_int_equal(WEXITSTATUS(rc), 1);
}

static void test_main_nonexistent_file(void **state)
{
    (void)state;
    int rc = system(VGP_EXE " /tmp/nonexistent_vgp_xyz.log > /dev/null 2>&1");
    assert_int_equal(WEXITSTATUS(rc), 1);
}

static void test_main_stack_flag(void **state)
{
    (void)state;
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             VGP_EXE " -t " INT_LOG_DIR "/c_error_generator_int_app_c_valgrind.log > /dev/null 2>&1");
    int rc = system(cmd);
    assert_int_equal(WEXITSTATUS(rc), 0);
}

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

    /* Non-user code at context limit, no verbose — exercises false branch of line 584 */
    process_stack_trace_line("   at 0x123: malloc (in /usr/lib/libc.so.6)", &ps);
    /* stack_lines_shown should not increment since verbose is off */
}

/* ---- Tests for process_summary_lines verbose branches ---- */

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

/* ---- Tests for process_stack_trace_line ellipsis with verbose ---- */

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

    /* This line is not user code and we've reached the context limit, verbose prints ellipsis */
    process_stack_trace_line("   at 0x123: malloc (in /usr/lib/libc.so.6)", &ps);
    assert_int_equal(ps.stack_lines_shown, STACK_TRACE_CONTEXT_LINES + 1);

    app_config.verbose = false;
}

/* ---- Additional process_log_file tests for more coverage ---- */

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

static void test_process_log_file_many_stack_lines(void **state)
{
    (void)state;
    /* Build a log with more than STACK_TRACE_CONTEXT_LINES non-user stack entries */
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

static void test_process_log_file_with_real_source(void **state)
{
    (void)state;
    /* Create a temporary C source file with a known function */
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

    /* Build a valgrind-like log referencing that file */
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

static void test_process_log_file_many_stack_lines_no_verbose(void **state)
{
    (void)state;
    /* Many stack lines without verbose — exercises the non-verbose ellipsis branch */
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

/* ---- Additional extract_file_and_line edge cases ---- */

static void test_extract_file_and_line_scope_operator(void **state)
{
    (void)state;
    char filename[MAX_LINE_LENGTH];
    char function_name[MAX_LINE_LENGTH];
    int line_number = 0;

    /* Namespace::Class::method — should normalize to just "method" */
    assert_true(extract_file_and_line("   at 0x109A00: Namespace::Class::method (/path/to/source.cpp:157)",
                                      filename, function_name, &line_number));
    assert_string_equal(function_name, "method");
}

static void test_extract_file_and_line_dotted_suffix(void **state)
{
    (void)state;
    char filename[MAX_LINE_LENGTH];
    char function_name[MAX_LINE_LENGTH];
    int line_number = 0;

    /* Fortran-style dotted suffix: trigger_invalid_write.3 */
    assert_true(extract_file_and_line("   at 0x109A00: trigger_invalid_write.3 (/path/to/source.f90:35)",
                                      filename, function_name, &line_number));
    assert_string_equal(function_name, "trigger_invalid_write");
}

/* ---- Test for is_valid_function_char ---- */
/* NOTE: is_valid_function_char is declared in vgp.h but not defined in vgp.c,
   so it cannot be tested here. */

/* Shorthand for tests that produce noisy stdout/stderr */
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
