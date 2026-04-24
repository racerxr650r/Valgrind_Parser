# High-Level Requirements (HLRs) for Valgrind Parser (vgp)

**Version:** 2.0
**Date:** 4/21/2026
**Author(s):** John Anderson

## 1. Introduction

This document defines the high-level requirements (HLRs) for the
`vgp` (Valgrind Parser) command-line application. Each requirement is
written so that it can be traced to:

*   One or more sections of the [Software Design Document](SDD.md).
*   One or more low-level requirements in [LLRs.md](LLRs.md) (to be added
    when LLRs are revised).
*   One or more verification artefacts under [test/](../test/).

Requirement IDs use the prefix `HLR-` followed by a three-digit number.
IDs are stable: existing IDs must not be reassigned. New requirements
take the next unused number.

### 1.1 Scope
The requirements cover the shipped `vgp` executable, comprised of
[src/main.c](../src/main.c) and [src/vgp.c](../src/vgp.c) with the shared
header [inc/vgp.h](../inc/vgp.h). They do not constrain the build system,
the test harnesses, or the documentation tooling.

### 1.2 Terminology
*   **Shall** — mandatory requirement.
*   **Should** — recommended behaviour; deviations require justification.
*   **User code** — source files whose extension appears in the
    `USER_CODE_EXTENSIONS` table and whose path does not match
    `IGNORE_PATHS` (see [SDD.md §4.3.1](SDD.md#431-key-data-structures)).
*   **Error block** — a contiguous run of Valgrind log lines starting with
    a recognised error keyword and ending at the next non-stack-trace
    line.

## 2. Command-Line Interface and Application Lifecycle

These requirements cover the entry point, argument handling, file I/O
orchestration, and process exit behaviour. They are realised primarily in
[src/main.c](../src/main.c).

*   <a id="HLR-001"></a>**HLR-001: Command-Line Interface Provision.**
    The application shall provide a command-line interface that accepts a
    single positional argument (the Valgrind log file path) and a set of
    single-character option flags.
    *Trace:* [SDD.md §3.2.1](SDD.md).

*   <a id="HLR-002"></a>**HLR-002: Argument Parsing and Validation.**
    The application shall parse `argv` and shall:
    *   Accept the option flags `-v`, `-s`, `-l`, `-t`, and `-h`.
    *   Accept exactly one positional argument that is the input log file
        path.
    *   Reject unknown option flags.
    *   Reject invocations with no positional argument or with more than
        one positional argument.
    *Trace:* [SDD.md §3.2.1](SDD.md), [SDD.md §3.3.2](SDD.md).

*   <a id="HLR-003"></a>**HLR-003: Usage and Help Information Display.**
    When the `-h` flag is supplied, the application shall print a usage
    summary listing every supported option and shall terminate with
    success. Argument validation failures shall print a diagnostic to
    `stderr`.
    *Trace:* [SDD.md §3.2.1](SDD.md), [SDD.md §3.5](SDD.md).

*   <a id="HLR-004"></a>**HLR-004: Input Log File Handling.**
    The application shall open the user-supplied log file in read-only
    text mode (`"r"`). On failure, it shall report the path and the
    underlying system error (via `strerror(errno)`) to `stderr` and
    terminate with a failure status.
    *Trace:* [SDD.md §3.2.2](SDD.md), [SDD.md §3.3.2](SDD.md), [SDD.md §3.5](SDD.md).

*   <a id="HLR-005"></a>**HLR-005: Locale Initialisation.**
    Before parsing begins, the application shall initialise the numeric
    locale (`setlocale(LC_NUMERIC, "")`) so that grouped numeric values
    in Valgrind summary lines can be parsed correctly in the user's
    locale.
    *Trace:* [SDD.md §3.3.2](SDD.md), [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-006"></a>**HLR-006: Parser Module Invocation.**
    The application shall hand the open log file stream to the parser
    entry point `process_log_file(FILE *)` defined in
    [src/vgp.c](../src/vgp.c). The parser shall be the sole consumer of
    the stream after this call.
    *Trace:* [SDD.md §2.1](SDD.md), [SDD.md §3.3.2](SDD.md), [SDD.md §4.2](SDD.md).

*   <a id="HLR-007"></a>**HLR-007: Initial User Feedback.**
    The application shall write a banner line to `stdout` identifying the
    Valgrind log file being processed before any parser output is
    emitted.
    *Trace:* [SDD.md §3.3.2](SDD.md).

*   <a id="HLR-008"></a>**HLR-008: Resource Management.**
    The application shall close the input log file (via `fclose`) before
    returning from `main()`. No other file descriptors shall remain open
    at process exit.
    *Trace:* [SDD.md §3.3.2](SDD.md).

*   <a id="HLR-009"></a>**HLR-009: Application Exit Status.**
    The application shall terminate with `EXIT_SUCCESS` when parsing
    completes without an unrecoverable error and with `EXIT_FAILURE` when
    argument validation fails or the input log file cannot be opened.
    *Trace:* [SDD.md §3.3.2](SDD.md), [SDD.md §3.5](SDD.md).

*   <a id="HLR-010"></a>**HLR-010: Application-Level Error Reporting.**
    All operational error messages emitted by the application layer shall
    be written to `stderr` and shall be self-describing (identifying the
    failed operation and the affected resource). Routine progress and
    parser results shall be written to `stdout`.
    *Trace:* [SDD.md §3.2.3](SDD.md), [SDD.md §3.5](SDD.md).

## 3. Configuration and Output Modes

These requirements cover how command-line options shape the parser's
output. The configuration is held in the global `AppConfig`
(see [SDD.md §5](SDD.md#5-data-dictionary)).

*   <a id="HLR-011"></a>**HLR-011: Configurable Output Sections.**
    The application shall allow the user to independently enable the
    following optional output sections:
    *   Filtered call stack for each error (`-t`).
    *   Source listing for the offending function (`-s`).
    *   Leak summary (`-l`).
    *Trace:* [SDD.md §3.2.1](SDD.md), [SDD.md §5](SDD.md).

*   <a id="HLR-012"></a>**HLR-012: Verbose Mode.**
    When `-v` is supplied, the application shall behave as if `-s`, `-l`,
    and `-t` had also been supplied. The error summary at end-of-log
    shall always be printed regardless of the verbose flag.
    *Trace:* [SDD.md §3.2.1](SDD.md), [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-013"></a>**HLR-013: No Hidden Configuration.**
    The application shall not read configuration from environment
    variables or configuration files. All behaviour shall be
    deterministic given `argv` and the contents of the input log file.
    *Trace:* [SDD.md §2.2](SDD.md).

## 4. Log Pre-processing

These requirements cover normalisation of raw Valgrind log lines before
they are interpreted.

*   <a id="HLR-014"></a>**HLR-014: PID-Prefix Stripping.**
    For each input line, the parser shall remove the leading
    `==<digits>== ` prefix produced by Valgrind. Lines without such a
    prefix shall be processed unchanged.
    *Trace:* [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-015"></a>**HLR-015: Whitespace Skipping Outside Error Blocks.**
    Lines consisting only of whitespace shall be skipped while the parser
    is not inside an error block. Whitespace lines encountered while
    inside an error block may signal the end of that block.
    *Trace:* [SDD.md §4.3.2](SDD.md).

## 5. Error-Block Detection and Rendering

These requirements cover identification and human-readable rendering of
Valgrind error reports.

*   <a id="HLR-016"></a>**HLR-016: Error Block Detection.**
    The parser shall recognise the start of an error block when a
    non-stack-trace line contains any keyword from the
    `ERROR_KEYWORDS` recognition table.
    *Trace:* [SDD.md §4.3.1](SDD.md), [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-017"></a>**HLR-017: Error Block Termination.**
    The parser shall treat the first line that is not part of the
    expected stack-trace pattern (`"   at "` or `"   by "`) as the end of
    the current error block, and shall finalise that block before
    processing the line further.
    *Trace:* [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-018"></a>**HLR-018: Error Header Rendering.**
    When a new error block is detected, the parser shall emit a separator
    line followed by an `[ERROR #N] <error type>` header, where `N` is a
    monotonically increasing counter starting at 1.
    *Trace:* [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-019"></a>**HLR-019: End-of-Log Error-Block Finalisation.**
    If the input ends while an error block is still active, the parser
    shall finalise that block (emitting any pending source listing) and
    print a closing separator before returning.
    *Trace:* [SDD.md §4.3.2](SDD.md).

## 6. Stack-Trace Handling

These requirements cover parsing, filtering, and rendering of the per-error
stack trace.

*   <a id="HLR-020"></a>**HLR-020: Stack Frame Recognition.**
    The parser shall recognise lines beginning with `"   at "` or
    `"   by "` (after PID-prefix stripping) as stack-trace frames.
    *Trace:* [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-021"></a>**HLR-021: Stack Frame Decoding.**
    For each stack-trace frame the parser shall attempt to decode the
    function name, source file basename, and source line number, and
    shall render them in the form `function(basename:line)`.
    *Trace:* [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-022"></a>**HLR-022: User-Code Classification.**
    A stack frame shall be classified as user code only if its source file
    extension is listed in `USER_CODE_EXTENSIONS` and the frame's path
    does not match any entry in `IGNORE_PATHS`.
    *Trace:* [SDD.md §4.3.1](SDD.md), [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-023"></a>**HLR-023: Bounded Stack Trace Output.**
    The parser shall print at most `STACK_TRACE_CONTEXT_LINES` non-user
    frames per error block. Once that budget is exhausted before a
    user-code frame has been seen, the parser shall print a single
    ellipsis line and shall suppress further non-user frames for the same
    error.
    *Trace:* [SDD.md §4.3.2](SDD.md), [SDD.md §4.3.3](SDD.md), [SDD.md §5](SDD.md).

*   <a id="HLR-024"></a>**HLR-024: Stack Trace Output Gated by Options.**
    Per-frame stack output shall be emitted only when `-t` or `-v` is in
    effect. The user-code classification used to drive source listing
    shall be performed regardless of these flags.
    *Trace:* [SDD.md §3.2.1](SDD.md), [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-025"></a>**HLR-025: First-User-Frame Capture.**
    The parser shall capture the source file, function name, and line
    number of the first user-code frame in each error block. This
    captured information shall drive the optional source listing emitted
    when the block is finalised.
    *Trace:* [SDD.md §4.3.2](SDD.md), [SDD.md §5](SDD.md).

## 7. Source-Code Listing

These requirements cover the optional display of the offending function's
source code. They are realised by the helpers
`print_source_function`, `parse_ctags_output`, and `execute_command`.

*   <a id="HLR-026"></a>**HLR-026: Conditional Source Reference Output.**
    When a user-code frame has been captured for an error, the parser
    shall emit a `Source (<file>:<line>)` reference line as part of the
    error block's finalisation, irrespective of whether full source
    listing is enabled.
    *Trace:* [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-027"></a>**HLR-027: Conditional Source Body Output.**
    When `-s` or `-v` is in effect and a user-code frame has been
    captured, the parser shall print the body of the offending function
    from the source file, with each line prefixed by its line number and
    the offending line marked with a `>` indicator.
    *Trace:* [SDD.md §4.3.2](SDD.md), [SDD.md §4.3.3](SDD.md).

*   <a id="HLR-028"></a>**HLR-028: Editor-Jumpable References.**
    The `Source (<file>:<line>)` reference and per-frame stack-trace
    output shall use the `file:line` form so that capable terminal-aware
    editors can navigate directly to the location.
    *Trace:* [SDD.md §2.2](SDD.md), [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-029"></a>**HLR-029: Multi-Language Source Support.**
    The source-listing feature shall determine the source file's language
    via `ctags --print-language` and shall correctly identify function
    boundaries for C, C++, Rust (brace-matched), and Fortran
    (`END SUBROUTINE` matched). Other languages shall be reported as
    unsupported without aborting the run.
    *Trace:* [SDD.md §4.3.1](SDD.md), [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-030"></a>**HLR-030: External-Tool Dependency Isolation.**
    Failures in the external `ctags` invocation, in opening a source file,
    or in parsing ctags output shall be reported to `stderr` and shall
    cause only the source listing to be skipped; the surrounding error
    block shall still be rendered.
    *Trace:* [SDD.md §4.4](SDD.md), [SDD.md §4.5](SDD.md).

## 8. Summary Sections

These requirements cover the leak summary and final error summary blocks
emitted by Valgrind at the end of a run.

*   <a id="HLR-031"></a>**HLR-031: Leak Summary Recognition.**
    The parser shall recognise the `LEAK SUMMARY:` marker and the four
    leak category lines (`definitely lost:`, `indirectly lost:`,
    `possibly lost:`, `still reachable:`) when not currently inside an
    error block.
    *Trace:* [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-032"></a>**HLR-032: Leak Summary Output Gated by Options.**
    Leak summary output shall be emitted only when `-l` or `-v` is in
    effect.
    *Trace:* [SDD.md §3.2.1](SDD.md), [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-033"></a>**HLR-033: Leak Detail Formatting.**
    Each recognised leak category line shall be reformatted as
    `* <category>: <bytes> bytes in <blocks> blocks`, using locale-aware
    numeric parsing for the byte and block counts.
    *Trace:* [SDD.md §3.3.2](SDD.md), [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-034"></a>**HLR-034: Final Error Summary Output.**
    On encountering the `ERROR SUMMARY:` line, the parser shall emit a
    `--- FINAL COUNTS ---` block reporting the count of errors classified
    by `vgp` and the difference between Valgrind's reported error total
    and that count (treated as possible leaks). This output shall always
    be emitted, regardless of the configuration flags.
    *Trace:* [SDD.md §4.3.2](SDD.md).

*   <a id="HLR-035"></a>**HLR-035: Robust Numeric Parsing.**
    When numeric parsing of a summary line fails, the parser shall fall
    back to printing the original (PID-stripped) line so that no
    information is silently lost.
    *Trace:* [SDD.md §4.3.2](SDD.md), [SDD.md §4.5](SDD.md).

## 9. Parser State and Robustness

These requirements cover cross-cutting properties of the parser engine.

*   <a id="HLR-036"></a>**HLR-036: Streaming Single-Pass Operation.**
    The parser shall process the input log in a single pass, reading at
    most one line at a time into a fixed-size buffer. It shall not build
    an in-memory model of the entire log.
    *Trace:* [SDD.md §2.2](SDD.md), [SDD.md §4.3.3](SDD.md).

*   <a id="HLR-037"></a>**HLR-037: Bounded Line Buffer.**
    Each input line shall be read into a buffer of at most
    `MAX_LINE_LENGTH` bytes. Lines longer than this limit shall be
    truncated rather than causing a buffer overflow.
    *Trace:* [SDD.md §4.3.3](SDD.md), [SDD.md §4.5](SDD.md), [SDD.md §5](SDD.md).

*   <a id="HLR-038"></a>**HLR-038: Stateful Parsing Context.**
    The parser shall thread a single `ParseState` instance through its
    per-line helpers to track whether it is currently inside an error
    block, how many stack frames have been printed, whether a user-code
    frame has been captured, the captured frame's location, and the
    running error count.
    *Trace:* [SDD.md §4.3.2](SDD.md), [SDD.md §5](SDD.md).

*   <a id="HLR-039"></a>**HLR-039: Defensive Argument Validation.**
    Every parser helper that accepts a pointer argument shall return
    early (without dereferencing) when given a `NULL` pointer.
    *Trace:* [SDD.md §4.5](SDD.md).

*   <a id="HLR-040"></a>**HLR-040: Non-Fatal Recovery from Malformed Input.**
    Malformed or unrecognised lines shall not abort the run. The parser
    shall either skip the line or fall back to printing it verbatim, and
    shall continue with subsequent lines.
    *Trace:* [SDD.md §4.3.2](SDD.md), [SDD.md §4.5](SDD.md).

## 10. Portability and Dependencies

*   <a id="HLR-041"></a>**HLR-041: Standard-C Implementation.**
    The application shall be implementable using only the C standard
    library plus the POSIX functions explicitly required by other
    requirements. No third-party libraries shall be linked into the
    `vgp` binary.
    *Trace:* [SDD.md §2.2](SDD.md), [SDD.md §3.4](SDD.md), [SDD.md §4.4](SDD.md).

*   <a id="HLR-042"></a>**HLR-042: POSIX Process Helpers.**
    The application may use `popen()` / `pclose()` for invoking the
    external `ctags` binary. The corresponding feature-test macros
    (`_POSIX_C_SOURCE`, `_GNU_SOURCE`) shall be defined in
    [inc/vgp.h](../inc/vgp.h) so that the same configuration is shared
    by every translation unit.
    *Trace:* [SDD.md §4.4](SDD.md).

*   <a id="HLR-043"></a>**HLR-043: External `ctags` Dependency Scope.**
    The Universal Ctags binary shall be required only when source listing
    is requested (`-s` or `-v`). Runs that do not request source listing
    shall not invoke any external process.
    *Trace:* [SDD.md §2.2](SDD.md), [SDD.md §4.3.2](SDD.md), [SDD.md §4.4](SDD.md).

## 11. Testability

*   <a id="HLR-044"></a>**HLR-044: Unit-Testable Helpers.**
    Every non-trivial parser helper shall be declared in
    [inc/vgp.h](../inc/vgp.h) with external linkage so that it can be
    invoked directly from the cmocka unit tests under
    [test/unit/](../test/unit/).
    *Trace:* [SDD.md §4.2](SDD.md).

*   <a id="HLR-045"></a>**HLR-045: Deterministic Output.**
    For a given `argv` and a given input log file, the application shall
    produce byte-identical output on `stdout`. (Output on `stderr` may
    include locale-dependent system error strings.)
    *Trace:* [SDD.md §2.2](SDD.md), [SDD.md §3.5](SDD.md).
