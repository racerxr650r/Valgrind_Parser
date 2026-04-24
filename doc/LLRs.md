# Low-Level Requirements (LLRs) for Valgrind Parser (vgp)

**Version:** 2.0
**Date:** 4/21/2026
**Author(s):** John Anderson

## 1. Introduction

This document defines the low-level requirements (LLRs) for the
implementation of `vgp`. Each LLR is bound to a specific function in
[src/main.c](../src/main.c) or [src/vgp.c](../src/vgp.c) and traces back
to one or more high-level requirements in [HLRs.md](HLRs.md).

### 1.1 Identifier Scheme
LLR identifiers use a per-function prefix followed by a two-digit
sequence number, e.g. `LLR-PCL-03` for the third LLR of
`parse_command_line`. The prefixes used in this document are:

| Prefix | Function |
| ------ | -------- |
| `LLR-MAIN` | `main` ([src/main.c](../src/main.c)) |
| `LLR-PCL` | `parse_command_line` ([src/main.c](../src/main.c)) |
| `LLR-SVPP` | `strip_valgrind_pid_prefix` ([src/vgp.c](../src/vgp.c)) |
| `LLR-IUCST` | `is_user_code_stack_trace` ([src/vgp.c](../src/vgp.c)) |
| `LLR-GFN` | `get_function_name` ([src/vgp.c](../src/vgp.c)) |
| `LLR-EFAL` | `extract_file_and_line` ([src/vgp.c](../src/vgp.c)) |
| `LLR-PEH` | `print_error_header` ([src/vgp.c](../src/vgp.c)) |
| `LLR-PLSL` | `print_leak_summary_line` ([src/vgp.c](../src/vgp.c)) |
| `LLR-PFES` | `print_final_error_summary` ([src/vgp.c](../src/vgp.c)) |
| `LLR-EC` | `execute_command` ([src/vgp.c](../src/vgp.c)) |
| `LLR-PCO` | `parse_ctags_output` ([src/vgp.c](../src/vgp.c)) |
| `LLR-PSF` | `print_source_function` ([src/vgp.c](../src/vgp.c)) |
| `LLR-IPS` | `initialize_parse_state` ([src/vgp.c](../src/vgp.c)) |
| `LLR-CSNE` | `check_start_new_error` ([src/vgp.c](../src/vgp.c)) |
| `LLR-PSTL` | `process_stack_trace_line` ([src/vgp.c](../src/vgp.c)) |
| `LLR-FEB` | `finalize_error_block` ([src/vgp.c](../src/vgp.c)) |
| `LLR-PIEB` | `process_in_error_block` ([src/vgp.c](../src/vgp.c)) |
| `LLR-PSL` | `process_summary_lines` ([src/vgp.c](../src/vgp.c)) |
| `LLR-PLF` | `process_log_file` ([src/vgp.c](../src/vgp.c)) |
| `LLR-GBL` | Module globals ([src/vgp.c](../src/vgp.c)) |

LLR IDs are stable. Updates that change behaviour replace the existing
ID's text in place; new behaviour takes the next unused number for that
function.

### 1.2 Traceability
Every LLR carries a `*Trace:*` line citing the HLR identifiers from
[HLRs.md](HLRs.md) (with the requirement's short name) that it
implements. This is the structure used for the forthcoming traceability
matrix.

## 2. Module Globals ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-GBL-01"></a>**LLR-GBL-01** — A file-scope `AppConfig app_config` shall be defined
    with all boolean flags initialised to `false` and `log_file`
    initialised to `NULL`.
    *Trace:* HLR-002 (Argument Parsing and Validation), HLR-011 (Configurable Output Sections), HLR-013 (No Hidden Configuration).

*   <a id="LLR-GBL-02"></a>**LLR-GBL-02** — A `NULL`-terminated `const char *USER_CODE_EXTENSIONS[]`
    array shall be defined containing the file extensions recognised as
    user code: `".c"`, `".cpp"`, `".h"`, `".hpp"`, `".C"`, `".CPP"`,
    `".H"`, `".HPP"`, `".cc"`, `".hh"`, `".cxx"`, `".hxx"`, `".f90"`,
    `".f"`, `".F"`, `".for"`, `".ada"`, `".ads"`, `".adb"`, `".rs"`.
    *Trace:* HLR-022 (User-Code Classification), HLR-029 (Multi-Language Source Support).

*   <a id="LLR-GBL-03"></a>**LLR-GBL-03** — A `NULL`-terminated `const char *IGNORE_PATHS[]`
    array shall be defined containing the path fragments `"/usr/"`,
    `"/lib/"`, and `"vg_"`.
    *Trace:* HLR-022 (User-Code Classification).

*   <a id="LLR-GBL-04"></a>**LLR-GBL-04** — A `NULL`-terminated `const char *ERROR_KEYWORDS[]`
    array shall be defined containing at least the substrings
    `"Invalid read"`, `"Invalid write"`,
    `"depends on uninitialised value"`, `"Invalid free"`,
    `"Mismatched free"`, `"Source and destination overlap"`, and
    `"Invalid usage of address"`.
    *Trace:* HLR-016 (Error Block Detection).

## 3. `main` ([src/main.c](../src/main.c))

*   <a id="LLR-MAIN-01"></a>**LLR-MAIN-01** — `main` shall be defined with the signature
    `int main(int argc, char *argv[])`.
    *Trace:* HLR-001 (Command-Line Interface Provision).

*   <a id="LLR-MAIN-02"></a>**LLR-MAIN-02** — As its first executable statement, `main` shall
    call `setlocale(LC_NUMERIC, "")`.
    *Trace:* HLR-005 (Locale Initialisation).

*   <a id="LLR-MAIN-03"></a>**LLR-MAIN-03** — `main` shall invoke `parse_command_line(argc, argv)`
    to populate the global `app_config` and to dispatch the help/error
    exit paths.
    *Trace:* HLR-002 (Argument Parsing and Validation), HLR-003 (Usage and Help Information Display).

*   <a id="LLR-MAIN-04"></a>**LLR-MAIN-04** — `main` shall capture `app_config.log_file` into a
    local `const char *filename` and shall call `fopen(filename, "r")`
    to obtain a `FILE *` for the log.
    *Trace:* HLR-004 (Input Log File Handling).

*   <a id="LLR-MAIN-05"></a>**LLR-MAIN-05** — If `fopen` returns `NULL`, `main` shall write the
    diagnostic `"Error opening file '%s': %s\n"` (with `filename` and
    `strerror(errno)`) to `stderr` and shall return `EXIT_FAILURE`.
    *Trace:* HLR-004 (Input Log File Handling), HLR-009 (Application Exit Status), HLR-010 (Application-Level Error Reporting).

*   <a id="LLR-MAIN-06"></a>**LLR-MAIN-06** — On successful `fopen`, `main` shall print
    `"Parsing Valgrind Log File: %s\n"` (with `filename`) to `stdout`
    before invoking the parser.
    *Trace:* HLR-007 (Initial User Feedback).

*   <a id="LLR-MAIN-07"></a>**LLR-MAIN-07** — `main` shall call `process_log_file(file)` exactly
    once with the open `FILE *`.
    *Trace:* HLR-006 (Parser Module Invocation).

*   <a id="LLR-MAIN-08"></a>**LLR-MAIN-08** — On return from `process_log_file`, `main` shall
    call `fclose(file)`.
    *Trace:* HLR-008 (Resource Management).

*   <a id="LLR-MAIN-09"></a>**LLR-MAIN-09** — `main` shall return `EXIT_SUCCESS` after a
    successful `fclose`.
    *Trace:* HLR-009 (Application Exit Status).

## 4. `parse_command_line` ([src/main.c](../src/main.c))

The current implementation is a hand-rolled `argv` walker; it does not
use `getopt`.

*   <a id="LLR-PCL-01"></a>**LLR-PCL-01** — `parse_command_line` shall be defined with the
    signature `void parse_command_line(int argc, char *argv[])`.
    *Trace:* HLR-002 (Argument Parsing and Validation).

*   <a id="LLR-PCL-02"></a>**LLR-PCL-02** — The function shall iterate over `argv[i]` for `i`
    from `1` to `argc-1` inclusive.
    *Trace:* HLR-002 (Argument Parsing and Validation).

*   <a id="LLR-PCL-03"></a>**LLR-PCL-03** — When `argv[i][0] == '-'`, the function shall
    dispatch on `argv[i][1]` and set the corresponding `app_config`
    flag:
    *   `'v'` -> `app_config.verbose = true`.
    *   `'s'` -> `app_config.print_source = true`.
    *   `'l'` -> `app_config.print_leak_summary = true`.
    *   `'t'` -> `app_config.print_stack = true`.
    *Trace:* HLR-002 (Argument Parsing and Validation), HLR-011 (Configurable Output Sections).

*   <a id="LLR-PCL-04"></a>**LLR-PCL-04** — When `argv[i][1] == 'h'`, the function shall print a
    usage block to `stdout` listing each supported option (`-v`, `-s`,
    `-l`, `-t`, `-h`) with a short description and shall call
    `exit(EXIT_SUCCESS)`.
    *Trace:* HLR-003 (Usage and Help Information Display), HLR-009 (Application Exit Status).

*   <a id="LLR-PCL-05"></a>**LLR-PCL-05** — Any option character not in the set
    `{'v','s','l','t','h'}` shall cause the function to write
    `"Unknown option: %s\n"` to `stderr` and call `exit(EXIT_FAILURE)`.
    *Trace:* HLR-002 (Argument Parsing and Validation), HLR-009 (Application Exit Status), HLR-010 (Application-Level Error Reporting).

*   <a id="LLR-PCL-06"></a>**LLR-PCL-06** — When `argv[i][0] != '-'`, the token shall be treated
    as the positional log-file path. If `app_config.log_file` is
    `NULL`, the token shall be assigned to it.
    *Trace:* HLR-002 (Argument Parsing and Validation).

*   <a id="LLR-PCL-07"></a>**LLR-PCL-07** — If a non-option token is encountered while
    `app_config.log_file` is already non-`NULL`, the function shall
    write `"Multiple log files specified: %s and %s\n"` (with the prior
    and current paths) to `stderr` and call `exit(EXIT_FAILURE)`.
    *Trace:* HLR-002 (Argument Parsing and Validation), HLR-009 (Application Exit Status), HLR-010 (Application-Level Error Reporting).

*   <a id="LLR-PCL-08"></a>**LLR-PCL-08** — After the loop, if `app_config.log_file` is `NULL`,
    the function shall write `"No log file specified. Use -h for help.\n"`
    to `stderr` and call `exit(EXIT_FAILURE)`.
    *Trace:* HLR-002 (Argument Parsing and Validation), HLR-003 (Usage and Help Information Display), HLR-009 (Application Exit Status).

*   <a id="LLR-PCL-09"></a>**LLR-PCL-09** — `parse_command_line` shall not return any value; on
    success it shall return normally with `app_config` populated.
    *Trace:* HLR-002 (Argument Parsing and Validation).

## 5. `strip_valgrind_pid_prefix` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-SVPP-01"></a>**LLR-SVPP-01** — Signature: `char *strip_valgrind_pid_prefix(char *line)`.
    *Trace:* HLR-014 (PID-Prefix Stripping).

*   <a id="LLR-SVPP-02"></a>**LLR-SVPP-02** — If `line` is `NULL`, the function shall return
    `NULL`.
    *Trace:* HLR-039 (Defensive Argument Validation).

*   <a id="LLR-SVPP-03"></a>**LLR-SVPP-03** — The function shall locate the first `"=="`
    substring with `strstr`. If absent, it shall return `line` unchanged.
    *Trace:* HLR-014 (PID-Prefix Stripping), HLR-040 (Non-Fatal Recovery from Malformed Input).

*   <a id="LLR-SVPP-04"></a>**LLR-SVPP-04** — After advancing past the leading `"=="`, the
    function shall skip whitespace via `isspace`, require at least one
    digit (returning `line` unchanged otherwise), and skip the run of
    digits.
    *Trace:* HLR-014 (PID-Prefix Stripping), HLR-040 (Non-Fatal Recovery from Malformed Input).

*   <a id="LLR-SVPP-05"></a>**LLR-SVPP-05** — After the digits, the function shall again skip
    whitespace and shall require the literal sequence `"== "` (matched
    via `strncmp`). If absent, it shall return `line` unchanged.
    *Trace:* HLR-014 (PID-Prefix Stripping), HLR-040 (Non-Fatal Recovery from Malformed Input).

*   <a id="LLR-SVPP-06"></a>**LLR-SVPP-06** — On a successful match the function shall advance
    past the trailing `"== "` and shall return a pointer into the
    original buffer at the start of the message body.
    *Trace:* HLR-014 (PID-Prefix Stripping).

## 6. `is_user_code_stack_trace` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-IUCST-01"></a>**LLR-IUCST-01** — Signature: `bool is_user_code_stack_trace(const char *line)`.
    *Trace:* HLR-022 (User-Code Classification).

*   <a id="LLR-IUCST-02"></a>**LLR-IUCST-02** — If `line` is `NULL`, the function shall return
    `false`.
    *Trace:* HLR-039 (Defensive Argument Validation).

*   <a id="LLR-IUCST-03"></a>**LLR-IUCST-03** — The function shall return `false` if `line` does
    not begin with either `"   at "` or `"   by "` (`strncmp` of the
    leading 6 characters).
    *Trace:* HLR-020 (Stack Frame Recognition), HLR-022 (User-Code Classification).

*   <a id="LLR-IUCST-04"></a>**LLR-IUCST-04** — The function shall iterate `USER_CODE_EXTENSIONS`
    until it finds an entry that appears as a substring of `line` (via
    `strstr`). If no entry matches, it shall return `false`.
    *Trace:* HLR-022 (User-Code Classification), HLR-029 (Multi-Language Source Support).

*   <a id="LLR-IUCST-05"></a>**LLR-IUCST-05** — The function shall iterate `IGNORE_PATHS` and
    return `false` if any entry appears as a substring of `line`.
    *Trace:* HLR-022 (User-Code Classification).

*   <a id="LLR-IUCST-06"></a>**LLR-IUCST-06** — Otherwise the function shall return `true`.
    *Trace:* HLR-022 (User-Code Classification).

## 7. `get_function_name` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-GFN-01"></a>**LLR-GFN-01** — Signature: `char *get_function_name(const char *line, char *newline)`.
    *Trace:* HLR-021 (Stack Frame Decoding).

*   <a id="LLR-GFN-02"></a>**LLR-GFN-02** — The function shall declare local `function` and
    `filename` buffers of size `MAX_LINE_LENGTH`, both initialised to
    `"?"`, and a `line_number` integer initialised to `0`.
    *Trace:* HLR-021 (Stack Frame Decoding), HLR-037 (Bounded Line Buffer).

*   <a id="LLR-GFN-03"></a>**LLR-GFN-03** — The function shall attempt to extract the function
    name, file path, and line number from `line` using
    `sscanf(line, "%*s %*s %[^ ] (%[^:]:%d)", function, filename, &line_number)`.
    *Trace:* HLR-021 (Stack Frame Decoding).

*   <a id="LLR-GFN-04"></a>**LLR-GFN-04** — Any trailing parenthesis suffix in `function`
    (a `'('` not at the final position) shall be truncated to `'\0'` so
    that overload/parameter notation is dropped.
    *Trace:* HLR-021 (Stack Frame Decoding).

*   <a id="LLR-GFN-05"></a>**LLR-GFN-05** — If `line_number` is non-zero, the function shall
    write `"%s(%s:%d)\n"` (with `function`, `basename(filename)`, and
    `line_number`) to `newline` via `snprintf` bounded by the original
    `line` length.
    *Trace:* HLR-021 (Stack Frame Decoding), HLR-028 (Editor-Jumpable References).

*   <a id="LLR-GFN-06"></a>**LLR-GFN-06** — If `line_number` is zero, the function shall write
    `"?(?:0)\n"` to `newline`.
    *Trace:* HLR-021 (Stack Frame Decoding), HLR-040 (Non-Fatal Recovery from Malformed Input).

*   <a id="LLR-GFN-07"></a>**LLR-GFN-07** — The function shall return the `newline` pointer.
    *Trace:* HLR-021 (Stack Frame Decoding).

## 8. `extract_file_and_line` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-EFAL-01"></a>**LLR-EFAL-01** — Signature:
    `bool extract_file_and_line(const char *line, char *filename, char *function_name, int *line_number)`.
    *Trace:* HLR-021 (Stack Frame Decoding), HLR-025 (First-User-Frame Capture).

*   <a id="LLR-EFAL-02"></a>**LLR-EFAL-02** — If any of `line`, `filename`, `function_name`, or
    `line_number` is `NULL`, the function shall return `false`.
    *Trace:* HLR-039 (Defensive Argument Validation).

*   <a id="LLR-EFAL-03"></a>**LLR-EFAL-03** — The function shall first attempt to parse `line`
    via `sscanf(line, "%*s %*s %[^ ] (%[^:]:%d)", temp_function, temp_filename, line_number)`
    against local `MAX_LINE_LENGTH` buffers.
    *Trace:* HLR-021 (Stack Frame Decoding), HLR-025 (First-User-Frame Capture).

*   <a id="LLR-EFAL-04"></a>**LLR-EFAL-04** — On a 3-field match, the function shall copy the
    first whitespace-delimited token of `temp_filename` to `filename`
    and of `temp_function` to `function_name` (via the internal
    `copy_first_token` helper), shall normalise the function name to
    drop parameter lists, dotted suffixes, and namespace prefixes (via
    the internal `normalize_function_name` helper), and shall return
    `true`.
    *Trace:* HLR-021 (Stack Frame Decoding), HLR-025 (First-User-Frame Capture).

*   <a id="LLR-EFAL-05"></a>**LLR-EFAL-05** — If the 3-field parse fails, the function shall try
    `sscanf(line, "%*[^:]: %[^:]:%d", temp_filename, line_number)`. On a
    2-field match it shall copy `temp_filename` into `filename`, set
    `function_name` to the empty string, and return `true`.
    *Trace:* HLR-021 (Stack Frame Decoding), HLR-040 (Non-Fatal Recovery from Malformed Input).

*   <a id="LLR-EFAL-06"></a>**LLR-EFAL-06** — If both prior parses fail, the function shall try
    `sscanf(line, "%*[^:]: %[^(](in %[^)])", temp_function, temp_filename)`.
    On a 2-field match it shall copy the first tokens into the output
    buffers, normalise the function name, set `*line_number = 0`, and
    return `true`.
    *Trace:* HLR-021 (Stack Frame Decoding), HLR-040 (Non-Fatal Recovery from Malformed Input).

*   <a id="LLR-EFAL-07"></a>**LLR-EFAL-07** — If none of the parses match, the function shall
    return `false` without modifying the output buffers.
    *Trace:* HLR-021 (Stack Frame Decoding), HLR-040 (Non-Fatal Recovery from Malformed Input).

## 9. `print_error_header` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-PEH-01"></a>**LLR-PEH-01** — Signature:
    `void print_error_header(const char *error_type, ParseState *state)`.
    *Trace:* HLR-018 (Error Header Rendering).

*   <a id="LLR-PEH-02"></a>**LLR-PEH-02** — The function shall print
    `"----------------------------------------\n"` to `stdout`.
    *Trace:* HLR-018 (Error Header Rendering).

*   <a id="LLR-PEH-03"></a>**LLR-PEH-03** — The function shall pre-increment `state->error_count`
    and print `"[ERROR #%d] "` using the new value.
    *Trace:* HLR-018 (Error Header Rendering), HLR-038 (Stateful Parsing Context).

*   <a id="LLR-PEH-04"></a>**LLR-PEH-04** — If the last character of `error_type` is `'\n'`, the
    function shall print `error_type` verbatim; otherwise it shall print
    `error_type` followed by `"\n"`.
    *Trace:* HLR-018 (Error Header Rendering), HLR-040 (Non-Fatal Recovery from Malformed Input).

*   <a id="LLR-PEH-05"></a>**LLR-PEH-05** — If `app_config.print_stack` or `app_config.verbose`
    is `true`, the function shall print `"Call Stack:\n"` to `stdout`.
    *Trace:* HLR-012 (Verbose Mode), HLR-024 (Stack Trace Output Gated by Options).

## 10. `print_leak_summary_line` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-PLSL-01"></a>**LLR-PLSL-01** — Signature:
    `void print_leak_summary_line(const char *line, const char *leak_type)`.
    *Trace:* HLR-033 (Leak Detail Formatting).

*   <a id="LLR-PLSL-02"></a>**LLR-PLSL-02** — The function shall attempt
    `sscanf(line, "%*[^:]: %'d %*s %*s %'d", &bytes, &blocks)` (locale
    aware) to extract the `bytes` and `blocks` counts.
    *Trace:* HLR-005 (Locale Initialisation), HLR-033 (Leak Detail Formatting).

*   <a id="LLR-PLSL-03"></a>**LLR-PLSL-03** — On a 2-field match the function shall print
    `"* %s: %d bytes in %d blocks\n"` to `stdout` with `leak_type`,
    `bytes`, and `blocks` substituted.
    *Trace:* HLR-033 (Leak Detail Formatting).

*   <a id="LLR-PLSL-04"></a>**LLR-PLSL-04** — If the parse fails, the function shall print the
    raw `line`, appending a newline only when `line` does not already
    end in `'\n'`.
    *Trace:* HLR-035 (Robust Numeric Parsing), HLR-040 (Non-Fatal Recovery from Malformed Input).

## 11. `print_final_error_summary` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-PFES-01"></a>**LLR-PFES-01** — Signature:
    `void print_final_error_summary(const char *line, ParseState *state)`.
    *Trace:* HLR-034 (Final Error Summary Output).

*   <a id="LLR-PFES-02"></a>**LLR-PFES-02** — The function shall attempt
    `sscanf(line, "%*[^:]: %'d", &error_count)` (locale aware) to read
    Valgrind's reported error total.
    *Trace:* HLR-005 (Locale Initialisation), HLR-034 (Final Error Summary Output).

*   <a id="LLR-PFES-03"></a>**LLR-PFES-03** — On a 1-field match the function shall print, in
    order, `"\n--- FINAL COUNTS ---\n"`,
    `"* Total Errors: %d\n"` (with `state->error_count`), and
    `"* Possible Leaks: %d\n"` (with `error_count - state->error_count`).
    *Trace:* HLR-034 (Final Error Summary Output), HLR-038 (Stateful Parsing Context).

*   <a id="LLR-PFES-04"></a>**LLR-PFES-04** — If the parse fails, the function shall print the
    raw `line`, appending a newline only when `line` does not already
    end in `'\n'`.
    *Trace:* HLR-035 (Robust Numeric Parsing), HLR-040 (Non-Fatal Recovery from Malformed Input).

## 12. `execute_command` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-EC-01"></a>**LLR-EC-01** — Signature:
    `bool execute_command(const char *command, char *output, size_t output_size)`.
    *Trace:* HLR-029 (Multi-Language Source Support), HLR-042 (POSIX Process Helpers), HLR-043 (External `ctags` Dependency Scope).

*   <a id="LLR-EC-02"></a>**LLR-EC-02** — If `command` is `NULL`, `output` is `NULL`, or
    `output_size` is `0`, the function shall return `false` without
    side effects.
    *Trace:* HLR-039 (Defensive Argument Validation).

*   <a id="LLR-EC-03"></a>**LLR-EC-03** — The function shall invoke `popen(command, "r")` to
    obtain a read pipe. If `popen` returns `NULL`, the function shall
    call `perror("popen")` and return `false`.
    *Trace:* HLR-030 (External-Tool Dependency Isolation), HLR-042 (POSIX Process Helpers).

*   <a id="LLR-EC-04"></a>**LLR-EC-04** — The function shall call `fgets(output, output_size, fp)`
    to capture the first line of the command's standard output.
    *Trace:* HLR-029 (Multi-Language Source Support).

*   <a id="LLR-EC-05"></a>**LLR-EC-05** — If `fgets` returns `NULL` and `ferror(fp)` is true,
    the function shall call `perror("fgets")`, call `pclose(fp)`, and
    return `false`.
    *Trace:* HLR-030 (External-Tool Dependency Isolation), HLR-040 (Non-Fatal Recovery from Malformed Input).

*   <a id="LLR-EC-06"></a>**LLR-EC-06** — On all other paths the function shall call
    `pclose(fp)` and return `true`.
    *Trace:* HLR-008 (Resource Management), HLR-029 (Multi-Language Source Support).

## 13. `parse_ctags_output` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-PCO-01"></a>**LLR-PCO-01** — Signature:
    `bool parse_ctags_output(const char *language, char *ctags_output, int *start_line, int *end_line)`.
    *Trace:* HLR-029 (Multi-Language Source Support).

*   <a id="LLR-PCO-02"></a>**LLR-PCO-02** — If any of `language`, `ctags_output`, `start_line`,
    or `end_line` is `NULL`, the function shall write
    `"Error: Invalid arguments to parse_ctags_output.\n"` to `stderr`
    and return `false`.
    *Trace:* HLR-030 (External-Tool Dependency Isolation), HLR-039 (Defensive Argument Validation).

*   <a id="LLR-PCO-03"></a>**LLR-PCO-03** — The function shall split the leading
    tab-delimited fields of `ctags_output` into a function-name token
    and a source-file token (failing with a diagnostic to `stderr` if
    either tab is missing).
    *Trace:* HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation).

*   <a id="LLR-PCO-04"></a>**LLR-PCO-04** — The function shall locate the substring `"line:"`
    in the remaining ctags fields and shall parse the following integer
    via `sscanf(input, "line:%d", start_line)`. A missing `"line:"`
    field shall produce a diagnostic to `stderr` and return `false`.
    *Trace:* HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation).

*   <a id="LLR-PCO-05"></a>**LLR-PCO-05** — The function shall open the source file via
    `fopen(file_name, "r")` and shall read forward until reaching the
    line numbered `*start_line`. If the file cannot be opened or the
    start line cannot be reached, the function shall report the failure
    on `stderr` and return `false`.
    *Trace:* HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation).

*   <a id="LLR-PCO-06"></a>**LLR-PCO-06** — When the detected language is `"C"`, `"C++"`, or
    `"Rust"`, the function shall scan forward from the start line,
    counting `'{'` and `'}'` characters, and shall set `*end_line` to
    the line on which the balanced brace count first returns to zero
    after the first `'{'`.
    *Trace:* HLR-029 (Multi-Language Source Support).

*   <a id="LLR-PCO-07"></a>**LLR-PCO-07** — When the detected language is `"Fortran"`, the
    function shall scan forward from the start line for a
    case-insensitive match of `"END SUBROUTINE <function_name>"` and
    shall set `*end_line` to the line on which the match is found.
    *Trace:* HLR-029 (Multi-Language Source Support).

*   <a id="LLR-PCO-08"></a>**LLR-PCO-08** — Languages other than C, C++, Rust, and Fortran
    shall cause the function to close the source file, write
    `"Note: Unsupported source language '%s'.\n"` to `stderr`, and
    return `false`.
    *Trace:* HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation).

*   <a id="LLR-PCO-09"></a>**LLR-PCO-09** — Before returning `true`, the function shall verify
    that `*start_line > 0`, `*end_line > 0`, and
    `*start_line < *end_line`. Otherwise it shall report the invalid
    range on `stderr` and return `false`.
    *Trace:* HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation).

## 14. `print_source_function` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-PSF-01"></a>**LLR-PSF-01** — Signature:
    `void print_source_function(const char *source_file, const char *function_name, int line_number)`.
    *Trace:* HLR-027 (Conditional Source Body Output), HLR-029 (Multi-Language Source Support).

*   <a id="LLR-PSF-02"></a>**LLR-PSF-02** — If `source_file` is `NULL`, `function_name` is
    `NULL`, or `line_number <= 0`, the function shall write
    `"Error: Invalid arguments to print_source_function.\n"` to
    `stderr` and return.
    *Trace:* HLR-030 (External-Tool Dependency Isolation), HLR-039 (Defensive Argument Validation).

*   <a id="LLR-PSF-03"></a>**LLR-PSF-03** — The function shall first run
    `ctags --print-language <source_file>` via `execute_command` and
    shall extract the language token following the `':'` separator. On
    failure it shall report the failure on `stderr` and return.
    *Trace:* HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation), HLR-043 (External `ctags` Dependency Scope).

*   <a id="LLR-PSF-04"></a>**LLR-PSF-04** — The function shall then run
    `ctags -o - --c-kinds=f --fields=+ne <source_file> | grep '^<function_name>'`
    via `execute_command`. On failure it shall write
    `"Error: Could not find function '%s' in file '%s'.\n"` to `stderr`
    and return.
    *Trace:* HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation).

*   <a id="LLR-PSF-05"></a>**LLR-PSF-05** — The function shall pass the captured language and
    ctags record to `parse_ctags_output`. On failure it shall write
    `"Error: Failed to parse ctags output for function '%s'.\n"` to
    `stderr` and return.
    *Trace:* HLR-029 (Multi-Language Source Support), HLR-030 (External-Tool Dependency Isolation).

*   <a id="LLR-PSF-06"></a>**LLR-PSF-06** — The function shall pre-increment `end_line` by 1
    and shall verify that `start_line > 0`, `end_line > 0`, and
    `start_line <= end_line`, reporting an error on `stderr` and
    returning otherwise.
    *Trace:* HLR-027 (Conditional Source Body Output), HLR-030 (External-Tool Dependency Isolation).

*   <a id="LLR-PSF-07"></a>**LLR-PSF-07** — The function shall open `source_file` via `fopen`
    in read-only mode. On failure it shall call `perror("fopen")` and
    return.
    *Trace:* HLR-027 (Conditional Source Body Output), HLR-030 (External-Tool Dependency Isolation).

*   <a id="LLR-PSF-08"></a>**LLR-PSF-08** — The function shall advance the file to `start_line`
    by reading lines via `fgets` and shall then print successive lines
    until `current_line` exceeds `end_line`. Each printed line shall be
    formatted as `"%c%4d %s"`, where the leading character is `'>'` for
    the offending `line_number` and `' '` otherwise.
    *Trace:* HLR-027 (Conditional Source Body Output), HLR-038 (Stateful Parsing Context).

*   <a id="LLR-PSF-09"></a>**LLR-PSF-09** — The function shall call `fclose` on the source file
    before returning.
    *Trace:* HLR-008 (Resource Management).

## 15. `initialize_parse_state` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-IPS-01"></a>**LLR-IPS-01** — Signature:
    `void initialize_parse_state(ParseState *state)`.
    *Trace:* HLR-038 (Stateful Parsing Context).

*   <a id="LLR-IPS-02"></a>**LLR-IPS-02** — If `state` is `NULL`, the function shall return
    without side effects.
    *Trace:* HLR-039 (Defensive Argument Validation).

*   <a id="LLR-IPS-03"></a>**LLR-IPS-03** — Otherwise the function shall set, in this order:
    `in_error_block = false`, `print_function = false`,
    `stack_lines_shown = 0`, `user_code_found_for_error = false`,
    `current_error_type[0] = '\0'`, `error_filename[0] = '\0'`,
    `error_function_name[0] = '\0'`, `error_line_number = -1`, and
    `error_count = 0`.
    *Trace:* HLR-038 (Stateful Parsing Context).

## 16. `check_start_new_error` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-CSNE-01"></a>**LLR-CSNE-01** — Signature:
    `bool check_start_new_error(const char *line_content, ParseState *state)`.
    *Trace:* HLR-016 (Error Block Detection).

*   <a id="LLR-CSNE-02"></a>**LLR-CSNE-02** — If `line_content` or `state` is `NULL`, the
    function shall return `false`.
    *Trace:* HLR-039 (Defensive Argument Validation).

*   <a id="LLR-CSNE-03"></a>**LLR-CSNE-03** — If `state->in_error_block` is already `true`, the
    function shall return `false` without searching for keywords.
    *Trace:* HLR-016 (Error Block Detection), HLR-038 (Stateful Parsing Context).

*   <a id="LLR-CSNE-04"></a>**LLR-CSNE-04** — The function shall iterate `ERROR_KEYWORDS` and
    test each entry against `line_content` via `strstr`. On the first
    match it shall:
    1.  Call `print_error_header(line_content, state)`.
    2.  Copy `ERROR_KEYWORDS[i]` into `state->current_error_type` via
        `strncpy` bounded by `sizeof(state->current_error_type) - 1`,
        then null-terminate the buffer's last byte.
    3.  Set `state->in_error_block = true`,
        `state->stack_lines_shown = 0`,
        `state->user_code_found_for_error = false`, and
        `state->print_function = false`.
    4.  Return `true`.
    *Trace:* HLR-016 (Error Block Detection), HLR-018 (Error Header Rendering), HLR-038 (Stateful Parsing Context).

*   <a id="LLR-CSNE-05"></a>**LLR-CSNE-05** — If no keyword matches, the function shall return
    `false` without modifying `state`.
    *Trace:* HLR-016 (Error Block Detection).

## 17. `process_stack_trace_line` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-PSTL-01"></a>**LLR-PSTL-01** — Signature:
    `void process_stack_trace_line(const char *line_content, ParseState *state)`.
    *Trace:* HLR-020 (Stack Frame Recognition), HLR-021 (Stack Frame Decoding).

*   <a id="LLR-PSTL-02"></a>**LLR-PSTL-02** — If `line_content` or `state` is `NULL`, the
    function shall return without side effects.
    *Trace:* HLR-039 (Defensive Argument Validation).

*   <a id="LLR-PSTL-03"></a>**LLR-PSTL-03** — The function shall evaluate the predicate
    `state->stack_lines_shown < STACK_TRACE_CONTEXT_LINES ||
    is_user_code_stack_trace(line_content)`.
    *Trace:* HLR-022 (User-Code Classification), HLR-023 (Bounded Stack Trace Output).

*   <a id="LLR-PSTL-04"></a>**LLR-PSTL-04** — When that predicate is true and either
    `app_config.print_stack` or `app_config.verbose` is true, the
    function shall format `line_content` via `get_function_name` into a
    local `MAX_LINE_LENGTH` buffer, print `"  - %s"` of that buffer to
    `stdout`, and increment `state->stack_lines_shown`.
    *Trace:* HLR-012 (Verbose Mode), HLR-021 (Stack Frame Decoding), HLR-024 (Stack Trace Output Gated by Options), HLR-028 (Editor-Jumpable References).

*   <a id="LLR-PSTL-05"></a>**LLR-PSTL-05** — When the predicate is true,
    `state->user_code_found_for_error` is `false`, and
    `is_user_code_stack_trace(line_content)` returns `true`, the
    function shall set `state->user_code_found_for_error = true` and
    shall assign the result of
    `extract_file_and_line(line_content, state->error_filename,
    state->error_function_name, &state->error_line_number)` to
    `state->print_function`.
    *Trace:* HLR-022 (User-Code Classification), HLR-025 (First-User-Frame Capture).

*   <a id="LLR-PSTL-06"></a>**LLR-PSTL-06** — When the predicate is false,
    `state->user_code_found_for_error` is false, and
    `state->stack_lines_shown == STACK_TRACE_CONTEXT_LINES`, and either
    `app_config.print_stack` or `app_config.verbose` is true, the
    function shall print `"  - ...\n"` to `stdout` and increment
    `state->stack_lines_shown` to suppress further ellipses.
    *Trace:* HLR-023 (Bounded Stack Trace Output), HLR-024 (Stack Trace Output Gated by Options).

## 18. `finalize_error_block` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-FEB-01"></a>**LLR-FEB-01** — Signature: `void finalize_error_block(ParseState *state)`.
    *Trace:* HLR-017 (Error Block Termination), HLR-026 (Conditional Source Reference Output).

*   <a id="LLR-FEB-02"></a>**LLR-FEB-02** — If `state` is `NULL`, the function shall return
    without side effects.
    *Trace:* HLR-039 (Defensive Argument Validation).

*   <a id="LLR-FEB-03"></a>**LLR-FEB-03** — When `state->user_code_found_for_error` is `false`
    and `state->stack_lines_shown > 0`, the function shall print
    `"  (-> Check stack trace above for user code related to '%s')\n"`
    to `stdout`, substituting `state->current_error_type`.
    *Trace:* HLR-022 (User-Code Classification), HLR-025 (First-User-Frame Capture).

*   <a id="LLR-FEB-04"></a>**LLR-FEB-04** — When `state->print_function` is `true`, the function
    shall print `"Source (%s:%d)\n"` to `stdout`, substituting
    `state->error_filename` and `state->error_line_number`.
    *Trace:* HLR-026 (Conditional Source Reference Output), HLR-028 (Editor-Jumpable References).

*   <a id="LLR-FEB-05"></a>**LLR-FEB-05** — When `state->print_function` is `true` and either
    `app_config.print_source` or `app_config.verbose` is `true`, the
    function shall call
    `print_source_function(state->error_filename, state->error_function_name, state->error_line_number)`
    and shall print a trailing newline.
    *Trace:* HLR-012 (Verbose Mode), HLR-027 (Conditional Source Body Output), HLR-043 (External `ctags` Dependency Scope).

*   <a id="LLR-FEB-06"></a>**LLR-FEB-06** — Before returning, the function shall set
    `state->in_error_block = false` and `state->print_function = false`.
    *Trace:* HLR-017 (Error Block Termination), HLR-038 (Stateful Parsing Context).

## 19. `process_in_error_block` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-PIEB-01"></a>**LLR-PIEB-01** — Signature:
    `void process_in_error_block(const char *line_content, ParseState *state)`.
    *Trace:* HLR-017 (Error Block Termination), HLR-020 (Stack Frame Recognition).

*   <a id="LLR-PIEB-02"></a>**LLR-PIEB-02** — If `line_content` or `state` is `NULL`, the
    function shall return without side effects.
    *Trace:* HLR-039 (Defensive Argument Validation).

*   <a id="LLR-PIEB-03"></a>**LLR-PIEB-03** — When `line_content` begins with `"   at "` or
    `"   by "` (matched via `strncmp` of the leading 6 characters),
    the function shall call `process_stack_trace_line(line_content, state)`.
    *Trace:* HLR-020 (Stack Frame Recognition), HLR-021 (Stack Frame Decoding).

*   <a id="LLR-PIEB-04"></a>**LLR-PIEB-04** — Otherwise the function shall call
    `finalize_error_block(state)`.
    *Trace:* HLR-017 (Error Block Termination), HLR-026 (Conditional Source Reference Output), HLR-027 (Conditional Source Body Output).

## 20. `process_summary_lines` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-PSL-01"></a>**LLR-PSL-01** — Signature:
    `void process_summary_lines(const char *line_content, ParseState *state)`.
    *Trace:* HLR-031 (Leak Summary Recognition), HLR-034 (Final Error Summary Output).

*   <a id="LLR-PSL-02"></a>**LLR-PSL-02** — If `line_content` is `NULL`, the function shall
    return without side effects.
    *Trace:* HLR-039 (Defensive Argument Validation).

*   <a id="LLR-PSL-03"></a>**LLR-PSL-03** — When `line_content` contains `"LEAK SUMMARY:"` and
    either `app_config.print_leak_summary` or `app_config.verbose` is
    `true`, the function shall print `"\n--- LEAK SUMMARY ---\n"` to
    `stdout`.
    *Trace:* HLR-012 (Verbose Mode), HLR-031 (Leak Summary Recognition), HLR-032 (Leak Summary Output Gated by Options).

*   <a id="LLR-PSL-04"></a>**LLR-PSL-04** — When `line_content` contains one of
    `"definitely lost:"`, `"indirectly lost:"`, `"possibly lost:"`, or
    `"still reachable:"` and either `app_config.print_leak_summary` or
    `app_config.verbose` is `true`, the function shall call
    `print_leak_summary_line(line_content, label)` with `label`
    respectively `"Definitely Lost"`, `"Indirectly Lost"`,
    `"Possibly Lost"`, or `"Still Reachable"`.
    *Trace:* HLR-012 (Verbose Mode), HLR-031 (Leak Summary Recognition), HLR-032 (Leak Summary Output Gated by Options), HLR-033 (Leak Detail Formatting).

*   <a id="LLR-PSL-05"></a>**LLR-PSL-05** — When `line_content` contains `"ERROR SUMMARY:"`
    the function shall call `print_final_error_summary(line_content,
    state)` regardless of any configuration flag.
    *Trace:* HLR-034 (Final Error Summary Output).

## 21. `process_log_file` ([src/vgp.c](../src/vgp.c))

*   <a id="LLR-PLF-01"></a>**LLR-PLF-01** — Signature: `void process_log_file(FILE *file)`.
    *Trace:* HLR-006 (Parser Module Invocation), HLR-036 (Streaming Single-Pass Operation).

*   <a id="LLR-PLF-02"></a>**LLR-PLF-02** — If `file` is `NULL`, the function shall return
    without side effects.
    *Trace:* HLR-039 (Defensive Argument Validation).

*   <a id="LLR-PLF-03"></a>**LLR-PLF-03** — The function shall declare a local `line` buffer of
    `MAX_LINE_LENGTH` bytes and a local `ParseState state`, and shall
    call `initialize_parse_state(&state)`.
    *Trace:* HLR-037 (Bounded Line Buffer), HLR-038 (Stateful Parsing Context).

*   <a id="LLR-PLF-04"></a>**LLR-PLF-04** — The function shall iterate by calling
    `fgets(line, sizeof(line), file)` until EOF.
    *Trace:* HLR-036 (Streaming Single-Pass Operation), HLR-037 (Bounded Line Buffer).

*   <a id="LLR-PLF-05"></a>**LLR-PLF-05** — On each iteration the function shall first call
    `strip_valgrind_pid_prefix(line)` and shall use the returned
    pointer as `line_content` for all subsequent dispatch.
    *Trace:* HLR-014 (PID-Prefix Stripping).

*   <a id="LLR-PLF-06"></a>**LLR-PLF-06** — When `state.in_error_block` is `false` and the
    `line_content` consists only of characters in `" \t\n\r"`
    (`strspn` equal to `strlen`), the iteration shall `continue` to the
    next line.
    *Trace:* HLR-015 (Whitespace Skipping Outside Error Blocks).

*   <a id="LLR-PLF-07"></a>**LLR-PLF-07** — When `state.in_error_block` is `true`, the function
    shall call `process_in_error_block(line_content, &state)`. After
    that call the function shall continue with the same iteration so
    that a finalised block's terminating line can also be evaluated by
    the new-error and summary paths.
    *Trace:* HLR-017 (Error Block Termination), HLR-020 (Stack Frame Recognition).

*   <a id="LLR-PLF-08"></a>**LLR-PLF-08** — When `state.in_error_block` is `false`, the function
    shall call `check_start_new_error(line_content, &state)`. If it
    returns `true`, the iteration shall `continue` to the next line.
    *Trace:* HLR-016 (Error Block Detection), HLR-018 (Error Header Rendering).

*   <a id="LLR-PLF-09"></a>**LLR-PLF-09** — When `state.in_error_block` is `false` and
    `check_start_new_error` returned `false`, the function shall call
    `process_summary_lines(line_content, &state)`.
    *Trace:* HLR-031 (Leak Summary Recognition), HLR-034 (Final Error Summary Output).

*   <a id="LLR-PLF-10"></a>**LLR-PLF-10** — After the read loop terminates, if
    `state.in_error_block` is `true`, the function shall call
    `finalize_error_block(&state)` and shall print
    `"----------------------------------------\n\n"` to `stdout`.
    *Trace:* HLR-019 (End-of-Log Error-Block Finalisation), HLR-026 (Conditional Source Reference Output).
