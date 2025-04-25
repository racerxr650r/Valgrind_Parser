# Low-Level Requirements: vgp.c

This document details the low-level requirements for each function within the `vgp.c` source file, referencing the corresponding high-level requirements (HLR) outlined in section 9 of the Software Design Document (`SDD.md`).

---

**1. `main(int argc, char *argv[])`**

*   **LLR 1.1:** Check if the number of command-line arguments (`argc`) is exactly 2 (program name + log file path). (HLR 1.1)
*   **LLR 1.2:** If `argc` is not 2, print a usage message to standard output detailing correct invocation. (HLR 13.1)
*   **LLR 1.3:** If `argc` is not 2, exit the program with a failure status (`EXIT_FAILURE`). (HLR 1.1)
*   **LLR 1.4:** Attempt to open the file specified by `argv[1]` in read mode (`"r"`). (HLR 1.1)
*   **LLR 1.5:** If the file cannot be opened, print an error message to standard error, including the filename and the system error description (`strerror(errno)`). (HLR 9.2)
*   **LLR 1.6:** If the file cannot be opened, exit the program with a failure status (`EXIT_FAILURE`). (HLR 1.1, HLR 9.2)
*   **LLR 1.7:** Print initial informational messages indicating the start of parsing and the name of the log file being processed. (HLR 8.1)
*   **LLR 1.8:** Call `process_log_file` to handle the core parsing logic, passing the opened file handle. (HLR 2.1)
*   **LLR 1.9:** Close the log file using `fclose`. (HLR 1.1)
*   **LLR 1.10:** Print a final message indicating the end of the summary. (HLR 8.1)
*   **LLR 1.11:** Exit the program with a success status (`EXIT_SUCCESS`) upon successful completion.

---

**2. `strip_valgrind_pid_prefix(char *line)`**

*   **LLR 2.1:** Check if the input `line` starts with the Valgrind PID pattern `==<digits>== `. (Implicit HLR 2.1)
*   **LLR 2.2:** If the pattern is found, return a pointer to the character immediately following the pattern. (Implicit HLR 2.1)
*   **LLR 2.3:** If the pattern is not found, return the original `line` pointer. (Implicit HLR 2.1)

---

**3. `is_user_code_stack_trace(const char *line_content)`**

*   **LLR 3.1:** Check if the `line_content` contains any of the file extensions defined in `USER_CODE_EXTENSIONS`. (HLR 4.1, HLR 10.1)
*   **LLR 3.2:** Check if the `line_content` contains any of the path prefixes defined in `IGNORE_PATHS`. (HLR 4.1, HLR 10.1)
*   **LLR 3.3:** Return `true` if a user code extension is found AND no ignored path prefix is found. (HLR 4.1)
*   **LLR 3.4:** Return `false` otherwise. (HLR 4.1)

---

**4. `get_function_name(const char *line, char *newline)`**

*   **LLR 4.1:** Parse the input stack trace `line` to extract the function name or source location information. (HLR 3.2, HLR 4.1)
*   **LLR 4.2:** Format the extracted information into the `newline` buffer, suitable for display in the simplified stack trace. (HLR 8.1)
*   **LLR 4.3:** Return the `newline` buffer. (HLR 8.1)
*   *Note: Specific parsing logic depends on Valgrind format variations.*

---

**5. `print_error_header(const char *line_content)`**

*   **LLR 5.1:** Print a separator line (`---...`) to visually distinguish error blocks. (HLR 8.1)
*   **LLR 5.2:** Print the `line_content` (containing the error type) as the header for the error block. (HLR 8.1)

---

**6. `print_leak_summary_header(void)`**

*   **LLR 6.1:** Print a separator line (`---...`). (HLR 8.1)
*   **LLR 6.2:** Print the "LEAK SUMMARY:" header. (HLR 6.1, HLR 8.1)

---

**7. `print_leak_summary_line(const char *line, const char *leak_type)`**

*   **LLR 7.1:** Attempt to parse the `line` using `sscanf` to extract the number of bytes and blocks for the leak. (HLR 6.2)
*   **LLR 7.2:** If parsing is successful, print a formatted line including the `leak_type`, bytes, and blocks. (HLR 6.2, HLR 8.1)
*   **LLR 7.3:** If parsing fails, print the original `line` content as a fallback. (HLR 9.1)

---

**8. `print_final_error_summary(const char *line)`**

*   **LLR 8.1:** Attempt to parse the `line` using `sscanf` to extract the total number of errors. (HLR 7.1)
*   **LLR 8.2:** If parsing is successful, print a formatted line indicating the total error count. (HLR 7.1, HLR 8.1)
*   **LLR 8.3:** If parsing fails, print the original `line` content as a fallback. (HLR 9.1)

---

**9. `extract_file_and_line(const char *line, char *filename, char *function_name, int *line_number)`**

*   **LLR 9.1:** Attempt to parse the stack trace `line` using `sscanf` or string manipulation to identify and extract the filename, function name (if available in the format), and line number. (HLR 3.2, HLR 4.1)
*   **LLR 9.2:** Store the extracted filename into the `filename` buffer. (HLR 3.2)
*   **LLR 9.3:** Store the extracted function name into the `function_name` buffer. (HLR 3.2)
*   **LLR 9.4:** Store the extracted line number into the `line_number` variable. (HLR 3.2)
*   **LLR 9.5:** Return `true` if filename and line number were successfully extracted. (HLR 3.2)
*   **LLR 9.6:** Return `false` if the required information could not be extracted. (HLR 3.2)

---

**10. `is_valid_function_char(char c)`**

*   **LLR 10.1:** Determine if the input character `c` is alphanumeric (`isalnum`) or an underscore (`_`). (HLR 5.1 - used by source parsing)
*   **LLR 10.2:** Return `true` if the character is valid for a C/C++ identifier, `false` otherwise. (HLR 5.1)

---

**11. `find_function_start_and_brace(FILE *file, const char *function_name, int target_line, int *out_def_line, int *out_brace_line)`**

*   **LLR 11.1:** Rewind the source `file` stream to the beginning. (HLR 5.1)
*   **LLR 11.2:** Read the source `file` line by line. (HLR 5.1)
*   **LLR 11.3:** For each line, attempt to identify if it contains the definition of the specified `function_name` (typically by looking for `function_name(...)`). (HLR 5.1)
*   **LLR 11.4:** If a potential definition line is found, record its line number. (HLR 5.1)
*   **LLR 11.5:** Continue reading subsequent lines to find the first occurrence of an opening curly brace (`{`). (HLR 5.1)
*   **LLR 11.6:** If an opening brace is found after a potential definition, check if the `target_line` is greater than or equal to the definition line number. (HLR 5.1)
*   **LLR 11.7:** If the conditions in LLR 11.6 are met, store the definition line number in `out_def_line` and the brace line number in `out_brace_line`. (HLR 5.1)
*   **LLR 11.8:** If conditions in LLR 11.6 are met, return `true`. (HLR 5.1)
*   **LLR 11.9:** If the end of the file is reached without meeting the conditions, return `false`. (HLR 5.1)
*   **LLR 11.10:** Handle cases where multiple functions might have the same name by resetting the search if a definition is found but the `target_line` precedes it. (HLR 5.1)

---

**12. `print_function_body(FILE *file, int start_print_line, int brace_start_line, int highlight_line)`**

*   **LLR 12.1:** Rewind the source `file` stream to the beginning. (HLR 5.1)
*   **LLR 12.2:** Read the source `file` line by line. (HLR 5.1)
*   **LLR 12.3:** Begin printing lines only when the current line number is greater than or equal to `start_print_line`. (HLR 5.1)
*   **LLR 12.4:** For each printed line, prepend the line number. (HLR 8.1)
*   **LLR 12.5:** If the current line number equals `highlight_line`, prepend a highlight marker (`>`) and append a highlight marker (`<`). (HLR 5.2, HLR 8.1)
*   **LLR 12.6:** If the current line number does not equal `highlight_line`, prepend spaces for alignment. (HLR 8.1)
*   **LLR 12.7:** Print the content of the source line. (HLR 5.1)
*   **LLR 12.8:** Ensure each printed line ends with a newline character. (HLR 8.1)
*   **LLR 12.9:** Starting from `brace_start_line`, track the nesting level of curly braces (`{`, `}`). (HLR 5.1)
*   **LLR 12.10:** Stop printing after the line containing the closing brace that brings the brace count back to zero or less (relative to the start of brace tracking). (HLR 5.1)
*   **LLR 12.11:** Implement a safety break (e.g., after 5000 lines) to prevent excessive printing in case of brace mismatch. (HLR 9.1)
*   **LLR 12.12:** If printing stops due to the safety break or EOF before the brace count returns to zero, print a warning message to standard error. (HLR 9.1)

---

**13. `print_source_function(const char *filename, const char *function_name, int line_number)`**

*   **LLR 13.1:** Check if `function_name` is NULL or empty; if so, print a warning to standard error and return. (HLR 9.1)
*   **LLR 13.2:** Check if `line_number` is less than or equal to 0; if so, print a warning to standard error and return. (HLR 9.1)
*   **LLR 13.3:** Attempt to open the source file specified by `filename` in read mode. (HLR 5.1)
*   **LLR 13.4:** If the source file cannot be opened, print an error to standard error (including `strerror(errno)`) and return. (HLR 9.2)
*   **LLR 13.5:** Call `find_function_start_and_brace` to locate the function definition and opening brace lines. (HLR 5.1)
*   **LLR 13.6:** If `find_function_start_and_brace` returns `true`, call `print_function_body` with the located line numbers and the target `line_number` to highlight. (HLR 5.1, HLR 5.2)
*   **LLR 13.7:** If `find_function_start_and_brace` returns `false`, print a warning to standard error indicating the function/line could not be located. (HLR 9.1)
*   **LLR 13.8:** Close the source file using `fclose`. (HLR 5.1)

---

**14. `initialize_parse_state(ParseState *state)`**

*   **LLR 14.1:** Set `state->in_error_block` to `false`. (HLR 2.1)
*   **LLR 14.2:** Set `state->print_function` to `false`. (HLR 2.1)
*   **LLR 14.3:** Set `state->stack_lines_shown` to `0`. (HLR 2.1)
*   **LLR 14.4:** Set `state->user_code_found_for_error` to `false`. (HLR 2.1)
*   **LLR 14.5:** Initialize `state->current_error_type` to an empty string. (HLR 2.1)
*   **LLR 14.6:** Initialize `state->error_filename` to an empty string. (HLR 2.1)
*   **LLR 14.7:** Initialize `state->error_function_name` to an empty string. (HLR 2.1)
*   **LLR 14.8:** Set `state->error_line_number` to `-1`. (HLR 2.1)

---

**15. `check_start_new_error(const char *line_content, ParseState *state)`**

*   **LLR 15.1:** If `state->in_error_block` is `true`, return `false` immediately. (HLR 3.1)
*   **LLR 15.2:** Iterate through the `ERROR_KEYWORDS` array. (HLR 3.1, HLR 10.1)
*   **LLR 15.3:** For each keyword, check if it is present in `line_content` using `strstr`. (HLR 3.1)
*   **LLR 15.4:** If a keyword is found:
    *   Call `print_error_header` with `line_content`. (HLR 8.1)
    *   Copy the found keyword into `state->current_error_type`. (HLR 3.2)
    *   Set `state->in_error_block` to `true`. (HLR 2.1)
    *   Reset `state->stack_lines_shown` to `0`. (HLR 2.1)
    *   Reset `state->user_code_found_for_error` to `false`. (HLR 2.1)
    *   Reset `state->print_function` to `false`. (HLR 2.1)
    *   Return `true`. (HLR 3.1)
*   **LLR 15.5:** If no keyword is found after checking all, return `false`. (HLR 3.1)

---

**16. `process_stack_trace_line(const char *line_content, ParseState *state)`**

*   **LLR 16.1:** Call `is_user_code_stack_trace` to determine if the line corresponds to user code. (HLR 4.1)
*   **LLR 16.2:** If `state->stack_lines_shown` is less than `STACK_TRACE_CONTEXT_LINES` OR the line is user code:
    *   Call `get_function_name` to format the stack entry. (HLR 4.1, HLR 8.1)
    *   Print the formatted stack entry, indented. (HLR 8.1)
    *   Increment `state->stack_lines_shown`. (HLR 4.2)
    *   If the line is user code AND `state->user_code_found_for_error` is `false`:
        *   Set `state->user_code_found_for_error` to `true`. (HLR 4.1)
        *   Call `extract_file_and_line` to get source location details. (HLR 3.2, HLR 4.1)
        *   Store the result of `extract_file_and_line` in `state->print_function`. (HLR 5.1)
*   **LLR 16.3:** Else if `state->user_code_found_for_error` is `false` AND `state->stack_lines_shown` equals `STACK_TRACE_CONTEXT_LINES`:
    *   Print an ellipsis (`...`) indicator, indented. (HLR 4.2, HLR 8.1)
    *   Increment `state->stack_lines_shown` (to prevent repeated ellipsis). (HLR 4.2)

---

**17. `finalize_error_block(ParseState *state)`**

*   **LLR 17.1:** If `state->user_code_found_for_error` is `false` AND `state->stack_lines_shown` is greater than 0, print a message prompting the user to check the trace for relevant user code. (HLR 8.1, HLR 9.1)
*   **LLR 17.2:** If `state->print_function` is `true`:
    *   Print a header indicating the source file being displayed. (HLR 8.1)
    *   Call `print_source_function` with the stored `error_filename`, `error_function_name`, and `error_line_number`. (HLR 5.1)
    *   Print a blank line for separation. (HLR 8.1)
*   **LLR 17.3:** Set `state->in_error_block` to `false`. (HLR 2.1)
*   **LLR 17.4:** Set `state->print_function` to `false`. (HLR 2.1)
*   *Note: State variables like `stack_lines_shown`, `user_code_found_for_error`, etc., are reset when a *new* error starts, not necessarily here.*

---

**18. `process_in_error_block(const char *line_content, ParseState *state)`**

*   **LLR 18.1:** Check if `line_content` starts with "   at " or "   by ". (HLR 4.1)
*   **LLR 18.2:** If it is a stack trace line, call `process_stack_trace_line` with `line_content` and `state`. (HLR 4.1)
*   **LLR 18.3:** If it is not a stack trace line, call `finalize_error_block` with `state`. (HLR 2.1)

---

**19. `process_summary_lines(const char *line_content)`**

*   **LLR 19.1:** Check if `line_content` contains "LEAK SUMMARY:". If yes, call `print_leak_summary_header`. (HLR 6.1)
*   **LLR 19.2:** Check if `line_content` contains "definitely lost:". If yes, call `print_leak_summary_line` with the line and "Definitely Lost". (HLR 6.2)
*   **LLR 19.3:** Check if `line_content` contains "indirectly lost:". If yes, call `print_leak_summary_line` with the line and "Indirectly Lost". (HLR 6.2)
*   **LLR 19.4:** Check if `line_content` contains "possibly lost:". If yes, call `print_leak_summary_line` with the line and "Possibly Lost". (HLR 6.2)
*   **LLR 19.5:** Check if `line_content` contains "still reachable:". If yes, call `print_leak_summary_line` with the line and "Still Reachable". (HLR 6.2)
*   **LLR 19.6:** Check if `line_content` contains "ERROR SUMMARY:". If yes, call `print_final_error_summary` with the line. (HLR 7.1)

---

**20. `process_log_file(FILE *file)`**

*   **LLR 20.1:** Declare a `ParseState` variable. (HLR 2.1)
*   **LLR 20.2:** Call `initialize_parse_state` to set up the initial state. (HLR 2.1)
*   **LLR 20.3:** Loop through the input `file` line by line using `fgets`. (HLR 2.1)
*   **LLR 20.4:** For each line, call `strip_valgrind_pid_prefix` to get the relevant content pointer. (Implicit HLR 2.1)
*   **LLR 20.5:** Skip lines that are empty or contain only whitespace, unless currently inside an error block (`state.in_error_block` is true). (HLR 2.1)
*   **LLR 20.6:** If `state.in_error_block` is `true`, call `process_in_error_block`. (HLR 2.1, HLR 4.1)
*   **LLR 20.7:** If `state.in_error_block` is `false` (either initially or after `finalize_error_block` was called):
    *   Call `check_start_new_error`. (HLR 3.1)
    *   If `check_start_new_error` returns `true`, continue to the next line. (HLR 2.1)
    *   If `check_start_new_error` returns `false`, call `process_summary_lines`. (HLR 6.1, HLR 7.1)
*   **LLR 20.8:** After the loop finishes, check if `state.in_error_block` is still `true`. (HLR 2.1, HLR 9.1)
*   **LLR 20.9:** If `state.in_error_block` is `true` after the loop, call `finalize_error_block` to handle the last unterminated block. (HLR 2.1, HLR 5.1)

---