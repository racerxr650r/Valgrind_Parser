# Software Design Document: Valgrind Parser (vgp)

**Version:** 2.0
**Date:** 4/21/2026
**Author(s):** John Anderson

## 1. Introduction

### 1.1 Purpose of the Document
This document provides a detailed design for the `vgp` (Valgrind Parser)
command-line application. It is intended for developers, testers, and maintainers
of the `vgp` software.

### 1.2 Scope of the Document
This document describes the design of the source modules that make up the
shipped binary:

*   [src/main.c](../src/main.c): Application entry point, command-line
    argument parsing, and top-level orchestration.
*   [src/vgp.c](../src/vgp.c): Streaming Valgrind log parser, output
    formatter, and source-code extraction logic.
*   [inc/vgp.h](../inc/vgp.h): Public types, constants, and function
    prototypes shared between the modules above and the unit-test harness.

It does not exhaustively describe the unit/integration test harnesses under
[test/](../test/) or build mechanics in the [Makefile](../Makefile), though
both are referenced where they influence the design.

### 1.3 Project Overview
The Valgrind Parser (`vgp`) is a command-line utility that converts the text
output of Valgrind's Memcheck tool into a more compact, human-readable form.
It strips Valgrind's PID prefix and bookkeeping noise, summarises errors and
leaks, optionally prints the call stack filtered to user code, and optionally
prints the offending function's source with the failing line highlighted.

`vgp` is a streaming, single-pass tool. It reads the log file line by line
and emits formatted output to `stdout` as it goes. It does not build an
in-memory representation of all parsed errors.

### 1.4 Definitions, Acronyms, and Abbreviations
*   **SDD:** Software Design Document
*   **VGP:** Valgrind Parser (the application itself)
*   **Valgrind:** A suite of tools for memory debugging, memory leak
    detection, and profiling. `vgp` targets the Memcheck tool's text output.
*   **CLI:** Command-Line Interface
*   **PID:** Process Identifier (used by Valgrind in its `==<PID>==` line
    prefix).
*   **ctags:** Universal Ctags — used at runtime to locate function
    definitions in source files.

### 1.5 References
*   Valgrind User Manual: <https://valgrind.org/docs/manual/manual.html>
*   Universal Ctags: <https://github.com/universal-ctags/ctags>
*   Project [Makefile](../Makefile)
*   [High Level Requirements](HLRs.md)
*   [Low Level Requirements](LLRs.md)
*   Man page: [doc/vgp.1](vgp.1)

### 1.6 Document Overview
*   Section 1: Introduction.
*   Section 2: System Overview.
*   Section 3: Detailed design for [src/main.c](../src/main.c).
*   Section 4: Detailed design for [src/vgp.c](../src/vgp.c).
*   Section 5: Data Dictionary.
*   Section 6: Traceability.

## 2. System Overview

### 2.1 System Architecture
`vgp` is a single executable composed of two C translation units that share
the [inc/vgp.h](../inc/vgp.h) header:

*   **[src/main.c](../src/main.c)** — Acts as the controller. It parses
    command-line arguments into a global `AppConfig`, opens the input log
    file, and invokes the parsing engine.
*   **[src/vgp.c](../src/vgp.c)** — Implements the streaming parser. It
    reads the log line by line, drives a small `ParseState` state machine,
    formats output, and shells out to `ctags` to locate source functions
    when source listing is requested.

The runtime data flow is:

1.  `main()` parses argv into the global `app_config` (`AppConfig`).
2.  `main()` opens the log file and calls `process_log_file(FILE*)`.
3.  `process_log_file()` reads each line, strips the Valgrind PID prefix,
    and dispatches it through `check_start_new_error()`,
    `process_in_error_block()`, and `process_summary_lines()`.
4.  Within an error block, stack frames are filtered to user code via
    `is_user_code_stack_trace()` and printed via `get_function_name()`.
5.  When the block ends, `finalize_error_block()` optionally invokes
    `print_source_function()`, which uses `ctags` (via `popen()`) to find
    the function's start line in the source file and prints the function
    body with the failing line marked.
6.  Leak and error summaries are formatted by `print_leak_summary_line()`
    and `print_final_error_summary()`.

### 2.2 Design Goals and Constraints
*   **Streaming:** Process logs line-by-line with bounded memory; no full
    in-memory model of the log is built.
*   **Zero runtime configuration:** No config files, no environment
    variables; behaviour is fully driven by command-line flags.
*   **Minimal dependencies:** Only the C standard library plus POSIX
    `popen()`/`pclose()` (gated by `_POSIX_C_SOURCE` / `_GNU_SOURCE`) and
    an external `ctags` binary on `PATH` for the optional source-listing
    feature.
*   **Multi-language source support:** Source listing supports C, C++,
    Rust, and Fortran user code; recognised file extensions are listed in
    `USER_CODE_EXTENSIONS`.
*   **Editor-friendly output:** Source references are printed as
    `file:line` so editors such as VS Code can jump to the location via
    Ctrl+Click in the terminal.
*   **Portability:** Targeted at Linux (the project is built and tested
    against gcc, gcovr, cppcheck, cmocka, and Universal Ctags on Linux).
*   **Testability:** All non-trivial helpers are exposed via
    [inc/vgp.h](../inc/vgp.h) so the cmocka unit tests under
    [test/unit/](../test/unit/) can exercise them directly.

## 3. Detailed Design for [src/main.c](../src/main.c)

### 3.1 Purpose and Responsibilities
[src/main.c](../src/main.c) is the entry point for the `vgp` executable.
Its responsibilities are:

*   Define `main()`.
*   Parse command-line arguments into the global `app_config`.
*   Print usage information for `-h` and on argument errors.
*   Open the input Valgrind log file and report I/O errors.
*   Set the locale so that summary numbers can be parsed/formatted with
    thousands grouping (`setlocale(LC_NUMERIC, "")`).
*   Hand the open `FILE*` to `process_log_file()` in `vgp.c`.
*   Close the file and return an appropriate exit code.

### 3.2 External Interfaces
#### 3.2.1 Command-Line Arguments

Synopsis: `vgp [OPTIONS] <FILE>`

| Option | Effect (mapped to `AppConfig` field) |
| ------ | ------------------------------------ |
| `-v`   | `verbose = true` — enables `-s`, `-l`, and `-t` behaviour. |
| `-s`   | `print_source = true` — print source of offending function. |
| `-l`   | `print_leak_summary = true` — print the leak summary block. |
| `-t`   | `print_stack = true` — print the (filtered) call stack. |
| `-h`   | Print usage to `stdout` and `exit(EXIT_SUCCESS)`. |
| `<FILE>` | Positional argument — path to the Valgrind log file. Required. |

Any unknown option, a missing file argument, or more than one positional
argument causes an error message to `stderr` and `exit(EXIT_FAILURE)`.

#### 3.2.2 File System

*   Reads the input log file from the path provided on the command line.
*   Writes formatted output to `stdout`. There is no `-o` option; users
    redirect with the shell (e.g. `vgp log.txt > out.txt`).
*   Indirectly invokes the external `ctags` binary on `PATH` when source
    listing is enabled (see Section 4.3.2).

#### 3.2.3 Standard I/O

*   `stdout`: All formatted parser output.
*   `stderr`: Usage errors, file-open failures, and internal warnings from
    the source-listing helpers.


### 3.3 Internal Structure
#### 3.3.1 Key Data Structures

`main.c` uses the global `AppConfig` defined in [inc/vgp.h](../inc/vgp.h)
and instantiated in [src/vgp.c](../src/vgp.c). See Section 5.


#### 3.3.2 Key Functions

*   **`void parse_command_line(int argc, char *argv[])`**
    *   Purpose: Walk `argv` and populate the global `app_config`.
    *   Logic:
        1.  Iterate `argv[1..argc-1]`.
        2.  For each token starting with `-`, dispatch on `argv[i][1]`
            (`v`/`s`/`l`/`t`/`h`) and set the corresponding flag in
            `app_config`. `-h` prints usage and exits with success.
        3.  Any other leading character causes an error and exit.
        4.  Otherwise treat the token as the log-file path. The first
            positional argument is stored in `app_config.log_file`; a
            second positional argument is an error.
        5.  After the loop, a missing `log_file` is an error.
    *   Notes: A hand-rolled loop is used rather than `getopt()`; only
        single-character options are supported and they cannot be bundled.

*   **`int main(int argc, char *argv[])`**
    *   Purpose: Application entry point.
    *   Return Value: `EXIT_SUCCESS` on success, `EXIT_FAILURE` if the
        input file cannot be opened. Argument errors exit from
        `parse_command_line()`.
    *   Logic:
        1.  `setlocale(LC_NUMERIC, "")` so `scanf("%'d", ...)` accepts
            grouped numbers in the locale's format.
        2.  Call `parse_command_line(argc, argv)`.
        3.  `fopen(app_config.log_file, "r")`. On failure, print
            `strerror(errno)` to `stderr` and return `EXIT_FAILURE`.
        4.  Print a one-line `"Parsing Valgrind Log File: <path>"`
            banner to `stdout`.
        5.  Call `process_log_file(file)`.
        6.  `fclose(file)` and return `EXIT_SUCCESS`.

### 3.4 Dependencies

*   Standard C: `stdio.h`, `stdlib.h`, `string.h`, `errno.h`, `locale.h`
    (transitively via [inc/vgp.h](../inc/vgp.h)).
*   [inc/vgp.h](../inc/vgp.h) for `AppConfig`, `app_config`, and
    `process_log_file()`.

### 3.5 Error Handling and Logging

*   **Argument errors:** Printed to `stderr`, immediate `exit(EXIT_FAILURE)`.
*   **File open errors:** Printed to `stderr` with `strerror(errno)`,
    return `EXIT_FAILURE` from `main()`.
*   **No internal logging framework.** Verbose mode (`-v`) is implemented
    by the parser turning on the other display flags, not by a separate
    log channel.

## 4. Detailed Design for [src/vgp.c](../src/vgp.c)

### 4.1 Purpose and Responsibilities
[src/vgp.c](../src/vgp.c) contains the parsing engine, output formatter,
and source-listing helper. Its responsibilities are:

*   Define and initialise the global `app_config` (`AppConfig`) and the
    global recognition tables (`USER_CODE_EXTENSIONS`, `IGNORE_PATHS`,
    `ERROR_KEYWORDS`).
*   Strip Valgrind's `==<PID>==` prefix from each line.
*   Detect the start of an error block via keyword matching.
*   Filter stack-trace frames to user code, suppressing system frames once
    a configurable context limit has been printed.
*   Detect and format the leak summary and final error summary.
*   On request, locate the source file and function for an error and print
    the function body with the offending line marked.

### 4.2 External Interfaces (declared in [inc/vgp.h](../inc/vgp.h))
The header exposes a wide surface area mainly to support unit testing.
The functions intended as the public entry points of the module are:

*   **`void process_log_file(FILE *file)`**
    *   Purpose: Drive the streaming parse of an open Valgrind log.
    *   Pre-condition: `file` is non-`NULL` and open for reading.
    *   Post-condition: All output for the log has been written to
        `stdout`. The function returns once EOF is reached. Any error
        block still active at EOF is finalised before return.
    *   Return Value: `void`. Parse errors do not abort processing; the
        function attempts to render whatever it can recognise.

*   **`void initialize_parse_state(ParseState *state)`**
    *   Purpose: Zero a `ParseState` instance prior to parsing.
    *   Pre-condition: `state` may be `NULL` (the function is a no-op).

The remaining declarations in [inc/vgp.h](../inc/vgp.h) (line-level
helpers, summary printers, source-listing helpers) are exposed only for
the cmocka test harnesses under [test/unit/](../test/unit/) and should be
treated as internal to the module.


### 4.3 Internal Structure
#### 4.3.1 Key Data Structures


*   **`AppConfig`** — see Section 5.
*   **`ParseState`** — see Section 5. A single `ParseState` is allocated on
    the stack in `process_log_file()` and threaded through the per-line
    helpers.
*   **Static recognition tables** (file scope, `NULL`-terminated arrays of
    string literals):
    *   `USER_CODE_EXTENSIONS` — file extensions considered "user code"
        (`.c`, `.cpp`, `.h`, `.hpp`, `.cc`, `.hh`, `.cxx`, `.hxx`,
        `.f90`, `.f`, `.F`, `.for`, `.ada`, `.ads`, `.adb`, `.rs`, and
        their uppercase variants).
    *   `IGNORE_PATHS` — path fragments that disqualify a frame even when
        the extension matches (`/usr/`, `/lib/`, `vg_`).
    *   `ERROR_KEYWORDS` — substrings whose presence on a non-stack line
        starts a new error block (e.g. `"Invalid read"`, `"Invalid write"`,
        `"depends on uninitialised value"`, `"Invalid free"`,
        `"Mismatched free"`, `"Source and destination overlap"`,
        `"Invalid usage of address"`).


#### 4.3.2 Key Functions

Functions are grouped by role; all are defined in [src/vgp.c](../src/vgp.c).

**Line-level utilities**

*   **`char *strip_valgrind_pid_prefix(char *line)`** — Locates the
    `"==<digits>=="` prefix, skips it (and the following space), and
    returns a pointer into the original buffer. Returns the original
    pointer if the prefix is not present.
*   **`bool is_valid_function_char(char c)`** — Predicate used when
    sanitising parsed function names.
*   **`bool is_user_code_stack_trace(const char *line)`** — Returns `true`
    iff the line is a Valgrind stack frame (`"   at "` / `"   by "`) that
    references a file whose extension is in `USER_CODE_EXTENSIONS` and
    whose path does not match any entry in `IGNORE_PATHS`.
*   **`char *get_function_name(const char *line, char *newline)`** — Parses a stack-frame line into `function(basename:line)` form using
    `sscanf` and `basename(3)`, writing the result into the caller-owned
    buffer `newline`.
*   **`bool extract_file_and_line(const char *line, char *filename,
    char *function_name, int *line_number)`** — Tries three `sscanf`
    patterns to extract `(filename, function, line)` from a stack frame.
    Falls back to "no line number" or "no function" forms. Output buffers
    are assumed to be `MAX_LINE_LENGTH` bytes.
**State machine**

*   **`void initialize_parse_state(ParseState *state)`** — Zero/reset all
    fields.
*   **`bool check_start_new_error(const char *line, ParseState *state)`** — If not currently in an error block and the line contains any
    `ERROR_KEYWORDS` substring, emits the error header via
    `print_error_header()`, records the matched keyword in
    `state->current_error_type`, and sets `state->in_error_block`.
*   **`void process_stack_trace_line(const char *line, ParseState *state)`** — Handles a `"   at "` / `"   by "` line. Honours the
    `STACK_TRACE_CONTEXT_LINES` (= 20) budget for non-user frames. The
    first user-code frame for each error captures `(filename, function,
    line_number)` into `state` and sets `print_function = true`.
*   **`void process_in_error_block(const char *line, ParseState *state)`** — Dispatches stack-trace lines to `process_stack_trace_line()`; any
    other line ends the block and triggers `finalize_error_block()`.
*   **`void finalize_error_block(ParseState *state)`** — When a block ends,
    optionally prints the captured `Source (file:line)` reference and, if
    `-s` or `-v` is in effect, calls `print_source_function()`. Resets the
    "in block" flags so the next error can be detected.
*   **`void process_summary_lines(const char *line, ParseState *state)`** — Recognises `"LEAK SUMMARY:"`, the four leak categories
    (`definitely lost`, `indirectly lost`, `possibly lost`,
    `still reachable`), and `"ERROR SUMMARY:"`. Leak lines are gated on
    `-l`/`-v`; the final error summary is always printed.
*   **`void process_log_file(FILE *file)`** — The driver loop. For each
    line it:    1.  Strips the PID prefix.
    2.  Skips whitespace-only lines unless inside an error block.
    3.  If inside an error block, calls `process_in_error_block()`.
    4.  If (now) outside a block, calls `check_start_new_error()`; if
        that did not start a block, calls `process_summary_lines()`.
    5.  After EOF, finalises any still-open error block.

**Output formatting**

*   **`void print_error_header(const char *error_type, ParseState *state)`** — Prints the `--------` separator, increments `error_count`, and prints
    `[ERROR #N] <error_type>`. Optionally prints a `Call Stack:` heading
    when stack output is enabled.
*   **`void print_leak_summary_line(const char *line, const char *type)`** — Parses `"...: <bytes> ... <blocks>"` with `scanf("%'d")` (locale-aware)
    and prints `"* <type>: <bytes> bytes in <blocks> blocks"`. Falls back
    to printing the raw line if parsing fails.
*   **`void print_final_error_summary(const char *line, ParseState *state)`** — Prints the `--- FINAL COUNTS ---` block with `Total Errors` (taken
    from `state->error_count`) and `Possible Leaks` (the difference
    between Valgrind's reported error total and the errors `vgp`
    classified).
**Source-code extraction**

*   **`bool execute_command(const char *command, char *output, size_t size)`** — Thin wrapper around `popen()` / `fgets()` / `pclose()` that captures
    the first line of stdout from a shell command. Used to invoke `ctags`.
*   **`bool parse_ctags_output(const char *language, char *ctags_output,
    int *start_line, int *end_line)`** — Splits a tab-delimited ctags
    record into function name, file name, and `line:` field, then scans
    the source file from `start_line` forward to find the function's
    closing point. End-of-function detection is brace-matched for C, C++,
    and Rust, and `END SUBROUTINE <name>` matched for Fortran. Other
    languages return `false`.
*   **`void print_source_function(const char *source_file,
    const char *function_name, int line_number)`** — Two-step ctags
    invocation:    1.  `ctags --print-language <file>` to determine the language.
    2.  `ctags -o - --c-kinds=f --fields=+ne <file> | grep '^<function>'`
        to find the function's start line.
    The function then opens the source file, advances to the start line,
    and prints each line of the function body prefixed with a 4-digit
    line number, marking the offending line with `>`.


#### 4.3.3 Parsing Strategy / Algorithm

*   **Streaming, single pass.** The log is read line by line via
    `fgets()` into a fixed `MAX_LINE_LENGTH` (= 2048) buffer; no
    accumulation across lines.
*   **State machine.** `ParseState` carries the small amount of context
    needed across lines: whether we are currently inside an error block,
    how many stack frames have been printed, whether the first user-code
    frame for the current error has been captured, and the running error
    count.
*   **Pattern recognition.** Pure C-string operations only — `strstr`,
    `strncmp`, `strspn`/`strcspn`, and a small number of `sscanf` format
    strings. No regex library is used.
*   **User-code filtering.** The `USER_CODE_EXTENSIONS` allow-list and
    `IGNORE_PATHS` deny-list together classify stack frames. Up to
    `STACK_TRACE_CONTEXT_LINES` (= 20) non-user frames may still be
    printed for context; after that, an ellipsis is printed once and
    further system frames are suppressed until a user frame appears.
*   **Source listing.** Instead of parsing source code itself, `vgp`
    delegates symbol lookup to `ctags` and uses simple language-specific
    rules to find the end of the function.

### 4.4 Dependencies

*   Standard C: `stdio.h`, `stdlib.h`, `string.h`, `stdbool.h`, `ctype.h`,
    `errno.h`, `libgen.h` (for `basename`), `locale.h`.
*   POSIX: `popen()` / `pclose()` (gated by `_POSIX_C_SOURCE >= 2` and
    `_GNU_SOURCE` defined in [inc/vgp.h](../inc/vgp.h)).
*   External binary: Universal Ctags, available as `ctags` on `PATH`.
    Required only when `-s` or `-v` is in effect.

### 4.5 Error Handling and Logging

*   **NULL-pointer guards.** Every public helper validates its pointer
    arguments and returns early on `NULL`.
*   **Fixed-size buffers.** The parser uses caller-owned buffers sized to
    `MAX_LINE_LENGTH`; truncation is tolerated rather than fatal.
*   **Malformed log lines.** Summary printers fall back to printing the
    raw line when their `sscanf` pattern fails to match, so unrecognised
    variations do not silently disappear.
*   **`ctags` / source-file errors.** `print_source_function()` and its
    helpers emit a diagnostic to `stderr` and return without printing
    source. The surrounding error block is otherwise rendered normally.
*   **No verbose log channel.** `-v` is shorthand for "enable all output
    sections"; it does not enable additional diagnostic logging.
## 5. Data Dictionary

*   **`AppConfig`** (defined in [inc/vgp.h](../inc/vgp.h), instantiated as
    the global `app_config` in [src/vgp.c](../src/vgp.c)) — Application configuration populated from the command line.

    | Field | Type | Description |
    | ----- | ---- | ----------- |
| `verbose` | `bool` | `-v`: enable all optional output sections. |
| `print_source` | `bool` | `-s`: include the offending function source. |
| `print_stack` | `bool` | `-t`: include the filtered call stack. |
| `print_leak_summary` | `bool` | `-l`: include the leak summary. |
| `log_file` | `char *` | Path to the Valgrind log file (positional arg). |
*   **`ParseState`** (defined in [inc/vgp.h](../inc/vgp.h)) — Per-parse state for the streaming line processor.

    | Field | Type | Description |
    | ----- | ---- | ----------- |
| `in_error_block` | `bool` | True between the error header and the line that ends the stack trace. |
| `print_function` | `bool` | True if a user-code frame for the current error was successfully decoded and source should be printed at finalisation. |
| `stack_lines_shown` | `int` | Number of stack frames printed for the current error (used to enforce `STACK_TRACE_CONTEXT_LINES`). |
| `user_code_found_for_error` | `bool` | True once the first user-code frame has been captured. |
| `current_error_type` | `char[MAX_ERROR_TYPE_LENGTH]` | Matched `ERROR_KEYWORDS` entry for the current error. |
| `error_filename` | `char[MAX_LINE_LENGTH]` | Source file of the first user-code frame. |
| `error_function_name` | `char[MAX_LINE_LENGTH]` | Function name of the first user-code frame. |
| `error_line_number` | `int` | Line number within `error_filename`. |
| `error_count` | `int` | Running count of errors emitted, displayed in the final summary. |
*   **Compile-time constants** (in [inc/vgp.h](../inc/vgp.h)):

    | Name | Value | Purpose |
    | ---- | ----- | ------- |
| `MAX_LINE_LENGTH` | 2048 | `fgets` buffer and most output buffers. |
| `STACK_TRACE_CONTEXT_LINES` | 20 | Non-user frames printed before suppression. |
| `MAX_ERROR_TYPE_LENGTH` | 2048 | Sizing of `ParseState.current_error_type`. |
| `MAX_SOURCE_LINE_LENGTH` | 2048 | Sizing of source-listing buffers. |
*   **Recognition tables** (file-scope, in [src/vgp.c](../src/vgp.c)):
    `USER_CODE_EXTENSIONS`, `IGNORE_PATHS`, `ERROR_KEYWORDS` — described
    in Section 4.3.1.
## 6. Traceability

The following table maps the high-level requirements in
[doc/HLRs.md](HLRs.md) and the low-level requirements in
[doc/LLRs.md](LLRs.md) to the design elements above. (Requirement IDs
should be reconciled against the latest revisions of those documents.)

| Requirement Theme | Design Section(s) |
| ----------------- | ----------------- |
| Parse Valgrind log into human-readable form | §4 (entire) |
| Strip PID/address noise | §4.3.2 `strip_valgrind_pid_prefix` |
| Identify error blocks | §4.3.2 `check_start_new_error`, `ERROR_KEYWORDS` |
| Filter stack to user code | §4.3.2 `is_user_code_stack_trace`, `process_stack_trace_line` |
| Print offending source (`-s`) | §4.3.2 `print_source_function`, `parse_ctags_output` |
| Print call stack (`-t`) | §4.3.2 `process_stack_trace_line`, `print_error_header` |
| Print leak summary (`-l`) | §4.3.2 `process_summary_lines`, `print_leak_summary_line` |
| Verbose mode (`-v`) | §3.3.2 `parse_command_line`; §4.3.2 (gating checks) |
| Final error/leak counts | §4.3.2 `print_final_error_summary` |
| Editor-jumpable file references | §4.3.2 `finalize_error_block` (`Source (file:line)`) |
| Multi-language source support | §4.3.1 `USER_CODE_EXTENSIONS`; §4.3.2 `parse_ctags_output` |
---
