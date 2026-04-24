# Software Test Plan: Valgrind Parser (vgp)

**Version:** 1.0
**Date:** 4/21/2026
**Author(s):** John Anderson

## 1. Introduction

### 1.1 Purpose
This document captures the verification plan for the `vgp`
(Valgrind Parser) command-line application. It enumerates every
automated test in [test/](../test/) and maps each test to the
low-level requirement(s) (LLRs) it verifies in
[LLRs.md](LLRs.md).

### 1.2 Scope
The plan covers the cmocka-based unit tests under
[test/unit/](../test/unit/) and the black-box integration tests
driven by [test/test_runner.c](../test/test_runner.c) against
Valgrind logs produced from the per-language error generators in
[test/integration/](../test/integration/).

Static analysis (cppcheck), coverage measurement (`gcov`), and
documentation-only requirements are out of scope; their
verification is described in the project [Makefile](../Makefile)
and tracked in [Traceability.md §5](Traceability.md#5-coverage-gaps).

### 1.3 Related Documents
*   [SDD.md](SDD.md) — design
*   [HLRs.md](HLRs.md) — high-level requirements
*   [LLRs.md](LLRs.md) — low-level requirements

## 2. Test Strategy

### 2.1 Test Levels

| Level | Source | Driver | Style |
| ----- | ------ | ------ | ----- |
| Unit | [test/unit/](../test/unit/) | cmocka group runners | White-box, per-function |
| Integration | [test/test_runner.c](../test/test_runner.c) | cmocka + `system()` + on-disk Valgrind logs | Black-box, end-to-end |

### 2.2 Test Framework
All tests use the [cmocka](https://cmocka.org/) C unit-test
framework. Each unit-test source file exposes a
`run_<module>_tests()` entry point that registers and runs its
tests via `cmocka_run_group_tests`. The top-level
[test/test_runner.c](../test/test_runner.c) `main()` invokes
each group runner in turn and then runs the integration test
group.

### 2.3 Build and Execution
All tests are built and executed by `make test`. The target:

1. Rebuilds the `vgp` binary with coverage flags
   (`-fprofile-arcs -ftest-coverage`).
2. Builds each integration error generator in
   [test/integration/](../test/integration/).
3. Runs each generator under Valgrind to produce a
   `build/*_valgrind.log` fixture.
4. Runs `vgp` on each fixture to produce a
   `build/*_vgp_output.log` fixture.
5. Runs `vgp` itself under Valgrind on each fixture to produce a
   `build/*_vgp_itself_valgrind.log` fixture.
6. Builds and executes the cmocka test runner, which calls each
   unit-test group and then the integration test group.

Coverage is subsequently consumed by `make coverage` (gcov +
lcov).

### 2.4 Pass/Fail Criteria
*   Every cmocka assertion in every test must pass.
*   The integration test
    `test_vgp_itself_no_valgrind_errors` must report
    `ERROR SUMMARY: 0 errors` for `vgp` itself on every
    per-language input.
*   Compilation must complete with no warnings under the
    project's `-Wall -fanalyzer …` flag set.

### 2.5 Traceability Convention
Every test function carries a doc-comment block that names the
LLR(s) it verifies and/or the HLR(s) those LLRs satisfy. This
plan is generated from those comment blocks together with the
`*Trace:*` lines in [LLRs.md](LLRs.md).

A small number of tests annotate only HLRs, because the
scenario they exercise is a combinatorial interaction rather
than a single per-function contract. Those tests appear in the
catalogue with `—` in the *Verifies* column and the HLRs they
cover listed in their purpose text.

## 3. Test Catalogue

Snapshot: **120 test(s)** across
**6 file(s)**.

### 3.1. [test/test_runner.c](../test/test_runner.c)

Role: **runner+integration**. **6 test(s).**

| # | Test | Verifies | Purpose |
| - | ---- | -------- | ------- |
| 1 | <a id="test_vgp_output_has_header"></a>`test_vgp_output_has_header` | `LLR-MAIN-06` | Verifies that every per-language vgp output log opens with the "Parsing Valgrind Log File:" banner emitted by main(). Traces to LLR-MAIN-06 and HLR-007 (Initial User Feedback). |
| 2 | <a id="test_vgp_output_error_counts"></a>`test_vgp_output_error_counts` | `LLR-CSNE-04`, `LLR-PEH-03`, `LLR-PFES-03` | Verifies that the "* Total Errors:" value reported by vgp matches the expected per-language range, confirming that the streaming parser recognises the same number of error blocks as the test corpus defines. Traces to LLR-PFES-03, LLR-PEH-03, LLR-CSNE-04 and HLR-016 (Error Block Detection), HLR-018 (Error Header Rendering), HLR-034 (Final Error Summary Output). |
| 3 | <a id="test_vgp_output_leak_counts"></a>`test_vgp_output_leak_counts` | `LLR-PFES-03` | Verifies the "* Possible Leaks:" derived count, which exercises the subtraction of vgp's classified errors from Valgrind's reported total. Traces to LLR-PFES-03 and HLR-034 (Final Error Summary Output). |
| 4 | <a id="test_vgp_output_has_leak_summary"></a>`test_vgp_output_has_leak_summary` | `LLR-PFES-03`, `LLR-PSL-03` | Verifies that languages whose Valgrind run produces a leak summary also produce both the "--- LEAK SUMMARY ---" and "--- FINAL COUNTS ---" markers in the vgp output. Traces to LLR-PSL-03, LLR-PFES-03 and HLR-031 (Leak Summary Recognition), HLR-033 (Leak Detail Formatting), HLR-034 (Final Error Summary Output). |
| 5 | <a id="test_vgp_output_has_expected_error_blocks"></a>`test_vgp_output_has_expected_error_blocks` | `LLR-PEH-03`, `LLR-PEH-04` | Verifies the "[ERROR #N]" header format and that the count of headers matches the per-language expected error count. Traces to LLR-PEH-03, LLR-PEH-04 and HLR-016 (Error Block Detection), HLR-018 (Error Header Rendering). |
| 6 | <a id="test_vgp_itself_no_valgrind_errors"></a>`test_vgp_itself_no_valgrind_errors` | — | Verifies that vgp itself, when run under Valgrind to parse each per-language log, produces zero Valgrind-detected errors. This is the dogfood test that backs the project's "no memory bugs" promise. Traces to HLR-008 (Resource Management), HLR-036 (Streaming Single-Pass Operation), HLR-037 (Bounded Line Buffer), HLR-041 (Standard-C Implementation). |

### 3.2. [test/unit/extract_file_and_line.c](../test/unit/extract_file_and_line.c)

Role: **unit**. **3 test(s).**

| # | Test | Verifies | Purpose |
| - | ---- | -------- | ------- |
| 1 | <a id="test_extract_file_and_line_rejects_null_args"></a>`test_extract_file_and_line_rejects_null_args` | `LLR-EFAL-02` | Verifies LLR-EFAL-02 (NULL-pointer guards): the function returns false when any of its four output/input pointers is NULL. Traces to HLR-039 (Defensive Argument Validation). |
| 2 | <a id="test_extract_file_and_line_parses_stack_entry_with_line"></a>`test_extract_file_and_line_parses_stack_entry_with_line` | `LLR-EFAL-03`, `LLR-EFAL-04` | Verifies LLR-EFAL-03, LLR-EFAL-04 (primary three-field sscanf path) on a Valgrind frame of the form "  at 0x...: function (file:line)". Also confirms namespace stripping done by normalize_function_name(). Traces to HLR-021 (Stack Frame Decoding) and HLR-025 (First-User-Frame Capture). |
| 3 | <a id="test_extract_file_and_line_parses_entry_without_line_number"></a>`test_extract_file_and_line_parses_entry_without_line_number` | `LLR-EFAL-06` | Verifies LLR-EFAL-06 (third "(in <obj>)" sscanf path): a frame with no source line number falls through to the "in <obj>" pattern, sets *line_number to 0, and still returns the function name and object path. Traces to HLR-021 (Stack Frame Decoding) and HLR-040 (Non-Fatal Recovery from Malformed Input). |

### 3.3. [test/unit/is_user_code_stack_trace.c](../test/unit/is_user_code_stack_trace.c)

Role: **unit**. **14 test(s).**

| # | Test | Verifies | Purpose |
| - | ---- | -------- | ------- |
| 1 | <a id="test_is_user_code_no_prefix"></a>`test_is_user_code_no_prefix` | `LLR-IUCST-03` | Verifies LLR-IUCST-03: lines lacking the "  at " / "  by " prefix are rejected outright (HLR-020 Stack Frame Recognition, HLR-022 User-Code Classification). |
| 2 | <a id="test_is_user_code_at_prefix_no_extension"></a>`test_is_user_code_at_prefix_no_extension` | `LLR-IUCST-03`, `LLR-IUCST-04` | Verifies LLR-IUCST-03 + LLR-IUCST-04: a "  at " frame whose path lacks a recognised user-code extension is not user code (HLR-022 User-Code Classification). |
| 3 | <a id="test_is_user_code_by_prefix_no_extension"></a>`test_is_user_code_by_prefix_no_extension` | `LLR-IUCST-03`, `LLR-IUCST-04` | Verifies LLR-IUCST-03 + LLR-IUCST-04 for the "  by " prefix (HLR-022 User-Code Classification). |
| 4 | <a id="test_is_user_code_at_prefix_with_c_extension_no_ignore"></a>`test_is_user_code_at_prefix_with_c_extension_no_ignore` | `LLR-IUCST-03`, `LLR-IUCST-04`, `LLR-IUCST-05`, `LLR-IUCST-06` | Verifies LLR-IUCST-03 + LLR-IUCST-04 + LLR-IUCST-05 + LLR-IUCST-06: a "  at " frame with a .c extension and no IGNORE_PATHS hit returns true (HLR-022 User-Code Classification, HLR-029 Multi-Language Source Support). |
| 5 | <a id="test_is_user_code_by_prefix_with_cpp_extension_no_ignore"></a>`test_is_user_code_by_prefix_with_cpp_extension_no_ignore` | `LLR-IUCST-03`, `LLR-IUCST-04`, `LLR-IUCST-05`, `LLR-IUCST-06` | Verifies LLR-IUCST-03 + LLR-IUCST-04 + LLR-IUCST-05 + LLR-IUCST-06 for the "  by " prefix and a .cpp extension (HLR-022, HLR-029). |
| 6 | <a id="test_is_user_code_at_prefix_with_h_extension_no_ignore"></a>`test_is_user_code_at_prefix_with_h_extension_no_ignore` | `LLR-IUCST-04`, `LLR-IUCST-06` | Verifies LLR-IUCST-04 + LLR-IUCST-06 for a .h extension (HLR-022 User-Code Classification, HLR-029 Multi-Language Source Support). |
| 7 | <a id="test_is_user_code_by_prefix_with_hpp_extension_no_ignore"></a>`test_is_user_code_by_prefix_with_hpp_extension_no_ignore` | `LLR-IUCST-04`, `LLR-IUCST-06` | Verifies LLR-IUCST-04 + LLR-IUCST-06 for a .hpp extension (HLR-022 User-Code Classification, HLR-029 Multi-Language Source Support). |
| 8 | <a id="test_is_user_code_at_prefix_with_c_extension_with_usr_include_ignore"></a>`test_is_user_code_at_prefix_with_c_extension_with_usr_include_ignore` | `LLR-GBL-03`, `LLR-IUCST-05` | Verifies LLR-IUCST-05 (IGNORE_PATHS deny-list): even when the extension matches USER_CODE_EXTENSIONS, a path beginning with /usr/ is rejected (HLR-022 User-Code Classification, LLR-GBL-03). |
| 9 | <a id="test_is_user_code_by_prefix_with_cpp_extension_with_lib_ignore"></a>`test_is_user_code_by_prefix_with_cpp_extension_with_lib_ignore` | `LLR-GBL-03`, `LLR-IUCST-05` | Verifies LLR-IUCST-05: a path containing /lib/ is rejected even when the extension matches (HLR-022 User-Code Classification, LLR-GBL-03). |
| 10 | <a id="test_is_user_code_at_prefix_with_c_extension_with_question_marks_ignore"></a>`test_is_user_code_at_prefix_with_c_extension_with_question_marks_ignore` | `LLR-IUCST-04` | Verifies LLR-IUCST-04: a frame whose path tokens do not contain any recognised user-code extension (here "???:0") is rejected (HLR-022 User-Code Classification). |
| 11 | <a id="test_is_user_code_at_prefix_no_user_extension"></a>`test_is_user_code_at_prefix_no_user_extension` | `LLR-IUCST-04` | Verifies LLR-IUCST-04: a non-source-file extension (.txt) is not in USER_CODE_EXTENSIONS and the frame is rejected (HLR-022 User-Code Classification). |
| 12 | <a id="test_is_user_code_null_input"></a>`test_is_user_code_null_input` | `LLR-IUCST-02` | Verifies LLR-IUCST-02 (NULL-pointer guard) and HLR-039 (Defensive Argument Validation). |
| 13 | <a id="test_is_user_code_just_prefix"></a>`test_is_user_code_just_prefix` | `LLR-IUCST-04` | Verifies LLR-IUCST-04: a bare "  at " / "  by " line with no path after it cannot match any USER_CODE_EXTENSIONS entry (HLR-022 User-Code Classification). |
| 14 | <a id="test_is_user_code_mixed_case_extension"></a>`test_is_user_code_mixed_case_extension` | `LLR-GBL-02`, `LLR-IUCST-04` | Verifies LLR-IUCST-04 + LLR-GBL-02: uppercase ".C" appears in USER_CODE_EXTENSIONS and is recognised as user code (HLR-022 User-Code Classification, HLR-029 Multi-Language Source Support). |

### 3.4. [test/unit/strip_valgrind_pid_prefix.c](../test/unit/strip_valgrind_pid_prefix.c)

Role: **unit**. **18 test(s).**

| # | Test | Verifies | Purpose |
| - | ---- | -------- | ------- |
| 1 | <a id="test_strip_no_prefix_at_all"></a>`test_strip_no_prefix_at_all` | `LLR-SVPP-03` | Verifies LLR-SVPP-03: when no "==" substring exists the original line pointer is returned unchanged (HLR-014 PID-Prefix Stripping, HLR-040 Non-Fatal Recovery). |
| 2 | <a id="test_strip_null_input"></a>`test_strip_null_input` | `LLR-SVPP-02` | Verifies LLR-SVPP-02: NULL input returns NULL without dereferencing (HLR-039 Defensive Argument Validation). |
| 3 | <a id="test_strip_empty_string"></a>`test_strip_empty_string` | `LLR-SVPP-03` | Verifies LLR-SVPP-03: empty input has no "==" prefix and is returned unchanged (HLR-014, HLR-040). |
| 4 | <a id="test_strip_only_double_equals"></a>`test_strip_only_double_equals` | `LLR-SVPP-04` | Verifies LLR-SVPP-04: input is bare "==" with no following digits, so the original pointer is returned (HLR-014, HLR-040). |
| 5 | <a id="test_strip_double_equals_no_pid_digits"></a>`test_strip_double_equals_no_pid_digits` | `LLR-SVPP-04` | Verifies LLR-SVPP-04: "==" present but no digit follows the optional whitespace, so input is returned unchanged (HLR-014 PID-Prefix Stripping, HLR-040 Non-Fatal Recovery). |
| 6 | <a id="test_strip_pid_no_trailing_marker"></a>`test_strip_pid_no_trailing_marker` | `LLR-SVPP-05` | Verifies LLR-SVPP-05: "==<digits>" present but no trailing "== " sequence, so the original line is returned (HLR-014, HLR-040). |
| 7 | <a id="test_strip_pid_trailing_double_equals_no_space"></a>`test_strip_pid_trailing_double_equals_no_space` | `LLR-SVPP-05` | Verifies LLR-SVPP-05: trailing "==" is missing the required following space, so the prefix does not match (HLR-014, HLR-040). |
| 8 | <a id="test_strip_valid_prefix_simple"></a>`test_strip_valid_prefix_simple` | `LLR-SVPP-06` | Verifies LLR-SVPP-06: full "==<pid>== " prefix is stripped and a pointer to the message body is returned (HLR-014 PID-Prefix Stripping). |
| 9 | <a id="test_strip_valid_prefix_spaces_around_pid"></a>`test_strip_valid_prefix_spaces_around_pid` | `LLR-SVPP-04`, `LLR-SVPP-05`, `LLR-SVPP-06` | Verifies LLR-SVPP-04 + LLR-SVPP-05 (whitespace tolerance around the PID) and LLR-SVPP-06 (HLR-014 PID-Prefix Stripping). |
| 10 | <a id="test_strip_valid_prefix_no_message_after"></a>`test_strip_valid_prefix_no_message_after` | `LLR-SVPP-06` | Verifies LLR-SVPP-06: prefix is stripped even when the message body is empty (HLR-014 PID-Prefix Stripping). |
| 11 | <a id="test_strip_valid_prefix_incomplete_end_marker"></a>`test_strip_valid_prefix_incomplete_end_marker` | `LLR-SVPP-05` | Verifies LLR-SVPP-05: trailing "==" with no following space and no body is rejected (HLR-014, HLR-040). |
| 12 | <a id="test_strip_prefix_like_too_short_for_pid"></a>`test_strip_prefix_like_too_short_for_pid` | `LLR-SVPP-04` | Verifies LLR-SVPP-04: leading "==" followed by whitespace and then "==" is not a PID prefix because the inner field is not digits (HLR-014, HLR-040). |
| 13 | <a id="test_strip_prefix_like_too_short_for_end_marker_full"></a>`test_strip_prefix_like_too_short_for_end_marker_full` | `LLR-SVPP-05` | Verifies LLR-SVPP-05: variants where the trailing "== " marker is absent or incomplete cause the original line to be returned (HLR-014, HLR-040). |
| 14 | <a id="test_strip_valid_prefix_short_end_marker"></a>`test_strip_valid_prefix_short_end_marker` | `LLR-SVPP-06` | Verifies LLR-SVPP-06: a single-character message body is correctly exposed after prefix stripping (HLR-014 PID-Prefix Stripping). |
| 15 | <a id="test_strip_line_is_exact_prefix"></a>`test_strip_line_is_exact_prefix` | `LLR-SVPP-06` | Verifies LLR-SVPP-06: a line consisting solely of a valid prefix yields an empty body (HLR-014 PID-Prefix Stripping). |
| 16 | <a id="test_strip_multiple_potential_prefixes"></a>`test_strip_multiple_potential_prefixes` | `LLR-SVPP-06` | Verifies LLR-SVPP-06: only the first valid prefix is stripped; any subsequent "==N==" patterns inside the body are preserved verbatim (HLR-014 PID-Prefix Stripping). |
| 17 | <a id="test_strip_valid_prefix_long_pid"></a>`test_strip_valid_prefix_long_pid` | `LLR-SVPP-04` | Verifies LLR-SVPP-04: arbitrarily long PID digit runs are accepted (HLR-014 PID-Prefix Stripping). |
| 18 | <a id="test_strip_leading_spaces_before_prefix"></a>`test_strip_leading_spaces_before_prefix` | `LLR-SVPP-03`, `LLR-SVPP-06` | Verifies LLR-SVPP-03 + LLR-SVPP-06: leading whitespace before the "==" is tolerated because strstr() finds the first "==" regardless (HLR-014 PID-Prefix Stripping). |

### 3.5. [test/unit/vgp_core.c](../test/unit/vgp_core.c)

Role: **unit**. **79 test(s).**

| # | Test | Verifies | Purpose |
| - | ---- | -------- | ------- |
| 1 | <a id="test_initialize_parse_state_normal"></a>`test_initialize_parse_state_normal` | `LLR-IPS-03` | Verifies LLR-IPS-03: every ParseState field is reset to its documented initial value, even when the structure already contains stale data. Traces to HLR-038 (Stateful Parsing Context). |
| 2 | <a id="test_initialize_parse_state_null"></a>`test_initialize_parse_state_null` | `LLR-IPS-02` | Verifies LLR-IPS-02: a NULL ParseState pointer is a no-op. Traces to HLR-039 (Defensive Argument Validation). |
| 3 | <a id="test_check_start_new_error_null_args"></a>`test_check_start_new_error_null_args` | `LLR-CSNE-02` | Verifies LLR-CSNE-02: NULL line_content or state pointers cause the function to return false without side effects. Traces to HLR-039 (Defensive Argument Validation). |
| 4 | <a id="test_check_start_new_error_detects_invalid_read"></a>`test_check_start_new_error_detects_invalid_read` | `LLR-CSNE-04`, `LLR-GBL-04` | Verifies LLR-CSNE-04 + LLR-GBL-04 for the "Invalid read" keyword: a matching line starts a new error block and bumps error_count. Traces to HLR-016 (Error Block Detection), HLR-018 (Error Header Rendering), HLR-038 (Stateful Parsing Context). |
| 5 | <a id="test_check_start_new_error_detects_invalid_write"></a>`test_check_start_new_error_detects_invalid_write` | `LLR-CSNE-04`, `LLR-GBL-04` | Verifies LLR-CSNE-04 + LLR-GBL-04 for the "Invalid write" keyword. Traces to HLR-016 (Error Block Detection). |
| 6 | <a id="test_check_start_new_error_detects_uninit"></a>`test_check_start_new_error_detects_uninit` | `LLR-CSNE-04`, `LLR-GBL-04` | Verifies LLR-CSNE-04 + LLR-GBL-04 for the "depends on uninitialised value" keyword. Traces to HLR-016 (Error Block Detection). |
| 7 | <a id="test_check_start_new_error_detects_invalid_free"></a>`test_check_start_new_error_detects_invalid_free` | `LLR-CSNE-04`, `LLR-GBL-04` | Verifies LLR-CSNE-04 + LLR-GBL-04 for the "Invalid free" keyword. Traces to HLR-016 (Error Block Detection). |
| 8 | <a id="test_check_start_new_error_detects_mismatched_free"></a>`test_check_start_new_error_detects_mismatched_free` | `LLR-CSNE-04`, `LLR-GBL-04` | Verifies LLR-CSNE-04 + LLR-GBL-04 for the "Mismatched free" keyword. Traces to HLR-016 (Error Block Detection). |
| 9 | <a id="test_check_start_new_error_detects_overlap"></a>`test_check_start_new_error_detects_overlap` | `LLR-CSNE-04`, `LLR-GBL-04` | Verifies LLR-CSNE-04 + LLR-GBL-04 for the "Source and destination overlap" keyword. Traces to HLR-016 (Error Block Detection). |
| 10 | <a id="test_check_start_new_error_no_match"></a>`test_check_start_new_error_no_match` | `LLR-CSNE-05` | Verifies LLR-CSNE-05: a non-error keyword line does not start an error block (HLR-016 Error Block Detection). |
| 11 | <a id="test_check_start_new_error_already_in_block"></a>`test_check_start_new_error_already_in_block` | `LLR-CSNE-03` | Verifies LLR-CSNE-03: while the parser is already inside an error block, no new block is started even if the line matches a keyword (HLR-016 Error Block Detection, HLR-038 Stateful Parsing Context). |
| 12 | <a id="test_process_stack_trace_line_null_args"></a>`test_process_stack_trace_line_null_args` | `LLR-PSTL-02` | Verifies LLR-PSTL-02: NULL line_content or state cause the function to return without side effects. Traces to HLR-039 (Defensive Argument Validation). |
| 13 | <a id="test_process_stack_trace_line_finds_user_code"></a>`test_process_stack_trace_line_finds_user_code` | `LLR-PSTL-05` | Verifies LLR-PSTL-05: the first user-code frame in the block is captured into ParseState and arms print_function for the eventual source listing. Traces to HLR-022 (User-Code Classification), HLR-025 (First-User-Frame Capture). |
| 14 | <a id="test_process_stack_trace_line_non_user_code"></a>`test_process_stack_trace_line_non_user_code` | `LLR-PSTL-05` | Verifies LLR-PSTL-05 negative path: a non-user-code frame does not arm user_code_found_for_error. Traces to HLR-022 (User-Code Classification). |
| 15 | <a id="test_process_stack_trace_line_with_print_stack"></a>`test_process_stack_trace_line_with_print_stack` | `LLR-PSTL-04` | Verifies LLR-PSTL-04: when print_stack is enabled, every printed frame increments stack_lines_shown. Traces to HLR-024 (Stack Trace Output Gated by Options), HLR-038 (Stateful Parsing Context). |
| 16 | <a id="test_process_stack_trace_line_ellipsis_branch"></a>`test_process_stack_trace_line_ellipsis_branch` | `LLR-PSTL-06` | Verifies LLR-PSTL-06: at the STACK_TRACE_CONTEXT_LINES boundary, a non-user frame triggers the one-shot ellipsis and increments stack_lines_shown to suppress further ellipses. Traces to HLR-023 (Bounded Stack Trace Output), HLR-024 (Stack Trace Output Gated by Options). |
| 17 | <a id="test_finalize_error_block_null"></a>`test_finalize_error_block_null` | `LLR-FEB-02` | Verifies LLR-FEB-02: NULL state is a no-op. Traces to HLR-039 (Defensive Argument Validation). |
| 18 | <a id="test_finalize_error_block_no_user_code"></a>`test_finalize_error_block_no_user_code` | `LLR-FEB-03`, `LLR-FEB-06` | Verifies LLR-FEB-03 and LLR-FEB-06: when no user-code frame was found but stack_lines_shown > 0, the diagnostic hint is emitted and the in_error_block / print_function flags are reset. Traces to HLR-017 (Error Block Termination), HLR-022 (User-Code Classification), HLR-025 (First-User-Frame Capture). |
| 19 | <a id="test_finalize_error_block_with_print_function"></a>`test_finalize_error_block_with_print_function` | `LLR-FEB-04`, `LLR-FEB-05`, `LLR-FEB-06` | Verifies LLR-FEB-04 + LLR-FEB-06 in isolation from LLR-FEB-05: with print_function set but print_source / verbose off, only the "Source (file:line)" reference line is emitted. Traces to HLR-026 (Conditional Source Reference Output), HLR-028 (Editor-Jumpable References), HLR-017 (Error Block Termination). |
| 20 | <a id="test_finalize_error_block_with_verbose"></a>`test_finalize_error_block_with_verbose` | `LLR-FEB-05` | Verifies LLR-FEB-05: with verbose enabled the function attempts to print the source via print_source_function() (which gracefully fails when the file does not exist). Traces to HLR-012 (Verbose Mode), HLR-027 (Conditional Source Body Output), HLR-030 (External-Tool Dependency Isolation), HLR-043 (External ctags Dependency Scope). |
| 21 | <a id="test_process_in_error_block_null_args"></a>`test_process_in_error_block_null_args` | `LLR-PIEB-02` | Verifies LLR-PIEB-02: NULL pointers are no-ops. Traces to HLR-039 (Defensive Argument Validation). |
| 22 | <a id="test_process_in_error_block_stack_trace"></a>`test_process_in_error_block_stack_trace` | `LLR-PIEB-03` | Verifies LLR-PIEB-03: a "  at " / "  by " line is dispatched to process_stack_trace_line and the block remains active. Traces to HLR-020 (Stack Frame Recognition), HLR-021 (Stack Frame Decoding). |
| 23 | <a id="test_process_in_error_block_non_stack_trace"></a>`test_process_in_error_block_non_stack_trace` | `LLR-PIEB-04` | Verifies LLR-PIEB-04: a non-stack-trace line ends the block via finalize_error_block, clearing in_error_block. Traces to HLR-017 (Error Block Termination). |
| 24 | <a id="test_process_summary_lines_null"></a>`test_process_summary_lines_null` | `LLR-PSL-02` | Verifies LLR-PSL-02: NULL line_content is a no-op. Traces to HLR-039 (Defensive Argument Validation). |
| 25 | <a id="test_process_summary_lines_leak_summary"></a>`test_process_summary_lines_leak_summary` | `LLR-PSL-03`, `LLR-PSL-04` | Verifies LLR-PSL-03 and LLR-PSL-04 with print_leak_summary enabled: the LEAK SUMMARY header and each of the four leak categories are recognised and dispatched. Traces to HLR-031 (Leak Summary Recognition), HLR-032 (Leak Summary Output Gated by Options), HLR-033 (Leak Detail Formatting). |
| 26 | <a id="test_process_summary_lines_leak_without_flag"></a>`test_process_summary_lines_leak_without_flag` | `LLR-PSL-03`, `LLR-PSL-04` | Verifies the negative gating path of LLR-PSL-03 and LLR-PSL-04: with print_leak_summary and verbose both false, recognised leak lines produce no output. Traces to HLR-032 (Leak Summary Output Gated by Options). |
| 27 | <a id="test_process_summary_lines_error_summary"></a>`test_process_summary_lines_error_summary` | `LLR-PSL-05` | Verifies LLR-PSL-05: the ERROR SUMMARY line is always dispatched to print_final_error_summary regardless of any configuration flag. Traces to HLR-034 (Final Error Summary Output). |
| 28 | <a id="test_process_summary_lines_no_match"></a>`test_process_summary_lines_no_match` | — | Verifies the fall-through path of process_summary_lines: lines that match no recognised summary marker produce no output and do not crash. Traces to HLR-031 (Leak Summary Recognition), HLR-040 (Non-Fatal Recovery from Malformed Input). |
| 29 | <a id="test_print_leak_summary_line_parse_success"></a>`test_print_leak_summary_line_parse_success` | `LLR-PLSL-02`, `LLR-PLSL-03` | Verifies LLR-PLSL-02 and LLR-PLSL-03: a well-formed leak line is parsed into bytes/blocks counts and reformatted. Traces to HLR-005 (Locale Initialisation), HLR-033 (Leak Detail Formatting). |
| 30 | <a id="test_print_leak_summary_line_parse_fail_with_newline"></a>`test_print_leak_summary_line_parse_fail_with_newline` | `LLR-PLSL-04` | Verifies LLR-PLSL-04 (newline branch): an unparseable line that already ends in '\n' is printed verbatim without an extra newline. Traces to HLR-035 (Robust Numeric Parsing), HLR-040 (Non-Fatal Recovery from Malformed Input). |
| 31 | <a id="test_print_leak_summary_line_parse_fail_no_newline"></a>`test_print_leak_summary_line_parse_fail_no_newline` | `LLR-PLSL-04` | Verifies LLR-PLSL-04 (no-newline branch): an unparseable line without a trailing newline is printed with one appended. Traces to HLR-035 (Robust Numeric Parsing), HLR-040 (Non-Fatal Recovery from Malformed Input). |
| 32 | <a id="test_print_final_error_summary_parse_success"></a>`test_print_final_error_summary_parse_success` | `LLR-PFES-02`, `LLR-PFES-03` | Verifies LLR-PFES-02 and LLR-PFES-03: the FINAL COUNTS block is emitted with Total Errors taken from ParseState and Possible Leaks computed as Valgrind's total minus Total Errors. Traces to HLR-005 (Locale Initialisation), HLR-034 (Final Error Summary Output), HLR-038 (Stateful Parsing Context). |
| 33 | <a id="test_print_final_error_summary_parse_fail_with_newline"></a>`test_print_final_error_summary_parse_fail_with_newline` | `LLR-PFES-04` | Verifies LLR-PFES-04 (newline branch). Traces to HLR-035 (Robust Numeric Parsing), HLR-040 (Non-Fatal Recovery from Malformed Input). |
| 34 | <a id="test_print_final_error_summary_parse_fail_no_newline"></a>`test_print_final_error_summary_parse_fail_no_newline` | `LLR-PFES-04` | Verifies LLR-PFES-04 (no-newline branch). Traces to HLR-035 (Robust Numeric Parsing), HLR-040 (Non-Fatal Recovery from Malformed Input). |
| 35 | <a id="test_print_error_header_with_newline"></a>`test_print_error_header_with_newline` | `LLR-PEH-02`, `LLR-PEH-04` | Verifies LLR-PEH-02..05 with verbose enabled and an error_type that already ends with '\n' (LLR-PEH-04 verbatim branch). Traces to HLR-012 (Verbose Mode), HLR-018 (Error Header Rendering), HLR-024 (Stack Trace Output Gated by Options). |
| 36 | <a id="test_print_error_header_without_newline"></a>`test_print_error_header_without_newline` | `LLR-PEH-04`, `LLR-PEH-05` | Verifies LLR-PEH-04 newline-append branch and LLR-PEH-05 (Call Stack: heading printed when print_stack is on). Traces to HLR-018 (Error Header Rendering), HLR-024 (Stack Trace Output Gated by Options). |
| 37 | <a id="test_print_error_header_no_stack_flag"></a>`test_print_error_header_no_stack_flag` | `LLR-PEH-05` | Verifies the negative branch of LLR-PEH-05: with both flags off, the "Call Stack:" heading is suppressed but the rest of the header still prints. Traces to HLR-018 (Error Header Rendering), HLR-024 (Stack Trace Output Gated by Options). |
| 38 | <a id="test_get_function_name_with_line_number"></a>`test_get_function_name_with_line_number` | `LLR-GFN-03`, `LLR-GFN-05` | Verifies LLR-GFN-03 + LLR-GFN-05: a fully-formed stack frame is reformatted as "function(basename:line)". Traces to HLR-021 (Stack Frame Decoding), HLR-028 (Editor-Jumpable References). |
| 39 | <a id="test_get_function_name_without_line_number"></a>`test_get_function_name_without_line_number` | `LLR-GFN-06` | Verifies LLR-GFN-06: when the line number cannot be parsed (for an "(in <obj>)" frame), the function emits the placeholder "?(?:0)". Traces to HLR-021 (Stack Frame Decoding), HLR-040 (Non-Fatal Recovery from Malformed Input). |
| 40 | <a id="test_get_function_name_paren_in_function"></a>`test_get_function_name_paren_in_function` | `LLR-GFN-04` | Verifies LLR-GFN-04: a function name carrying a parameter list (e.g. C++ overload notation) has the suffix truncated. Traces to HLR-021 (Stack Frame Decoding). |
| 41 | <a id="test_process_log_file_null"></a>`test_process_log_file_null` | `LLR-PLF-02` | Verifies LLR-PLF-02: a NULL FILE* argument is a no-op. Traces to HLR-039 (Defensive Argument Validation). |
| 42 | <a id="test_process_log_file_with_error_block"></a>`test_process_log_file_with_error_block` | `LLR-PLF-04`, `LLR-PLF-09` | Verifies LLR-PLF-04..LLR-PLF-09 end-to-end on a minimal log containing a single error block followed by an ERROR SUMMARY line. Traces to HLR-014 (PID-Prefix Stripping), HLR-016 (Error Block Detection), HLR-018 (Error Header Rendering), HLR-020 (Stack Frame Recognition), HLR-034 (Final Error Summary Output), HLR-036 (Streaming Single-Pass Operation). |
| 43 | <a id="test_process_log_file_eof_in_error_block"></a>`test_process_log_file_eof_in_error_block` | `LLR-PLF-10` | Verifies LLR-PLF-10: when EOF is reached while still inside an error block, finalize_error_block is invoked and a closing separator is printed. Traces to HLR-019 (End-of-Log Error-Block Finalisation). |
| 44 | <a id="test_process_log_file_leak_summary"></a>`test_process_log_file_leak_summary` | `LLR-PLF-09` | Verifies LLR-PLF-09 dispatching to process_summary_lines for a leak-summary-only log. Traces to HLR-031 (Leak Summary Recognition), HLR-033 (Leak Detail Formatting), HLR-034 (Final Error Summary Output). |
| 45 | <a id="test_process_log_file_multiple_errors"></a>`test_process_log_file_multiple_errors` | `LLR-PLF-07`, `LLR-PLF-08` | Verifies LLR-PLF-07 + LLR-PLF-08 sequencing: a finalised block's terminator line is re-evaluated as a possible new error header so that two adjacent blocks are both detected. Traces to HLR-016 (Error Block Detection), HLR-017 (Error Block Termination), HLR-018 (Error Header Rendering). |
| 46 | <a id="test_process_log_file_no_verbose"></a>`test_process_log_file_no_verbose` | — | Verifies the gated-output paths through process_log_file when no optional output flag is enabled: only the error summary is printed. Traces to HLR-024 (Stack Trace Output Gated by Options), HLR-027 (Conditional Source Body Output), HLR-032 (Leak Summary Output Gated by Options), HLR-034 (Final Error Summary Output). |
| 47 | <a id="test_process_log_file_with_print_source"></a>`test_process_log_file_with_print_source` | — | Verifies the print_source path through finalize_error_block when the source file referenced by the captured frame does not exist (the print_source_function helper reports failure and the block still finishes cleanly). Traces to HLR-027 (Conditional Source Body Output), HLR-030 (External-Tool Dependency Isolation), HLR-043 (External ctags Dependency Scope). |
| 48 | <a id="test_extract_file_and_line_format_without_function"></a>`test_extract_file_and_line_format_without_function` | `LLR-EFAL-07` | Verifies LLR-EFAL-07: a line that matches none of the three sscanf patterns returns false. Traces to HLR-021 (Stack Frame Decoding), HLR-040 (Non-Fatal Recovery from Malformed Input). |
| 49 | <a id="test_extract_file_and_line_in_format"></a>`test_extract_file_and_line_in_format` | `LLR-EFAL-06` | Verifies LLR-EFAL-06 (third "(in <obj>)" sscanf pattern) end-to-end. Traces to HLR-021 (Stack Frame Decoding), HLR-040 (Non-Fatal Recovery from Malformed Input). |
| 50 | <a id="test_execute_command_null_args"></a>`test_execute_command_null_args` | `LLR-EC-02` | Verifies LLR-EC-02: NULL pointers and a zero output_size are rejected. Traces to HLR-039 (Defensive Argument Validation). |
| 51 | <a id="test_execute_command_success"></a>`test_execute_command_success` | `LLR-EC-03`, `LLR-EC-06` | Verifies LLR-EC-03..LLR-EC-06: a successful popen/fgets/pclose round trip captures the first line of the command's stdout. Traces to HLR-029 (Multi-Language Source Support), HLR-042 (POSIX Process Helpers). |
| 52 | <a id="test_print_source_function_null_args"></a>`test_print_source_function_null_args` | `LLR-PSF-02` | Verifies LLR-PSF-02: NULL pointers and non-positive line numbers cause the function to bail out with a stderr diagnostic. Traces to HLR-039 (Defensive Argument Validation), HLR-030 (External-Tool Dependency Isolation). |
| 53 | <a id="test_parse_ctags_output_null_args"></a>`test_parse_ctags_output_null_args` | `LLR-PCO-02` | Verifies LLR-PCO-02: any NULL pointer argument is rejected with a stderr diagnostic. Traces to HLR-039 (Defensive Argument Validation), HLR-030 (External-Tool Dependency Isolation). |
| 54 | <a id="test_parse_ctags_output_no_tab"></a>`test_parse_ctags_output_no_tab` | `LLR-PCO-03` | Verifies LLR-PCO-03: ctags output without any tab cannot be split into the expected fields and is rejected. Traces to HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation). |
| 55 | <a id="test_parse_ctags_output_one_tab"></a>`test_parse_ctags_output_one_tab` | `LLR-PCO-03` | Verifies LLR-PCO-03: only a single tab separator is also insufficient. Traces to HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation). |
| 56 | <a id="test_parse_ctags_output_no_line_field"></a>`test_parse_ctags_output_no_line_field` | `LLR-PCO-04` | Verifies LLR-PCO-04: a record with no "line:" field is rejected. Traces to HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation). |
| 57 | <a id="test_parse_ctags_output_bad_file"></a>`test_parse_ctags_output_bad_file` | `LLR-PCO-05` | Verifies LLR-PCO-05: a record that points at a non-existent source file is rejected when fopen fails. Traces to HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation). |
| 58 | <a id="test_parse_ctags_output_unsupported_language"></a>`test_parse_ctags_output_unsupported_language` | `LLR-PCO-08` | Verifies LLR-PCO-08: a language outside {C, C++, Rust, Fortran} causes a non-fatal "Unsupported source language" diagnostic and returns false. Traces to HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation). |
| 59 | <a id="test_parse_ctags_output_c_success"></a>`test_parse_ctags_output_c_success` | `LLR-PCO-06`, `LLR-PCO-09` | Verifies LLR-PCO-06 + LLR-PCO-09: the brace-balanced end-of-function detection for C reports start_line=4 and an end_line on or after the closing brace. Traces to HLR-029 (Multi-Language Source Support). |
| 60 | <a id="test_parse_ctags_output_fortran_success"></a>`test_parse_ctags_output_fortran_success` | `LLR-PCO-07`, `LLR-PCO-09` | Verifies LLR-PCO-07 + LLR-PCO-09: the "END SUBROUTINE <name>" match reports the expected end_line for Fortran source. Traces to HLR-029 (Multi-Language Source Support). |
| 61 | <a id="test_print_source_function_real_c_file"></a>`test_print_source_function_real_c_file` | `LLR-PSF-03`, `LLR-PSF-09` | Verifies LLR-PSF-03..LLR-PSF-09 end-to-end on a real on-disk source file: the function is located via ctags and printed with the target line marked. Traces to HLR-027 (Conditional Source Body Output), HLR-029 (Multi-Language Source Support), HLR-043 (External ctags Dependency Scope). |
| 62 | <a id="test_print_source_function_nonexistent_function"></a>`test_print_source_function_nonexistent_function` | `LLR-PSF-04` | Verifies LLR-PSF-04: when the requested function is not present in the source file, the helper writes a diagnostic and returns without crashing. Traces to HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation). |
| 63 | <a id="test_execute_command_empty_output"></a>`test_execute_command_empty_output` | `LLR-EC-04`, `LLR-EC-06` | Verifies LLR-EC-04..LLR-EC-06 for a command that prints nothing: fgets returns NULL with no ferror and the function still returns true. Traces to HLR-029 (Multi-Language Source Support), HLR-040 (Non-Fatal Recovery from Malformed Input), HLR-042 (POSIX Process Helpers). |
| 64 | <a id="test_main_help_flag"></a>`test_main_help_flag` | `LLR-PCL-04` | Verifies LLR-PCL-04: the -h flag prints usage and exits with success. Traces to HLR-003 (Usage and Help Information Display), HLR-009 (Application Exit Status). |
| 65 | <a id="test_main_unknown_flag"></a>`test_main_unknown_flag` | `LLR-PCL-05` | Verifies LLR-PCL-05: an unknown short option is rejected with an "Unknown option:" diagnostic and EXIT_FAILURE. Traces to HLR-002 (Argument Parsing and Validation), HLR-009 (Application Exit Status), HLR-010 (Application-Level Error Reporting). |
| 66 | <a id="test_main_no_log_file"></a>`test_main_no_log_file` | `LLR-PCL-08` | Verifies LLR-PCL-08: invocation with no positional argument exits with EXIT_FAILURE. Traces to HLR-002 (Argument Parsing and Validation), HLR-003 (Usage and Help Information Display), HLR-009 (Application Exit Status). |
| 67 | <a id="test_main_multiple_log_files"></a>`test_main_multiple_log_files` | `LLR-PCL-07` | Verifies LLR-PCL-07: a second positional argument is rejected with "Multiple log files specified:" and EXIT_FAILURE. Traces to HLR-002 (Argument Parsing and Validation), HLR-009 (Application Exit Status), HLR-010 (Application-Level Error Reporting). |
| 68 | <a id="test_main_nonexistent_file"></a>`test_main_nonexistent_file` | `LLR-MAIN-04`, `LLR-MAIN-05` | Verifies LLR-MAIN-04 + LLR-MAIN-05: when fopen() of the input log fails, main() emits "Error opening file '...': ..." and returns EXIT_FAILURE. Traces to HLR-004 (Input Log File Handling), HLR-009 (Application Exit Status), HLR-010 (Application-Level Error Reporting). |
| 69 | <a id="test_main_stack_flag"></a>`test_main_stack_flag` | `LLR-MAIN-04`, `LLR-MAIN-09`, `LLR-PCL-03` | Verifies LLR-PCL-03 (-t -> print_stack) and LLR-MAIN-04..LLR-MAIN-09 end-to-end on a real Valgrind log produced by the integration build. Traces to HLR-002 (Argument Parsing and Validation), HLR-006 (Parser Module Invocation), HLR-009 (Application Exit Status), HLR-024 (Stack Trace Output Gated by Options). |
| 70 | <a id="test_process_stack_trace_line_ellipsis_no_verbose"></a>`test_process_stack_trace_line_ellipsis_no_verbose` | `LLR-PSTL-06` | Verifies the negative gating branch of LLR-PSTL-06: with both print_stack and verbose off, the ellipsis is not printed and stack_lines_shown is not incremented. Traces to HLR-023 (Bounded Stack Trace Output), HLR-024 (Stack Trace Output Gated by Options). |
| 71 | <a id="test_process_summary_lines_leak_with_verbose"></a>`test_process_summary_lines_leak_with_verbose` | `LLR-PSL-03`, `LLR-PSL-04` | Verifies the verbose-only path of LLR-PSL-03 and LLR-PSL-04: with print_leak_summary off but verbose on, leak lines still print. Traces to HLR-012 (Verbose Mode), HLR-031 (Leak Summary Recognition), HLR-032 (Leak Summary Output Gated by Options). |
| 72 | <a id="test_process_stack_trace_line_ellipsis_verbose"></a>`test_process_stack_trace_line_ellipsis_verbose` | `LLR-PSTL-06` | Verifies LLR-PSTL-06 verbose branch: with verbose on (and print_stack off) the ellipsis is printed once at the STACK_TRACE_CONTEXT_LINES boundary. Traces to HLR-012 (Verbose Mode), HLR-023 (Bounded Stack Trace Output), HLR-024 (Stack Trace Output Gated by Options). |
| 73 | <a id="test_process_log_file_with_leak_flags_only"></a>`test_process_log_file_with_leak_flags_only` | `LLR-PLF-09` | Verifies LLR-PLF-09 dispatching to the leak path with print_leak_summary the only flag enabled. Traces to HLR-031 (Leak Summary Recognition), HLR-032 (Leak Summary Output Gated by Options), HLR-033 (Leak Detail Formatting), HLR-034 (Final Error Summary Output). |
| 74 | <a id="test_process_log_file_error_with_stack_flag"></a>`test_process_log_file_error_with_stack_flag` | `LLR-PSTL-04` | Verifies LLR-PSTL-04 inside a real log: with print_stack the filtered call stack is emitted for an error containing both user- and library-frame entries. Traces to HLR-022 (User-Code Classification), HLR-024 (Stack Trace Output Gated by Options). |
| 75 | <a id="test_process_log_file_many_stack_lines"></a>`test_process_log_file_many_stack_lines` | `LLR-PSTL-06` | Verifies LLR-PSTL-06 in a realistic streaming context: more than STACK_TRACE_CONTEXT_LINES non-user frames trigger exactly one ellipsis and subsequent frames are suppressed. Traces to HLR-023 (Bounded Stack Trace Output), HLR-036 (Streaming Single-Pass Operation). |
| 76 | <a id="test_process_log_file_with_real_source"></a>`test_process_log_file_with_real_source` | — | Verifies the full source-listing pipeline end-to-end against a real temporary source file: process_log_file -> finalize_error_block -> print_source_function -> ctags lookup -> source body printed. Traces to HLR-026 (Conditional Source Reference Output), HLR-027 (Conditional Source Body Output), HLR-029 (Multi-Language Source Support), HLR-043 (External ctags Dependency Scope). |
| 77 | <a id="test_process_log_file_many_stack_lines_no_verbose"></a>`test_process_log_file_many_stack_lines_no_verbose` | — | Verifies the negative gating branch in a streaming context: many non-user frames with both print_stack and verbose off produce no stack output and no ellipsis. Traces to HLR-023 (Bounded Stack Trace Output), HLR-024 (Stack Trace Output Gated by Options). |
| 78 | <a id="test_extract_file_and_line_scope_operator"></a>`test_extract_file_and_line_scope_operator` | `LLR-EFAL-04` | Verifies LLR-EFAL-04 normalisation: a fully-qualified C++ name like "Namespace::Class::method" is reduced to its final identifier "method". Traces to HLR-021 (Stack Frame Decoding), HLR-025 (First-User-Frame Capture). |
| 79 | <a id="test_extract_file_and_line_dotted_suffix"></a>`test_extract_file_and_line_dotted_suffix` | `LLR-EFAL-04` | Verifies LLR-EFAL-04 normalisation: a Fortran-style dotted suffix such as "trigger_invalid_write.3" is truncated at the dot. Traces to HLR-021 (Stack Frame Decoding), HLR-025 (First-User-Frame Capture), HLR-029 (Multi-Language Source Support). |

## 4. LLR Coverage Matrix

Every LLR in [LLRs.md](LLRs.md) and the test(s) that verify it.
LLRs marked **(no direct test)** are exercised transitively or
verified by code review — see
[Traceability.md](Traceability.md) for the per-LLR justification.

| LLR | Function | HLR(s) | Verifying Test(s) |
| --- | -------- | ------ | ----------------- |
| `LLR-GBL-01` | `Module Globals ([src/vgp.c](../src/vgp.c))` | `HLR-002`, `HLR-011`, `HLR-013` | **(no direct test)** |
| `LLR-GBL-02` | `Module Globals ([src/vgp.c](../src/vgp.c))` | `HLR-022`, `HLR-029` | `test_is_user_code_mixed_case_extension` |
| `LLR-GBL-03` | `Module Globals ([src/vgp.c](../src/vgp.c))` | `HLR-022` | `test_is_user_code_at_prefix_with_c_extension_with_usr_include_ignore`, `test_is_user_code_by_prefix_with_cpp_extension_with_lib_ignore` |
| `LLR-GBL-04` | `Module Globals ([src/vgp.c](../src/vgp.c))` | `HLR-016` | `test_check_start_new_error_detects_invalid_read`, `test_check_start_new_error_detects_invalid_write`, `test_check_start_new_error_detects_uninit`, `test_check_start_new_error_detects_invalid_free`, `test_check_start_new_error_detects_mismatched_free`, `test_check_start_new_error_detects_overlap` |
| `LLR-MAIN-01` | `main` | `HLR-001` | **(no direct test)** |
| `LLR-MAIN-02` | `main` | `HLR-005` | **(no direct test)** |
| `LLR-MAIN-03` | `main` | `HLR-002`, `HLR-003` | **(no direct test)** |
| `LLR-MAIN-04` | `main` | `HLR-004` | `test_main_nonexistent_file`, `test_main_stack_flag` |
| `LLR-MAIN-05` | `main` | `HLR-004`, `HLR-009`, `HLR-010` | `test_main_nonexistent_file` |
| `LLR-MAIN-06` | `main` | `HLR-007` | `test_vgp_output_has_header` |
| `LLR-MAIN-07` | `main` | `HLR-006` | **(no direct test)** |
| `LLR-MAIN-08` | `main` | `HLR-008` | **(no direct test)** |
| `LLR-MAIN-09` | `main` | `HLR-009` | `test_main_stack_flag` |
| `LLR-PCL-01` | `parse_command_line` | `HLR-002` | **(no direct test)** |
| `LLR-PCL-02` | `parse_command_line` | `HLR-002` | **(no direct test)** |
| `LLR-PCL-03` | `parse_command_line` | `HLR-002`, `HLR-011` | `test_main_stack_flag` |
| `LLR-PCL-04` | `parse_command_line` | `HLR-003`, `HLR-009` | `test_main_help_flag` |
| `LLR-PCL-05` | `parse_command_line` | `HLR-002`, `HLR-009`, `HLR-010` | `test_main_unknown_flag` |
| `LLR-PCL-06` | `parse_command_line` | `HLR-002` | **(no direct test)** |
| `LLR-PCL-07` | `parse_command_line` | `HLR-002`, `HLR-009`, `HLR-010` | `test_main_multiple_log_files` |
| `LLR-PCL-08` | `parse_command_line` | `HLR-002`, `HLR-003`, `HLR-009` | `test_main_no_log_file` |
| `LLR-PCL-09` | `parse_command_line` | `HLR-002` | **(no direct test)** |
| `LLR-SVPP-01` | `strip_valgrind_pid_prefix` | `HLR-014` | **(no direct test)** |
| `LLR-SVPP-02` | `strip_valgrind_pid_prefix` | `HLR-039` | `test_strip_null_input` |
| `LLR-SVPP-03` | `strip_valgrind_pid_prefix` | `HLR-014`, `HLR-040` | `test_strip_no_prefix_at_all`, `test_strip_empty_string`, `test_strip_leading_spaces_before_prefix` |
| `LLR-SVPP-04` | `strip_valgrind_pid_prefix` | `HLR-014`, `HLR-040` | `test_strip_only_double_equals`, `test_strip_double_equals_no_pid_digits`, `test_strip_valid_prefix_spaces_around_pid`, `test_strip_prefix_like_too_short_for_pid`, `test_strip_valid_prefix_long_pid` |
| `LLR-SVPP-05` | `strip_valgrind_pid_prefix` | `HLR-014`, `HLR-040` | `test_strip_pid_no_trailing_marker`, `test_strip_pid_trailing_double_equals_no_space`, `test_strip_valid_prefix_spaces_around_pid`, `test_strip_valid_prefix_incomplete_end_marker`, `test_strip_prefix_like_too_short_for_end_marker_full` |
| `LLR-SVPP-06` | `strip_valgrind_pid_prefix` | `HLR-014` | `test_strip_valid_prefix_simple`, `test_strip_valid_prefix_spaces_around_pid`, `test_strip_valid_prefix_no_message_after`, `test_strip_valid_prefix_short_end_marker`, `test_strip_line_is_exact_prefix`, `test_strip_multiple_potential_prefixes`, `test_strip_leading_spaces_before_prefix` |
| `LLR-IUCST-01` | `is_user_code_stack_trace` | `HLR-022` | **(no direct test)** |
| `LLR-IUCST-02` | `is_user_code_stack_trace` | `HLR-039` | `test_is_user_code_null_input` |
| `LLR-IUCST-03` | `is_user_code_stack_trace` | `HLR-020`, `HLR-022` | `test_is_user_code_no_prefix`, `test_is_user_code_at_prefix_no_extension`, `test_is_user_code_by_prefix_no_extension`, `test_is_user_code_at_prefix_with_c_extension_no_ignore`, `test_is_user_code_by_prefix_with_cpp_extension_no_ignore` |
| `LLR-IUCST-04` | `is_user_code_stack_trace` | `HLR-022`, `HLR-029` | `test_is_user_code_at_prefix_no_extension`, `test_is_user_code_by_prefix_no_extension`, `test_is_user_code_at_prefix_with_c_extension_no_ignore`, `test_is_user_code_by_prefix_with_cpp_extension_no_ignore`, `test_is_user_code_at_prefix_with_h_extension_no_ignore`, `test_is_user_code_by_prefix_with_hpp_extension_no_ignore`, `test_is_user_code_at_prefix_with_c_extension_with_question_marks_ignore`, `test_is_user_code_at_prefix_no_user_extension`, `test_is_user_code_just_prefix`, `test_is_user_code_mixed_case_extension` |
| `LLR-IUCST-05` | `is_user_code_stack_trace` | `HLR-022` | `test_is_user_code_at_prefix_with_c_extension_no_ignore`, `test_is_user_code_by_prefix_with_cpp_extension_no_ignore`, `test_is_user_code_at_prefix_with_c_extension_with_usr_include_ignore`, `test_is_user_code_by_prefix_with_cpp_extension_with_lib_ignore` |
| `LLR-IUCST-06` | `is_user_code_stack_trace` | `HLR-022` | `test_is_user_code_at_prefix_with_c_extension_no_ignore`, `test_is_user_code_by_prefix_with_cpp_extension_no_ignore`, `test_is_user_code_at_prefix_with_h_extension_no_ignore`, `test_is_user_code_by_prefix_with_hpp_extension_no_ignore` |
| `LLR-GFN-01` | `get_function_name` | `HLR-021` | **(no direct test)** |
| `LLR-GFN-02` | `get_function_name` | `HLR-021`, `HLR-037` | **(no direct test)** |
| `LLR-GFN-03` | `get_function_name` | `HLR-021` | `test_get_function_name_with_line_number` |
| `LLR-GFN-04` | `get_function_name` | `HLR-021` | `test_get_function_name_paren_in_function` |
| `LLR-GFN-05` | `get_function_name` | `HLR-021`, `HLR-028` | `test_get_function_name_with_line_number` |
| `LLR-GFN-06` | `get_function_name` | `HLR-021`, `HLR-040` | `test_get_function_name_without_line_number` |
| `LLR-GFN-07` | `get_function_name` | `HLR-021` | **(no direct test)** |
| `LLR-EFAL-01` | `extract_file_and_line` | `HLR-021`, `HLR-025` | **(no direct test)** |
| `LLR-EFAL-02` | `extract_file_and_line` | `HLR-039` | `test_extract_file_and_line_rejects_null_args` |
| `LLR-EFAL-03` | `extract_file_and_line` | `HLR-021`, `HLR-025` | `test_extract_file_and_line_parses_stack_entry_with_line` |
| `LLR-EFAL-04` | `extract_file_and_line` | `HLR-021`, `HLR-025` | `test_extract_file_and_line_parses_stack_entry_with_line`, `test_extract_file_and_line_scope_operator`, `test_extract_file_and_line_dotted_suffix` |
| `LLR-EFAL-05` | `extract_file_and_line` | `HLR-021`, `HLR-040` | **(no direct test)** |
| `LLR-EFAL-06` | `extract_file_and_line` | `HLR-021`, `HLR-040` | `test_extract_file_and_line_parses_entry_without_line_number`, `test_extract_file_and_line_in_format` |
| `LLR-EFAL-07` | `extract_file_and_line` | `HLR-021`, `HLR-040` | `test_extract_file_and_line_format_without_function` |
| `LLR-PEH-01` | `print_error_header` | `HLR-018` | **(no direct test)** |
| `LLR-PEH-02` | `print_error_header` | `HLR-018` | `test_print_error_header_with_newline` |
| `LLR-PEH-03` | `print_error_header` | `HLR-018`, `HLR-038` | `test_vgp_output_error_counts`, `test_vgp_output_has_expected_error_blocks` |
| `LLR-PEH-04` | `print_error_header` | `HLR-018`, `HLR-040` | `test_vgp_output_has_expected_error_blocks`, `test_print_error_header_with_newline`, `test_print_error_header_without_newline` |
| `LLR-PEH-05` | `print_error_header` | `HLR-012`, `HLR-024` | `test_print_error_header_without_newline`, `test_print_error_header_no_stack_flag` |
| `LLR-PLSL-01` | `print_leak_summary_line` | `HLR-033` | **(no direct test)** |
| `LLR-PLSL-02` | `print_leak_summary_line` | `HLR-005`, `HLR-033` | `test_print_leak_summary_line_parse_success` |
| `LLR-PLSL-03` | `print_leak_summary_line` | `HLR-033` | `test_print_leak_summary_line_parse_success` |
| `LLR-PLSL-04` | `print_leak_summary_line` | `HLR-035`, `HLR-040` | `test_print_leak_summary_line_parse_fail_with_newline`, `test_print_leak_summary_line_parse_fail_no_newline` |
| `LLR-PFES-01` | `print_final_error_summary` | `HLR-034` | **(no direct test)** |
| `LLR-PFES-02` | `print_final_error_summary` | `HLR-005`, `HLR-034` | `test_print_final_error_summary_parse_success` |
| `LLR-PFES-03` | `print_final_error_summary` | `HLR-034`, `HLR-038` | `test_vgp_output_error_counts`, `test_vgp_output_leak_counts`, `test_vgp_output_has_leak_summary`, `test_print_final_error_summary_parse_success` |
| `LLR-PFES-04` | `print_final_error_summary` | `HLR-035`, `HLR-040` | `test_print_final_error_summary_parse_fail_with_newline`, `test_print_final_error_summary_parse_fail_no_newline` |
| `LLR-EC-01` | `execute_command` | `HLR-029`, `HLR-042`, `HLR-043` | **(no direct test)** |
| `LLR-EC-02` | `execute_command` | `HLR-039` | `test_execute_command_null_args` |
| `LLR-EC-03` | `execute_command` | `HLR-030`, `HLR-042` | `test_execute_command_success` |
| `LLR-EC-04` | `execute_command` | `HLR-029` | `test_execute_command_empty_output` |
| `LLR-EC-05` | `execute_command` | `HLR-030`, `HLR-040` | **(no direct test)** |
| `LLR-EC-06` | `execute_command` | `HLR-008`, `HLR-029` | `test_execute_command_success`, `test_execute_command_empty_output` |
| `LLR-PCO-01` | `parse_ctags_output` | `HLR-029` | **(no direct test)** |
| `LLR-PCO-02` | `parse_ctags_output` | `HLR-030`, `HLR-039` | `test_parse_ctags_output_null_args` |
| `LLR-PCO-03` | `parse_ctags_output` | `HLR-029`, `HLR-030` | `test_parse_ctags_output_no_tab`, `test_parse_ctags_output_one_tab` |
| `LLR-PCO-04` | `parse_ctags_output` | `HLR-029`, `HLR-030` | `test_parse_ctags_output_no_line_field` |
| `LLR-PCO-05` | `parse_ctags_output` | `HLR-029`, `HLR-030` | `test_parse_ctags_output_bad_file` |
| `LLR-PCO-06` | `parse_ctags_output` | `HLR-029` | `test_parse_ctags_output_c_success` |
| `LLR-PCO-07` | `parse_ctags_output` | `HLR-029` | `test_parse_ctags_output_fortran_success` |
| `LLR-PCO-08` | `parse_ctags_output` | `HLR-029`, `HLR-030` | `test_parse_ctags_output_unsupported_language` |
| `LLR-PCO-09` | `parse_ctags_output` | `HLR-029`, `HLR-030` | `test_parse_ctags_output_c_success`, `test_parse_ctags_output_fortran_success` |
| `LLR-PSF-01` | `print_source_function` | `HLR-027`, `HLR-029` | **(no direct test)** |
| `LLR-PSF-02` | `print_source_function` | `HLR-030`, `HLR-039` | `test_print_source_function_null_args` |
| `LLR-PSF-03` | `print_source_function` | `HLR-029`, `HLR-030`, `HLR-043` | `test_print_source_function_real_c_file` |
| `LLR-PSF-04` | `print_source_function` | `HLR-029`, `HLR-030` | `test_print_source_function_nonexistent_function` |
| `LLR-PSF-05` | `print_source_function` | `HLR-029`, `HLR-030` | **(no direct test)** |
| `LLR-PSF-06` | `print_source_function` | `HLR-027`, `HLR-030` | **(no direct test)** |
| `LLR-PSF-07` | `print_source_function` | `HLR-027`, `HLR-030` | **(no direct test)** |
| `LLR-PSF-08` | `print_source_function` | `HLR-027`, `HLR-038` | **(no direct test)** |
| `LLR-PSF-09` | `print_source_function` | `HLR-008` | `test_print_source_function_real_c_file` |
| `LLR-IPS-01` | `initialize_parse_state` | `HLR-038` | **(no direct test)** |
| `LLR-IPS-02` | `initialize_parse_state` | `HLR-039` | `test_initialize_parse_state_null` |
| `LLR-IPS-03` | `initialize_parse_state` | `HLR-038` | `test_initialize_parse_state_normal` |
| `LLR-CSNE-01` | `check_start_new_error` | `HLR-016` | **(no direct test)** |
| `LLR-CSNE-02` | `check_start_new_error` | `HLR-039` | `test_check_start_new_error_null_args` |
| `LLR-CSNE-03` | `check_start_new_error` | `HLR-016`, `HLR-038` | `test_check_start_new_error_already_in_block` |
| `LLR-CSNE-04` | `check_start_new_error` | `HLR-016`, `HLR-018`, `HLR-038` | `test_vgp_output_error_counts`, `test_check_start_new_error_detects_invalid_read`, `test_check_start_new_error_detects_invalid_write`, `test_check_start_new_error_detects_uninit`, `test_check_start_new_error_detects_invalid_free`, `test_check_start_new_error_detects_mismatched_free`, `test_check_start_new_error_detects_overlap` |
| `LLR-CSNE-05` | `check_start_new_error` | `HLR-016` | `test_check_start_new_error_no_match` |
| `LLR-PSTL-01` | `process_stack_trace_line` | `HLR-020`, `HLR-021` | **(no direct test)** |
| `LLR-PSTL-02` | `process_stack_trace_line` | `HLR-039` | `test_process_stack_trace_line_null_args` |
| `LLR-PSTL-03` | `process_stack_trace_line` | `HLR-022`, `HLR-023` | **(no direct test)** |
| `LLR-PSTL-04` | `process_stack_trace_line` | `HLR-012`, `HLR-021`, `HLR-024`, `HLR-028` | `test_process_stack_trace_line_with_print_stack`, `test_process_log_file_error_with_stack_flag` |
| `LLR-PSTL-05` | `process_stack_trace_line` | `HLR-022`, `HLR-025` | `test_process_stack_trace_line_finds_user_code`, `test_process_stack_trace_line_non_user_code` |
| `LLR-PSTL-06` | `process_stack_trace_line` | `HLR-023`, `HLR-024` | `test_process_stack_trace_line_ellipsis_branch`, `test_process_stack_trace_line_ellipsis_no_verbose`, `test_process_stack_trace_line_ellipsis_verbose`, `test_process_log_file_many_stack_lines` |
| `LLR-FEB-01` | `finalize_error_block` | `HLR-017`, `HLR-026` | **(no direct test)** |
| `LLR-FEB-02` | `finalize_error_block` | `HLR-039` | `test_finalize_error_block_null` |
| `LLR-FEB-03` | `finalize_error_block` | `HLR-022`, `HLR-025` | `test_finalize_error_block_no_user_code` |
| `LLR-FEB-04` | `finalize_error_block` | `HLR-026`, `HLR-028` | `test_finalize_error_block_with_print_function` |
| `LLR-FEB-05` | `finalize_error_block` | `HLR-012`, `HLR-027`, `HLR-043` | `test_finalize_error_block_with_print_function`, `test_finalize_error_block_with_verbose` |
| `LLR-FEB-06` | `finalize_error_block` | `HLR-017`, `HLR-038` | `test_finalize_error_block_no_user_code`, `test_finalize_error_block_with_print_function` |
| `LLR-PIEB-01` | `process_in_error_block` | `HLR-017`, `HLR-020` | **(no direct test)** |
| `LLR-PIEB-02` | `process_in_error_block` | `HLR-039` | `test_process_in_error_block_null_args` |
| `LLR-PIEB-03` | `process_in_error_block` | `HLR-020`, `HLR-021` | `test_process_in_error_block_stack_trace` |
| `LLR-PIEB-04` | `process_in_error_block` | `HLR-017`, `HLR-026`, `HLR-027` | `test_process_in_error_block_non_stack_trace` |
| `LLR-PSL-01` | `process_summary_lines` | `HLR-031`, `HLR-034` | **(no direct test)** |
| `LLR-PSL-02` | `process_summary_lines` | `HLR-039` | `test_process_summary_lines_null` |
| `LLR-PSL-03` | `process_summary_lines` | `HLR-012`, `HLR-031`, `HLR-032` | `test_vgp_output_has_leak_summary`, `test_process_summary_lines_leak_summary`, `test_process_summary_lines_leak_without_flag`, `test_process_summary_lines_leak_with_verbose` |
| `LLR-PSL-04` | `process_summary_lines` | `HLR-012`, `HLR-031`, `HLR-032`, `HLR-033` | `test_process_summary_lines_leak_summary`, `test_process_summary_lines_leak_without_flag`, `test_process_summary_lines_leak_with_verbose` |
| `LLR-PSL-05` | `process_summary_lines` | `HLR-034` | `test_process_summary_lines_error_summary` |
| `LLR-PLF-01` | `process_log_file` | `HLR-006`, `HLR-036` | **(no direct test)** |
| `LLR-PLF-02` | `process_log_file` | `HLR-039` | `test_process_log_file_null` |
| `LLR-PLF-03` | `process_log_file` | `HLR-037`, `HLR-038` | **(no direct test)** |
| `LLR-PLF-04` | `process_log_file` | `HLR-036`, `HLR-037` | `test_process_log_file_with_error_block` |
| `LLR-PLF-05` | `process_log_file` | `HLR-014` | **(no direct test)** |
| `LLR-PLF-06` | `process_log_file` | `HLR-015` | **(no direct test)** |
| `LLR-PLF-07` | `process_log_file` | `HLR-017`, `HLR-020` | `test_process_log_file_multiple_errors` |
| `LLR-PLF-08` | `process_log_file` | `HLR-016`, `HLR-018` | `test_process_log_file_multiple_errors` |
| `LLR-PLF-09` | `process_log_file` | `HLR-031`, `HLR-034` | `test_process_log_file_with_error_block`, `test_process_log_file_leak_summary`, `test_process_log_file_with_leak_flags_only` |
| `LLR-PLF-10` | `process_log_file` | `HLR-019`, `HLR-026` | `test_process_log_file_eof_in_error_block` |

## 5. Integration Test Environment

The integration tests inspect on-disk artefacts produced by the
`make test` build step. The relevant per-language fixtures are:

| Fixture | Source | Generated Valgrind Log | Generated vgp Output |
| ------- | ------ | --- | --- |
| `c_error_generator` | [test/integration/c_error_generator.c](../test/integration/c_error_generator.c) | `build/c_error_generator_int_app_c_valgrind.log` | `build/c_error_generator_int_app_c_vgp_output.log` |
| `cpp_error_generator` | [test/integration/cpp_error_generator.cpp](../test/integration/cpp_error_generator.cpp) | `build/cpp_error_generator_int_app_cpp_valgrind.log` | `build/cpp_error_generator_int_app_cpp_vgp_output.log` |
| `fortran_error_generator` | [test/integration/fortran_error_generator.f90](../test/integration/fortran_error_generator.f90) | `build/fortran_error_generator_int_app_f90_valgrind.log` | `build/fortran_error_generator_int_app_f90_vgp_output.log` |
| `rust_error_generator` | [test/integration/rust_error_generator.rs](../test/integration/rust_error_generator.rs) | `build/rust_error_generator_int_app_rs_valgrind.log` | `build/rust_error_generator_int_app_rs_vgp_output.log` |
| `ada_error_generator` | [test/integration/ada_error_generator.adb](../test/integration/ada_error_generator.adb) | `build/ada_error_generator_int_app_ada_valgrind.log` | `build/ada_error_generator_int_app_ada_vgp_output.log` |
Each generator is compiled with its language toolchain by the
Makefile, then executed under Valgrind to produce the input log,
then `vgp` is run on that log to produce the formatted output
the integration tests inspect. A second Valgrind run captures
`vgp` itself executing each input, producing
`*_vgp_itself_valgrind.log` files that
`test_vgp_itself_no_valgrind_errors` checks for zero errors.

## 6. Tooling and Dependencies

| Component | Required For | Notes |
| --------- | ------------ | ----- |
| `gcc` | Build of `vgp` and unit tests | C99, with `-fanalyzer`, `-fprofile-arcs`, `-ftest-coverage`. |
| `g++` | Integration C++ generator | Standard C++ compiler. |
| `gfortran` | Integration Fortran generator | Required for `fortran_error_generator`. |
| `gnatmake` | Integration Ada generator | Required for `ada_error_generator`. |
| `rustc` | Integration Rust generator | Required for `rust_error_generator`. |
| `valgrind` | Generation of input logs and self-check | `--leak-check=full --show-leak-kinds=all` per the Makefile. |
| `cmocka` | Test framework | Linked into the test runner. |
| `ctags` (Universal Ctags) | `print_source_function` runtime | Indirectly exercised by `test_print_source_function_*` and `test_parse_ctags_output_*`. |
| `gcov` | Coverage measurement | Driven by `make coverage`. |

## 7. Maintenance

Every new test added under [test/](../test/) shall include a
doc-comment block citing the LLR(s) and/or HLR(s) it verifies.
New LLRs shall be added to [LLRs.md](LLRs.md) with a `*Trace:*`
line naming the HLR(s) they implement. After any such change,
this plan, [Traceability.md](Traceability.md), and
[Traceability_Graph.md](Traceability_Graph.md) shall be
regenerated from the source-of-truth comment blocks.
