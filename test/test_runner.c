/*
 * test_runner.c
 *
 * Top-level driver for the cmocka unit + integration test suites.
 * This file is responsible for two things:
 *
 *   1. Invoking the per-function unit-test groups defined under
 *      test/unit/ (each of which annotates its tests with the
 *      LLR identifiers from doc/LLRs.md and the HLR identifiers from
 *      doc/HLRs.md).
 *
 *   2. Running a small set of black-box "integration" tests that
 *      cross-check the actual on-disk vgp output for each of the
 *      per-language error generators in test/integration/. These
 *      tests verify the externally-observable behaviour of the
 *      shipped vgp binary end-to-end.
 *
 * Each integration test is annotated below with the HLR(s) it
 * exercises so a future traceability matrix can be built.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Unit test group runners from individual test files */
extern int run_extract_file_and_line_tests(void);
extern int run_is_user_code_stack_trace_tests(void);
extern int run_strip_valgrind_pid_prefix_tests(void);
extern int run_vgp_core_tests(void);

/* Build directory path (set via -DBUILD_DIR at compile time) */
#ifndef BUILD_DIR
#define BUILD_DIR "build"
#endif

/* Integration test app basenames and their expected properties */
typedef struct {
    const char *app_basename;
    int min_expected_error_count;
    int max_expected_error_count;
    int expected_leak_count;
    bool has_leak_summary;
    bool check_vgp_no_valgrind_errors;
} IntegrationTestCase;

static const IntegrationTestCase integration_cases[] = {
    { "c_error_generator_int_app_c",       6, 6, 3, true,  true  },
    { "cpp_error_generator_int_app_cpp",    4, 4, 1, true,  true  },
    { "fortran_error_generator_int_app_f90",3, 3, 4, false, true  },
    { "rust_error_generator_int_app_rs",    3, 4, 1, true,  false },
    { "ada_error_generator_int_app_ada",    0, 0, 2, true,  true  },
};

#define NUM_INTEGRATION_CASES (sizeof(integration_cases) / sizeof(integration_cases[0]))

/* ---- Helper functions ---- */

/* Read an entire file into a malloc'd buffer. Returns NULL on failure.
 * Caller must free the returned buffer. (No requirement coverage; this
 * is test-harness plumbing.) */
static char *read_file_contents(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    if (len < 0) {
        fclose(f);
        return NULL;
    }
    rewind(f);

    char *buf = malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t read_len = fread(buf, 1, (size_t)len, f);
    buf[read_len] = '\0';
    fclose(f);
    return buf;
}

/* Search for a line matching the prefix in the file contents and
 * extract the integer value after it. Returns -1 if not found.
 * Test-harness plumbing only. */
static int extract_int_after_prefix(const char *contents, const char *prefix)
{
    const char *p = strstr(contents, prefix);
    if (!p)
        return -1;
    p += strlen(prefix);
    return atoi(p);
}

/* Check that a Valgrind log captured while running vgp itself contains
 * "ERROR SUMMARY: 0 errors". Returns true if 0 errors found. */
static bool vgp_itself_has_no_valgrind_errors(const char *contents)
{
    const char *p = strstr(contents, "ERROR SUMMARY:");
    if (!p)
        return false;
    p += strlen("ERROR SUMMARY:");
    while (*p == ' ')
        p++;
    return (*p == '0');
}

/* ---- Integration test functions ---- */

/*
 * Verifies that every per-language vgp output log opens with the
 * "Parsing Valgrind Log File:" banner emitted by main().
 * Traces to LLR-MAIN-06 and HLR-007 (Initial User Feedback).
 */
static void test_vgp_output_has_header(void **state)
{
    (void)state;
    for (size_t i = 0; i < NUM_INTEGRATION_CASES; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s_vgp_output.log",
                 BUILD_DIR, integration_cases[i].app_basename);

        char *contents = read_file_contents(path);
        if (!contents)
            fail_msg("Failed to read file: %s", path);
        if (!strstr(contents, "Parsing Valgrind Log File:"))
            fail_msg("Missing 'Parsing Valgrind Log File:' header in %s", path);
        free(contents);
    }
}

/*
 * Verifies that the "* Total Errors:" value reported by vgp matches the
 * expected per-language range, confirming that the streaming parser
 * recognises the same number of error blocks as the test corpus
 * defines.
 * Traces to LLR-PFES-03, LLR-PEH-03, LLR-CSNE-04 and
 * HLR-016 (Error Block Detection),
 * HLR-018 (Error Header Rendering),
 * HLR-034 (Final Error Summary Output).
 */
static void test_vgp_output_error_counts(void **state)
{
    (void)state;
    for (size_t i = 0; i < NUM_INTEGRATION_CASES; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s_vgp_output.log",
                 BUILD_DIR, integration_cases[i].app_basename);

        char *contents = read_file_contents(path);
        if (!contents)
            fail_msg("Failed to read file: %s", path);

        int total_errors = extract_int_after_prefix(contents, "* Total Errors: ");
        if (total_errors < integration_cases[i].min_expected_error_count ||
            total_errors > integration_cases[i].max_expected_error_count)
            fail_msg("%s: expected Total Errors %d..%d, got %d",
                     integration_cases[i].app_basename,
                     integration_cases[i].min_expected_error_count,
                     integration_cases[i].max_expected_error_count, total_errors);

        free(contents);
    }
}

/*
 * Verifies the "* Possible Leaks:" derived count, which exercises the
 * subtraction of vgp's classified errors from Valgrind's reported
 * total.
 * Traces to LLR-PFES-03 and HLR-034 (Final Error Summary Output).
 */
static void test_vgp_output_leak_counts(void **state)
{
    (void)state;
    for (size_t i = 0; i < NUM_INTEGRATION_CASES; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s_vgp_output.log",
                 BUILD_DIR, integration_cases[i].app_basename);

        char *contents = read_file_contents(path);
        if (!contents)
            fail_msg("Failed to read file: %s", path);

        int possible_leaks = extract_int_after_prefix(contents, "* Possible Leaks: ");
        if (possible_leaks != integration_cases[i].expected_leak_count)
            fail_msg("%s: expected Possible Leaks %d, got %d",
                     integration_cases[i].app_basename,
                     integration_cases[i].expected_leak_count, possible_leaks);

        free(contents);
    }
}

/*
 * Verifies that languages whose Valgrind run produces a leak summary
 * also produce both the "--- LEAK SUMMARY ---" and
 * "--- FINAL COUNTS ---" markers in the vgp output.
 * Traces to LLR-PSL-03, LLR-PFES-03 and
 * HLR-031 (Leak Summary Recognition),
 * HLR-033 (Leak Detail Formatting),
 * HLR-034 (Final Error Summary Output).
 */
static void test_vgp_output_has_leak_summary(void **state)
{
    (void)state;
    for (size_t i = 0; i < NUM_INTEGRATION_CASES; i++) {
        if (!integration_cases[i].has_leak_summary)
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s_vgp_output.log",
                 BUILD_DIR, integration_cases[i].app_basename);

        char *contents = read_file_contents(path);
        if (!contents)
            fail_msg("Failed to read file: %s", path);
        if (!strstr(contents, "--- LEAK SUMMARY ---"))
            fail_msg("Missing '--- LEAK SUMMARY ---' in %s", path);
        if (!strstr(contents, "--- FINAL COUNTS ---"))
            fail_msg("Missing '--- FINAL COUNTS ---' in %s", path);
        free(contents);
    }
}

/*
 * Verifies the "[ERROR #N]" header format and that the count of headers
 * matches the per-language expected error count.
 * Traces to LLR-PEH-03, LLR-PEH-04 and
 * HLR-016 (Error Block Detection),
 * HLR-018 (Error Header Rendering).
 */
static void test_vgp_output_has_expected_error_blocks(void **state)
{
    (void)state;
    for (size_t i = 0; i < NUM_INTEGRATION_CASES; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s_vgp_output.log",
                 BUILD_DIR, integration_cases[i].app_basename);

        char *contents = read_file_contents(path);
        if (!contents)
            fail_msg("Failed to read file: %s", path);

        int count = 0;
        const char *p = contents;
        while ((p = strstr(p, "[ERROR #")) != NULL) {
            count++;
            p++;
        }
        if (count < integration_cases[i].min_expected_error_count ||
            count > integration_cases[i].max_expected_error_count)
            fail_msg("%s: expected %d..%d [ERROR #] blocks, got %d",
                     integration_cases[i].app_basename,
                     integration_cases[i].min_expected_error_count,
                     integration_cases[i].max_expected_error_count, count);

        free(contents);
    }
}

/*
 * Verifies that vgp itself, when run under Valgrind to parse each
 * per-language log, produces zero Valgrind-detected errors. This is the
 * dogfood test that backs the project's "no memory bugs" promise.
 * Traces to HLR-008 (Resource Management),
 * HLR-036 (Streaming Single-Pass Operation),
 * HLR-037 (Bounded Line Buffer),
 * HLR-041 (Standard-C Implementation).
 */
static void test_vgp_itself_no_valgrind_errors(void **state)
{
    (void)state;
    for (size_t i = 0; i < NUM_INTEGRATION_CASES; i++) {
        if (!integration_cases[i].check_vgp_no_valgrind_errors)
            continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s_vgp_itself_valgrind.log",
                 BUILD_DIR, integration_cases[i].app_basename);

        char *contents = read_file_contents(path);
        if (!contents)
            fail_msg("Failed to read file: %s", path);
        if (!vgp_itself_has_no_valgrind_errors(contents))
            fail_msg("vgp itself had Valgrind errors when processing %s",
                     integration_cases[i].app_basename);
        free(contents);
    }
}

int main(void)
{
    int failures = 0;
    int group_result;

    printf("==========================================================\n");
    printf("  VGP Test Runner - Unit & Integration Tests\n");
    printf("==========================================================\n\n");

    /* Run unit tests */
    printf("--- Unit: extract_file_and_line ---\n");
    group_result = run_extract_file_and_line_tests();
    failures += group_result;

    printf("\n--- Unit: is_user_code_stack_trace ---\n");
    group_result = run_is_user_code_stack_trace_tests();
    failures += group_result;

    printf("\n--- Unit: strip_valgrind_pid_prefix ---\n");
    group_result = run_strip_valgrind_pid_prefix_tests();
    failures += group_result;

    printf("\n--- Unit: vgp_core ---\n");
    group_result = run_vgp_core_tests();
    failures += group_result;

    /* Run integration tests */
    printf("\n--- Integration: vgp output verification ---\n");
    const struct CMUnitTest integration_tests[] = {
        cmocka_unit_test(test_vgp_output_has_header),
        cmocka_unit_test(test_vgp_output_error_counts),
        cmocka_unit_test(test_vgp_output_leak_counts),
        cmocka_unit_test(test_vgp_output_has_leak_summary),
        cmocka_unit_test(test_vgp_output_has_expected_error_blocks),
        cmocka_unit_test(test_vgp_itself_no_valgrind_errors),
    };

    failures += cmocka_run_group_tests(integration_tests, NULL, NULL);

    /* Final summary */
    printf("\n==========================================================\n");
    if (failures == 0)
        printf("  RESULT: ALL TESTS PASSED\n");
    else
        printf("  RESULT: %d TEST GROUP(S) FAILED\n", failures);
    printf("==========================================================\n");

    return failures;
}
