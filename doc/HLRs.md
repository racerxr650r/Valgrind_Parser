# High-Level Requirements (HLRs) for Valgrind Parser (vgp)

This document outlines the high-level requirements for the `main.c` and `vgp.c` components of the Valgrind Parser application.

## HLRs for `/home/john/Projects/Valgrind_Parser/main.c`

These requirements primarily cover the application's entry point, command-line interaction, file handling, and orchestration of the parsing process.

*   **HLR-001: Command-Line Interface Provision:** The application shall provide a command-line interface (CLI) for user interaction.
*   **HLR-002: Argument Parsing and Validation:** The application shall parse and validate command-line arguments. This includes:
    *   Accepting a mandatory Valgrind log filename.
    *   Accepting optional flags: `-t` (trace), `-s` (source), `-l` (leaks), `-v` (verbose), `-h` (help).
    *   Verifying that exactly one filename argument is provided (in addition to the program name).
*   **HLR-003: Input Log File Handling:** The application shall attempt to open the Valgrind log file specified by the user in read mode (`"r"`). It shall verify that the file can be successfully opened for reading.
*   **HLR-004: Usage and Help Information Display:** The application shall display usage instructions and option descriptions to standard output if:
    *   Invoked with a help flag (e.g., `-h`).
    *   Command-line argument validation fails (e.g., incorrect number of arguments, missing mandatory filename).
*   **HLR-005: Parser Module Invocation:** The application shall invoke the core Valgrind log processing functions (defined in `/home/john/Projects/Valgrind_Parser/vgp.c`) by passing the opened file handle for the log file and any relevant processed command-line options that dictate parsing behavior.
*   **HLR-006: Application Exit Status:** The application shall terminate with:
    *   A success status (`EXIT_SUCCESS`) upon successful completion of parsing and output.
    *   A failure status (`EXIT_FAILURE`) if critical errors occur, such as inability to open the input log file or invalid command-line arguments.
*   **HLR-007: Initial and Final User Feedback:** The application shall print informational messages to standard output, including:
    *   A message indicating the start of parsing and the name of the log file being processed.
    *   A message indicating the completion of the summary output.
*   **HLR-008: Resource Management:** The application shall ensure that opened files, specifically the Valgrind log file, are properly closed using `fclose` before termination.
*   **HLR-009: Application-Level Error Reporting:** The application shall report errors encountered during its high-level operations (e.g., failure to open the input log file) to standard error. These messages should be descriptive and, where appropriate, include system error information (e.g., via `strerror(errno)`).

## HLRs for `/home/john/Projects/Valgrind_Parser/vgp.c`

These requirements primarily cover the core parsing logic, data extraction from Valgrind logs, and formatted output generation.

*   **HLR-010: Log Line Pre-processing:** The parser shall implement functionality to strip common Valgrind-generated prefixes (e.g., `==<PID>== ` where `<PID>` is a sequence of digits) from individual log lines before further processing.
*   **HLR-011: Error Block Detection and Demarcation:** The parser shall identify the start of distinct error report blocks within the Valgrind log based on predefined keywords (e.g., "Invalid read", "Invalid write", "Conditional jump"). It shall also determine the end of an error block (e.g., by encountering a non-stack-trace line or a new error block).
*   **HLR-012: Error Type Extraction and Display:** For each detected error block, the parser shall extract the main error message/type from the initial line of the block and display it as a formatted header.
*   **HLR-013: Stack Trace Parsing and Formatting:** The parser shall parse stack trace lines (typically starting with "   at " or "   by ") associated with an error. It shall extract:
    *   Function name.
    *   Source file name.
    *   Source line number.
    This information shall be formatted for clear display (e.g., `function_name(file_name:source_line)`).
*   **HLR-014: User Code Prioritization in Stack Traces:** The parser shall differentiate between stack frames originating from user application code and those from system/library code. This differentiation will be based on:
    *   Presence of user-defined source file extensions (e.g., `.c`, `.cpp`).
    *   Absence of paths matching a list of ignored system/library paths.
    The display should prioritize user code stack frames.
*   **HLR-015: Controlled Stack Trace Display Depth:** The parser shall display a limited number of initial stack trace lines. If user code is not found within these initial lines, and more stack lines exist, an ellipsis (`...`) or similar indicator shall be printed. If user code is found, subsequent non-user code lines might also be summarized or limited.
*   **HLR-016: Source Code Snippet Retrieval (Conditional):** If indicated by command-line options (e.g., `-s`, `-v`), and a stack trace line points to user code with a valid filename and line number, the parser shall attempt to open the corresponding source file for reading.
*   **HLR-017: Function Body Identification in Source (Conditional):** Upon successfully opening a source file, the parser shall attempt to locate the definition of the function (identified from the stack trace and target line number). This includes finding the line where the function signature likely starts and the line number of its opening curly brace (`{`).
*   **HLR-018: Source Code Snippet Display with Highlighting (Conditional):** The parser shall display the body of the identified function from the source file.
    *   Each line of the displayed source code shall be prepended with its line number.
    *   The specific line number reported in the Valgrind error shall be highlighted (e.g., with `>` and `<` markers).
    *   The display shall stop after the closing curly brace (`}`) that matches the function's opening brace, or after a safety limit of lines.
*   **HLR-019: Leak Summary Identification and Header Display:** The parser shall identify the "LEAK SUMMARY:" section in the Valgrind log and print a distinct, formatted header for it if leak reporting is enabled.
*   **HLR-020: Leak Detail Parsing and Formatted Display:** The parser shall parse individual lines within the leak summary section (e.g., lines starting with "definitely lost:", "indirectly lost:", "possibly lost:", "still reachable:") to extract:
    *   The type of leak.
    *   The number of bytes lost/reachable.
    *   The number of blocks involved.
    This information shall be displayed in a structured, human-readable format.
*   **HLR-021: Final Error Summary Parsing and Display:** The parser shall identify the "ERROR SUMMARY:" line in the Valgrind log, extract the total number of errors reported, and display this count in a formatted manner (e.g., "Total Errors Reported by Valgrind: <count>").
*   **HLR-022: Robust Parsing of Numerical Data:** The parser shall robustly attempt to parse numerical data (e.g., bytes, blocks from leak summaries; error counts from the final summary) from expected line formats. If parsing fails for a specific line, it should fall back to printing the original line or a warning.
*   **HLR-023: Stateful Parsing Logic:** The parser shall maintain an internal state (e.g., using a `ParseState` structure) to correctly interpret context-dependent information across multiple log lines. This includes tracking:
    *   Whether currently inside an error block.
    *   Whether the source code for the current error should be printed.
    *   The number of stack lines shown for the current error.
    *   Whether user code has been found for the current error.
    *   Information about the current error (type, filename, function name, line number) needed for source display.
*   **HLR-024: Parser-Level Error/Warning Reporting:** The parser shall report warnings or errors to standard error for issues encountered during its specific tasks, such as:
    *   Failure to open a source file for snippet display (including `strerror(errno)`).
    *   Inability to locate a function definition or its opening brace in a source file.
    *   Reaching end-of-file or a line limit during source code printing before finding a matching closing brace.
    *   If no user code is found in a stack trace when source display is expected.
*   **HLR-025: Configurable Behavior based on Options:** The level of detail in the parser's output (e.g., whether to display stack traces, source code snippets, leak summaries) shall be directly controlled by the processed command-line options received from `/home/john/Projects/Valgrind_Parser/main.c`.
