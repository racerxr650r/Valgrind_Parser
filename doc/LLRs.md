# Low-Level Requirements (LLRs) for Valgrind Parser (vgp)

## Low-Level Requirements (LLRs) for the main() function in /home/john/Projects/Valgrind_Parser/main.c

These LLRs detail the specific operations and logic contained within the `main()` function.

*   **LLR-M01 (Ref: HLR-001):** The `main()` function in `/home/john/Projects/Valgrind_Parser/main.c` shall be defined with the signature `int main(int argc, char *argv[])` and serve as the application's primary entry point.
*   **LLR-M02 (Ref: N/A - General good practice):** As its first operational step, the `main()` function shall call `setlocale(LC_NUMERIC, "")` to ensure that numeric formatting (e.g., for thousands separators if used by `printf` with `%'d`) conforms to the user's locale settings.
*   **LLR-M03 (Ref: HLR-002, HLR-004):** The `main()` function shall invoke the `parse_command_line(argc, argv)` function, passing the command-line arguments it received. This delegates the responsibility of parsing these arguments, populating the global `app_config` structure, handling the help option (`-h`), and exiting on argument-related errors.
*   **LLR-M04 (Ref: HLR-003):** Subsequent to the return of `parse_command_line()` (which implies that argument parsing was successful and the program did not exit due to help request or argument error), the `main()` function shall retrieve the Valgrind log filename by assigning the value of `app_config.log_file` to a local `const char *filename` variable.
*   **LLR-M05 (Ref: HLR-003):** The `main()` function shall attempt to open the file specified by the local `filename` variable in read-only mode (`"r"`) using the `fopen()` standard library function. The result of this operation (a `FILE*` pointer or `NULL`) shall be stored in a local `FILE *file` variable.
*   **LLR-M06 (Ref: HLR-003, HLR-009, HLR-006):** The `main()` function shall check if the `file` variable is `NULL` immediately after the `fopen()` call.
    *   If `file` is `NULL` (indicating an error in opening the file), an error message shall be printed to `stderr` using `fprintf`. This message must be formatted as: `"Error opening file '%s': %s\n"`, where the first `%s` is replaced by the `filename` and the second `%s` is replaced by the system error message string obtained from `strerror(errno)`.
    *   Following the error message, the `main()` function shall return `EXIT_FAILURE`.
*   **LLR-M07 (Ref: HLR-007):** If the `file` variable is not `NULL` (indicating the file was opened successfully), the `main()` function shall print an informational message to `stdout` using `printf`. This message must be formatted as: `"Parsing Valgrind Log File: %s\n"`, where `%s` is replaced by the `filename`.
*   **LLR-M08 (Ref: HLR-005):** The `main()` function shall call the `process_log_file()` function (prototyped in `/home/john/Projects/Valgrind_Parser/vgp.h` and defined in `/home/john/Projects/Valgrind_Parser/vgp.c`), passing the `FILE *file` (the opened Valgrind log file) as its argument.
*   **LLR-M09 (Ref: HLR-008):** Upon return from the `process_log_file()` function, the `main()` function shall call `fclose(file)` to close the Valgrind log file stream.
*   **LLR-M10 (Ref: HLR-006):** After successfully closing the file, the `main()` function shall return `EXIT_SUCCESS` to indicate successful completion of the program.

## Low-Level Requirements (LLRs) for the parse_command_line() function in /home/john/Projects/Valgrind_Parser/main.c

These LLRs detail the specific operations and logic for parsing command-line arguments and configuring the application's behavior. It is assumed that this function operates on a globally accessible `app_config` structure.

*   **LLR-PCL01 (Ref: HLR-002):** The `parse_command_line()` function shall be defined with the signature `void parse_command_line(int argc, char *argv[])`.
*   **LLR-PCL02 (Ref: HLR-002):** The `parse_command_line()` function shall initialize all boolean flag members of the global `app_config` structure (e.g., `trace_enabled`, `source_enabled`, `leaks_enabled`, `verbose_enabled`, `help_enabled`) to `false` before parsing any arguments.
*   **LLR-PCL03 (Ref: HLR-002):** The `parse_command_line()` function shall initialize the `app_config.log_file` member to `NULL` before parsing any arguments.
*   **LLR-PCL04 (Ref: HLR-002):** The `parse_command_line()` function shall use a `while` loop in conjunction with the `getopt(argc, argv, "tslvh")` standard library function to iterate through and process command-line options.
*   **LLR-PCL05 (Ref: HLR-002):** Within the `getopt()` loop, a `switch` statement on the character returned by `getopt()` shall be used to handle recognized options:
    *   **Case 't':** The `app_config.trace_enabled` member shall be set to `true`.
    *   **Case 's':** The `app_config.source_enabled` member shall be set to `true`.
    *   **Case 'l':** The `app_config.leaks_enabled` member shall be set to `true`.
    *   **Case 'v':** The `app_config.verbose_enabled` member shall be set to `true`. Additionally, `app_config.trace_enabled`, `app_config.source_enabled`, and `app_config.leaks_enabled` shall also be set to `true` as the verbose option implies these.
    *   **Case 'h':** The `app_config.help_enabled` member shall be set to `true`.
    *   **Case '?':** (Handles unknown options or missing option arguments from `getopt()`) The `print_usage(argv[0])` function shall be called, and the program shall then exit with `EXIT_FAILURE`.
*   **LLR-PCL06 (Ref: HLR-004, HLR-006):** After the `getopt()` loop completes, if `app_config.help_enabled` is `true`, the `print_usage(argv[0])` function shall be called, and the program shall then exit with `EXIT_SUCCESS`.
*   **LLR-PCL07 (Ref: HLR-002):** After the `getopt()` loop and help check, the function shall check if the number of remaining non-option arguments (`argc - optind`) is exactly equal to `1`.
*   **LLR-PCL08 (Ref: HLR-002, HLR-004, HLR-006):** If `argc - optind` is not equal to `1`:
    *   An error message "Error: A single Valgrind log filename is required.\n" shall be printed to `stderr`.
    *   The `print_usage(argv[0])` function shall be called.
    *   The program shall then exit with `EXIT_FAILURE`.
*   **LLR-PCL09 (Ref: HLR-002):** If `argc - optind` is equal to `1`, the `app_config.log_file` member shall be assigned the value of `argv[optind]` (the first non-option argument, which is the filename).
*   **LLR-PCL10 (Ref: N/A - Internal consistency):** The `parse_command_line()` function shall not return a value (i.e., its return type is `void`) as it either successfully populates the global `app_config` or causes the program to exit.

## Low-Level Requirements (LLRs) for the is_user_code_stack_trace() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific logic for determining if a given line from a Valgrind log represents a stack trace frame originating from user-written code.

*   **LLR-IUCST01 (Ref: HLR-014):** The `is_user_code_stack_trace()` function shall be defined with the signature `bool is_user_code_stack_trace(const char *line)`.
*   **LLR-IUCST02 (Ref: HLR-014):** The function shall first check if the input `line` string starts with the Valgrind stack trace prefix "   at " (6 characters) using `strncmp()`.
*   **LLR-IUCST03 (Ref: HLR-014):** If the `line` does not start with "   at ", the function shall then check if it starts with the Valgrind stack trace prefix "   by " (6 characters) using `strncmp()`.
*   **LLR-IUCST04 (Ref: HLR-014):** If the `line` does not start with either "   at " or "   by ", the function shall immediately return `false`.
*   **LLR-IUCST05 (Ref: HLR-014):** If the line has a valid stack trace prefix, the function shall then iterate through the `USER_CODE_EXTENSIONS` global array (defined in `/home/john/Projects/Valgrind_Parser/vgp.c`). The iteration shall continue as long as the current array element is not `NULL`.
*   **LLR-IUCST06 (Ref: HLR-014):** For each extension string in `USER_CODE_EXTENSIONS`, the function shall use `strstr(line, USER_CODE_EXTENSIONS[i])` to check if the extension is present as a substring within the input `line`.
*   **LLR-IUCST07 (Ref: HLR-014):** If any user code extension is found in the `line`, a local boolean flag (e.g., `found_user_extension`) shall be set to `true`, and the iteration through `USER_CODE_EXTENSIONS` shall terminate immediately (e.g., using `break`).
*   **LLR-IUCST08 (Ref: HLR-014):** After checking all `USER_CODE_EXTENSIONS`, if the `found_user_extension` flag remains `false`, the function shall return `false`.
*   **LLR-IUCST09 (Ref: HLR-014):** If a user code extension was found, the function shall then iterate through the `IGNORE_PATHS` global array (defined in `/home/john/Projects/Valgrind_Parser/vgp.c`). The iteration shall continue as long as the current array element is not `NULL`.
*   **LLR-IUCST10 (Ref: HLR-014):** For each path string in `IGNORE_PATHS`, the function shall use `strstr(line, IGNORE_PATHS[i])` to check if the ignored path is present as a substring within the input `line`.
*   **LLR-IUCST11 (Ref: HLR-014):** If any ignored path is found in the `line`, the function shall immediately return `false`.
*   **LLR-IUCST12 (Ref: HLR-014):** If the input `line` starts with a valid stack trace prefix, contains a user code extension, and does not contain any of the ignored paths, the function shall return `true`.

## Low-Level Requirements (LLRs) for the strip_valgrind_pid_prefix() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific logic for identifying and removing the Valgrind process ID (PID) prefix (e.g., `==<PID>== `) from a given log line.

*   **LLR-SVPP01 (Ref: HLR-010):** The `strip_valgrind_pid_prefix()` function shall be defined with the signature `char *strip_valgrind_pid_prefix(char *line)`.
*   **LLR-SVPP02 (Ref: HLR-010):** The function shall first search for the initial "==" sequence in the input `line` string using `strstr()`.
*   **LLR-SVPP03 (Ref: HLR-010):** If the `strstr()` call in LLR-SVPP02 returns `NULL` (meaning "==" is not found), the function shall immediately return the original `line` pointer without modification.
*   **LLR-SVPP04 (Ref: HLR-010):** If "==" is found, a temporary pointer shall be advanced by 2 characters to move past this initial "==".
*   **LLR-SVPP05 (Ref: HLR-010):** The function shall then skip any subsequent whitespace characters (checked using `isspace()`) by advancing the temporary pointer.
*   **LLR-SVPP06 (Ref: HLR-010):** After skipping whitespace, the function shall check if the character at the current temporary pointer position is a digit (checked using `isdigit()`).
*   **LLR-SVPP07 (Ref: HLR-010):** If the character is not a digit, the function shall immediately return the original `line` pointer, as the format does not match the expected PID prefix.
*   **LLR-SVPP08 (Ref: HLR-010):** If the character is a digit, the function shall continue to advance the temporary pointer as long as subsequent characters are digits, effectively skipping the entire PID number.
*   **LLR-SVPP09 (Ref: HLR-010):** After skipping all digits of the PID, the function shall again skip any subsequent whitespace characters by advancing the temporary pointer.
*   **LLR-SVPP10 (Ref: HLR-010):** The function shall then check if the characters at the current temporary pointer position match the sequence "== " (three characters: equals, equals, space) using `strncmp()`.
*   **LLR-SVPP11 (Ref: HLR-010):** If the sequence "== " is not found, the function shall immediately return the original `line` pointer, as the format does not match the expected end of the PID prefix.
*   **LLR-SVPP12 (Ref: HLR-010):** If the "== " sequence is found, the temporary pointer shall be advanced by 3 characters to move past it.
*   **LLR-SVPP13 (Ref: HLR-010):** The function shall then return the final value of the temporary pointer, which now points to the beginning of the actual log message content after the stripped prefix.

## Low-Level Requirements (LLRs) for the get_function_name() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific logic for parsing a Valgrind stack trace line and reformatting it to show the function name, base filename, and line number.

*   **LLR-GFN01 (Ref: HLR-013):** The `get_function_name()` function shall be defined with the signature `char *get_function_name(const char *line, char *newline)`.
*   **LLR-GFN02 (Ref: HLR-013):** The function shall declare and initialize a local character array `function` of size `MAX_LINE_LENGTH` (defined in `/home/john/Projects/Valgrind_Parser/vgp.h`) with the default string "?".
*   **LLR-GFN03 (Ref: HLR-013):** The function shall declare and initialize a local character array `filename` of size `MAX_LINE_LENGTH` with the default string "?".
*   **LLR-GFN04 (Ref: HLR-013):** The function shall declare and initialize a local integer variable `line_number` to `0`.
*   **LLR-GFN05 (Ref: HLR-013):** The function shall calculate the length of the input `line` string using `strlen()` and store it in a local integer variable (e.g., `length`).
*   **LLR-GFN06 (Ref: HLR-013):** The function shall use `sscanf()` with the input `line` and the format string `"%*s %*s %[^ ] (%[^:]:%d)"` to attempt to parse and populate:
    *   The `function` array (extracting characters up to the first space after skipping two initial strings like "at" and the address).
    *   The `filename` array (extracting characters up to the colon `:` within the parentheses).
    *   The `line_number` integer variable (extracting digits after the colon).
*   **LLR-GFN07 (Ref: HLR-013):** After the `sscanf()` call, the function shall check if the `line_number` variable is non-zero. (Note: `sscanf`'s return value indicating the number of successful assignments would be a more robust check, but the current code checks `line_number` itself).
*   **LLR-GFN08 (Ref: HLR-013):** If `line_number` is non-zero (indicating a successful parse of the line number component):
    *   The function shall call `basename()` (from `libgen.h`) with the parsed `filename` as input to obtain only the file's base name.
    *   The function shall use `snprintf()` to format the output string into the `newline` buffer. The format shall be `"%s(%s:%d)\n"`, using the parsed `function`, the result of `basename(filename)`, and the parsed `line_number`.
    *   The `size` argument for `snprintf()` shall be the `length` calculated in LLR-GFN05 to prevent buffer overflows in `newline` (though `newline` should ideally be sized appropriately by the caller or have a known max size).
*   **LLR-GFN09 (Ref: HLR-013):** If `line_number` is zero (indicating the line number component was not successfully parsed or was parsed as zero):
    *   The function shall use `snprintf()` to format the output string into the `newline` buffer as `"?\?(?:0)\n"`.
    *   The `size` argument for `snprintf()` shall be the `length` calculated in LLR-GFN05.
*   **LLR-GFN10 (Ref: HLR-013):** The function shall return the pointer to the `newline` buffer.

## Low-Level Requirements (LLRs) for the print_error_header() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific operations for printing a formatted header at the beginning of a detected Valgrind error block.

*   **LLR-PEH01 (Ref: HLR-012):** The `print_error_header()` function shall be defined with the signature `void print_error_header(const char *error_type, ParseState *state)`.
*   **LLR-PEH02 (Ref: HLR-012):** The function shall first print a horizontal separator line "----------------------------------------\n" to `stdout` using `printf()`.
*   **LLR-PEH03 (Ref: HLR-012, HLR-021):** The function shall increment the `state->error_count` member by one.
*   **LLR-PEH04 (Ref: HLR-012):** The function shall then print a formatted string to `stdout` using `printf()`. This string shall be `"[ERROR #%d] "`, where `%d` is replaced by the new value of `state->error_count`.
*   **LLR-PEH05 (Ref: HLR-012):** The function shall calculate the length of the input `error_type` string using `strlen()`.
*   **LLR-PEH06 (Ref: HLR-012):** The function shall check if the last character of the `error_type` string (at index `length - 1`) is a newline character (`\n`).
*   **LLR-PEH07 (Ref: HLR-012):**
    *   If the last character of `error_type` is a newline, the function shall print the `error_type` string to `stdout` as is (e.g., using `printf("%s", error_type)`).
    *   If the last character of `error_type` is not a newline, the function shall print the `error_type` string to `stdout` followed by a newline character (e.g., using `printf("%s\n", error_type)`).
*   **LLR-PEH08 (Ref: HLR-012, HLR-025):** The function shall check if either the `app_config.print_stack` global boolean flag is `true` OR the `app_config.verbose` global boolean flag is `true`.
*   **LLR-PEH09 (Ref: HLR-012, HLR-025):** If the condition in LLR-PEH08 is met, the function shall print the string "Call Stack:\n" to `stdout` using `printf()`.

## Low-Level Requirements (LLRs) for the print_leak_summary_line() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific operations for parsing and printing a single line from Valgrind's leak summary section.

*   **LLR-PLSL01 (Ref: HLR-020):** The `print_leak_summary_line()` function shall be defined with the signature `void print_leak_summary_line(const char *line, const char *leak_type)`.
*   **LLR-PLSL02 (Ref: HLR-020):** The function shall declare two local integer variables, `bytes` and `blocks`, and initialize them to `0`.
*   **LLR-PLSL03 (Ref: HLR-020, HLR-022):** The function shall attempt to parse the input `line` string using `sscanf()` with the format string `"%*[^:]: %'d %*s %*s %'d"`.
    *   This format string is intended to:
        *   Skip all characters up to and including the first colon (`:`).
        *   Read an integer (respecting locale for thousands separators due to `'`) into the `bytes` variable.
        *   Skip the next string (e.g., "bytes").
        *   Skip the subsequent string (e.g., "in").
        *   Read an integer (respecting locale for thousands separators) into the `blocks` variable.
*   **LLR-PLSL04 (Ref: HLR-020):** The function shall check if the return value of the `sscanf()` call (from LLR-PLSL03) is equal to `2`, indicating that both `bytes` and `blocks` were successfully parsed and assigned.
*   **LLR-PLSL05 (Ref: HLR-020):** If the `sscanf()` call successfully parsed 2 items:
    *   The function shall print a formatted string to `stdout` using `printf()`.
    *   The format of this string shall be `"* %s: %d bytes in %d blocks\n"`, where the first `%s` is replaced by the input `leak_type` string, the first `%d` by the parsed `bytes` value, and the second `%d` by the parsed `blocks` value.
*   **LLR-PLSL06 (Ref: HLR-022):** If the `sscanf()` call did not successfully parse 2 items (i.e., its return value was not `2`):
    *   The function shall calculate the length of the input `line` string using `strlen()`.
    *   The function shall check if the last character of the `line` string (at index `length - 1`) is a newline character (`\n`).
*   **LLR-PLSL07 (Ref: HLR-022):** If `sscanf()` failed and the last character of `line` is a newline character:
    *   The function shall print the original `line` string to `stdout` as is (e.g., using `printf("%s", line)`).
*   **LLR-PLSL08 (Ref: HLR-022):** If `sscanf()` failed and the last character of `line` is not a newline character:
    *   The function shall print the original `line` string to `stdout` followed by a newline character (e.g., using `printf("%s\n", line)`).

## Low-Level Requirements (LLRs) for the print_final_error_summary() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific operations for parsing Valgrind's "ERROR SUMMARY:" line and printing a consolidated final count of errors and potential leaks.

*   **LLR-PFES01 (Ref: HLR-021):** The `print_final_error_summary()` function shall be defined with the signature `void print_final_error_summary(const char *line, ParseState *state)`.
*   **LLR-PFES02 (Ref: HLR-021):** The function shall declare a local integer variable `error_count` and initialize it to `0`.
*   **LLR-PFES03 (Ref: HLR-021, HLR-022):** The function shall attempt to parse the input `line` string using `sscanf()` with the format string `"%*[^:]: %'d"`.
    *   This format string is intended to:
        *   Skip all characters up to and including the first colon (`:`).
        *   Read an integer (respecting locale for thousands separators due to `'`) into the `error_count` variable.
*   **LLR-PFES04 (Ref: HLR-021):** The function shall check if the return value of the `sscanf()` call (from LLR-PFES03) is equal to `1`, indicating that the `error_count` was successfully parsed and assigned.
*   **LLR-PFES05 (Ref: HLR-021):** If the `sscanf()` call successfully parsed 1 item:
    *   The function shall print a header string "\n--- FINAL COUNTS ---\n" to `stdout` using `printf()`.
    *   The function shall print a formatted string to `stdout` using `printf()`. The format shall be `"* Total Errors: %d\n"`, where `%d` is replaced by the value of `state->error_count` (the count of errors processed by this parser).
    *   The function shall print a formatted string to `stdout` using `printf()`. The format shall be `"* Possible Leaks: %d\n"`, where `%d` is replaced by the result of `error_count - state->error_count` (the difference between Valgrind's total error/leak count and this parser's specific error count).
*   **LLR-PFES06 (Ref: HLR-022):** If the `sscanf()` call did not successfully parse 1 item (i.e., its return value was not `1`):
    *   The function shall calculate the length of the input `line` string using `strlen()`.
    *   The function shall check if the last character of the `line` string (at index `length - 1`) is a newline character (`\n`).
*   **LLR-PFES07 (Ref: HLR-022):** If `sscanf()` failed and the last character of `line` is a newline character:
    *   The function shall print the original `line` string to `stdout` as is (e.g., using `printf("%s", line)`).
*   **LLR-PFES08 (Ref: HLR-022):** If `sscanf()` failed and the last character of `line` is not a newline character:
    *   The function shall print the original `line` string to `stdout` followed by a newline character (e.g., using `printf("%s\n", line)`).

## Low-Level Requirements (LLRs) for the extract_file_and_line() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific logic for parsing a Valgrind stack trace line to extract the filename, function name, and line number.

*   **LLR-EFAL01 (Ref: HLR-013):** The `extract_file_and_line()` function shall be defined with the signature `bool extract_file_and_line(const char *line, char *filename, char *function_name, int *line_number)`.
*   **LLR-EFAL02 (Ref: HLR-013):** The function shall first check if any of the input pointers (`line`, `filename`, `function_name`, `line_number`) are `NULL`. If any are `NULL`, the function shall immediately return `false`.
*   **LLR-EFAL03 (Ref: HLR-013):** The function shall declare local temporary character arrays `temp_filename` and `temp_function`, each of size `MAX_LINE_LENGTH` (defined in `/home/john/Projects/Valgrind_Parser/vgp.h`).
*   **LLR-EFAL04 (Ref: HLR-013):** The function shall first attempt to parse the input `line` using `sscanf()` with the format string `"%*[^:]: %[^(]( %[^:]:%d)"`. This format attempts to capture:
    *   A function name (characters before an opening parenthesis, stored in `temp_function`).
    *   A filename (characters between the opening parenthesis and a colon, stored in `temp_filename`).
    *   A line number (integer after the colon, stored in the `line_number` pointer).
*   **LLR-EFAL05 (Ref: HLR-013, HLR-016, HLR-017):** If the `sscanf()` call in LLR-EFAL04 returns `3` (indicating all three components were successfully parsed and assigned):
    *   A pointer `filename_ptr` shall be assigned the result of `strtok(temp_filename, " \t")` to attempt to remove leading whitespace from `temp_filename`.
    *   `filename_ptr` shall then be reassigned the result of `strtok(filename_ptr, " \t")` to attempt to remove trailing whitespace from the (now potentially modified) `temp_filename`.
    *   The content pointed to by `filename_ptr` (or the modified `temp_filename`) shall be copied into the output `filename` buffer using `strcpy()`.
    *   A pointer `functionname_ptr` shall be assigned the result of `strtok(temp_function, " \t")` to attempt to remove leading whitespace from `temp_function`.
    *   `functionname_ptr` shall then be reassigned the result of `strtok(functionname_ptr, " \t")` to attempt to remove trailing whitespace from the (now potentially modified) `temp_function`.
    *   The content pointed to by `functionname_ptr` (or the modified `temp_function`) shall be copied into the output `function_name` buffer using `strcpy()`.
    *   The function shall return `true`.
*   **LLR-EFAL06 (Ref: HLR-013, HLR-016):** If the `sscanf()` call in LLR-EFAL04 did not return `3`, the function shall then attempt to parse the input `line` using `sscanf()` with the format string `"%*[^:]: %[^:]:%d"`. This format attempts to capture:
    *   A filename (characters before a colon, stored in `temp_filename`).
    *   A line number (integer after the colon, stored in the `line_number` pointer).
*   **LLR-EFAL07 (Ref: HLR-013, HLR-016):** If the `sscanf()` call in LLR-EFAL06 returns `2` (indicating filename and line number were successfully parsed):
    *   The content of `temp_filename` shall be copied into the output `filename` buffer using `strcpy()`.
    *   The output `function_name` buffer shall be set to an empty string using `strcpy(function_name, "")`.
    *   The function shall return `true`.
*   **LLR-EFAL08 (Ref: HLR-013, HLR-017):** If the `sscanf()` call in LLR-EFAL06 did not return `2`, the function shall then attempt to parse the input `line` using `sscanf()` with the format string `"%*[^:]: %[^(](in %[^)])"`. This format attempts to capture:
    *   A function name (characters before an opening parenthesis, stored in `temp_function`).
    *   A filename (characters within "in (...)" part, stored in `temp_filename`).
*   **LLR-EFAL09 (Ref: HLR-013, HLR-017):** If the `sscanf()` call in LLR-EFAL08 returns `2` (indicating function name and filename were successfully parsed):
    *   A pointer `filename_ptr` shall be assigned the result of `strtok(temp_filename, " \t")` to attempt to remove leading whitespace from `temp_filename`.
    *   `filename_ptr` shall then be reassigned the result of `strtok(filename_ptr, " \t")` to attempt to remove trailing whitespace from the (now potentially modified) `temp_filename`.
    *   The content pointed to by `filename_ptr` (or the modified `temp_filename`) shall be copied into the output `filename` buffer using `strcpy()`.
    *   A pointer `functionname_ptr` shall be assigned the result of `strtok(temp_function, " \t")` to attempt to remove leading whitespace from `temp_function`.
    *   `functionname_ptr` shall then be reassigned the result of `strtok(functionname_ptr, " \t")` to attempt to remove trailing whitespace from the (now potentially modified) `temp_function`.
    *   The content pointed to by `functionname_ptr` (or the modified `temp_function`) shall be copied into the output `function_name` buffer using `strcpy()`.
    *   The integer pointed to by `line_number` shall be set to `0`.
    *   The function shall return `true`.
*   **LLR-EFAL10 (Ref: HLR-013):** If none of the preceding `sscanf()` attempts result in a successful parse (as defined by their respective LLRs), the function shall return `false`.

## Low-Level Requirements (LLRs) for the execute_command() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific logic for executing an external shell command and capturing its first line of standard output. This function is primarily used to interact with `ctags` for function boundary detection.

*   **LLR-EC01 (Ref: HLR-017):** The `execute_command()` function shall be defined with the signature `bool execute_command(const char *command, char *output, size_t output_size)`.
*   **LLR-EC02 (Ref: HLR-017):** The function shall first validate its input arguments. It shall check if the `command` pointer is `NULL`, if the `output` pointer is `NULL`, or if `output_size` is `0`.
*   **LLR-EC03 (Ref: HLR-017):** If any of the conditions in LLR-EC02 are met (i.e., an invalid argument is detected), the function shall immediately return `false`.
*   **LLR-EC04 (Ref: HLR-017):** The function shall call `popen()` with the provided `command` string and the mode `"r"` (read) to execute the command and open a pipe to its standard output. The `FILE*` pointer returned by `popen()` shall be stored.
*   **LLR-EC05 (Ref: HLR-017, HLR-024):** The function shall check if the `FILE*` pointer returned by `popen()` is `NULL`.
    *   If it is `NULL` (indicating `popen()` failed), the function shall call `perror("popen")` to print a system error message to `stderr`.
    *   Following the `perror()` call, the function shall return `false`.
*   **LLR-EC06 (Ref: HLR-017):** If `popen()` was successful, the function shall call `fgets()` to read at most `output_size - 1` characters from the pipe (associated with the `FILE*` pointer from `popen()`) into the `output` buffer.
*   **LLR-EC07 (Ref: HLR-017, HLR-024):** The function shall check if the `fgets()` call returned `NULL` (indicating an error or end-of-file before any characters were read).
    *   If `fgets()` returned `NULL`, the function shall then check `ferror()` on the `FILE*` pointer to determine if a read error occurred.
    *   If `ferror()` returns a non-zero value (indicating a read error), the function shall call `perror("fgets")` to print a system error message to `stderr`.
    *   Regardless of whether `ferror()` indicated an error, if `fgets()` returned `NULL`, the function shall call `pclose()` on the `FILE*` pointer and then return `false`.
*   **LLR-EC08 (Ref: HLR-017):** After successfully reading from the pipe (or if `fgets` returned `NULL` but no read error occurred, which implies the command produced no output or only an empty line), the function shall call `pclose()` on the `FILE*` pointer to close the pipe and wait for the command to terminate.
*   **LLR-EC09 (Ref: HLR-017):** If all operations up to closing the pipe were successful (specifically, if `popen` succeeded and `fgets` did not indicate a stream error that caused an early return), the function shall return `true`. (Note: The current implementation returns `true` even if `fgets` returns `NULL` as long as `ferror` is not set, implying an empty output is not a failure of `execute_command` itself).

## Low-Level Requirements (LLRs) for the parse_ctags_output() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific logic for parsing the output string from a `ctags` command to extract the start and end line numbers of a function definition.

*   **LLR-PCO01 (Ref: HLR-017):** The `parse_ctags_output()` function shall be defined with the signature `bool parse_ctags_output(const char *ctags_output, int *start_line, int *end_line)`.
*   **LLR-PCO02 (Ref: HLR-017):** The function shall first check if any of the input pointers (`ctags_output`, `start_line`, `end_line`) are `NULL`.
*   **LLR-PCO03 (Ref: HLR-017, HLR-024):** If any of the input pointers checked in LLR-PCO02 are `NULL`, the function shall print the error message "Error: Invalid arguments to parse_ctags_output.\n" to `stderr` using `fprintf()`.
*   **LLR-PCO04 (Ref: HLR-017):** If any of the input pointers checked in LLR-PCO02 are `NULL`, the function shall immediately return `false`.
*   **LLR-PCO05 (Ref: HLR-017):** The function shall search for the substring "line:" within the `ctags_output` string using `strstr()`. The result of this search shall be stored in a local `char *input` pointer.
*   **LLR-PCO06 (Ref: HLR-017, HLR-024):** If the `input` pointer (from LLR-PCO05) is `NULL` (meaning "line:" was not found), the function shall print the error message "Error: Invalid ctags output format.\n" to `stderr` using `fprintf()`.
*   **LLR-PCO07 (Ref: HLR-017):** If the `input` pointer (from LLR-PCO05) is `NULL`, the function shall immediately return `false`.
*   **LLR-PCO08 (Ref: HLR-017):** If "line:" was found, the function shall attempt to parse an integer immediately following "line:" from the `input` string using `sscanf(input, "line:%d", start_line)`.
*   **LLR-PCO09 (Ref: HLR-017):** The function shall then search for the substring "end:" within the `input` string (which now points to or after "line:") using `strstr()`. The result of this search shall update the `input` pointer.
*   **LLR-PCO10 (Ref: HLR-017, HLR-024):** If the `input` pointer (from LLR-PCO09) is `NULL` (meaning "end:" was not found after "line:"), the function shall print the error message "Error: Invalid ctags output format.\n" to `stderr` using `fprintf()`.
*   **LLR-PCO11 (Ref: HLR-017):** If the `input` pointer (from LLR-PCO09) is `NULL`, the function shall immediately return `false`.
*   **LLR-PCO12 (Ref: HLR-017):** If "end:" was found, the function shall attempt to parse an integer immediately following "end:" from the `input` string using `sscanf(input, "end:%d", end_line)`.
*   **LLR-PCO13 (Ref: HLR-017):** The function shall validate the parsed line numbers. It will check if `*start_line` is less than or equal to `0`, OR if `*end_line` is less than or equal to `0`, OR if `*start_line` is greater than or equal to `*end_line`.
*   **LLR-PCO14 (Ref: HLR-017, HLR-024):** If any of the conditions in LLR-PCO13 are true (indicating invalid line numbers), the function shall print a formatted error message to `stderr` using `fprintf()`. The message shall be: "Error: Invalid line range in ctags output (start: %d, end: %d).\n", where the `%d` placeholders are filled with the values of `*start_line` and `*end_line` respectively.
*   **LLR-PCO15 (Ref: HLR-017):** If any of the conditions in LLR-PCO13 are true, the function shall immediately return `false`.
*   **LLR-PCO16 (Ref: HLR-017):** If all preceding checks and parsing steps are successful and the line numbers are valid, the function shall return `true`.

## Low-Level Requirements (LLRs) for the print_source_function() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific logic for locating and printing the source code of an entire function, highlighting a specific line within it, using `ctags` for function boundary detection.

*   **LLR-PSF01 (Ref: HLR-018):** The `print_source_function()` function shall be defined with the signature `void print_source_function(const char *source_file, const char *function_name, int line_number)`.
*   **LLR-PSF02 (Ref: HLR-016, HLR-017, HLR-024):** The function shall first validate its input arguments. It shall check if the `source_file` pointer is `NULL`, if the `function_name` pointer is `NULL`, or if `line_number` is less than or equal to `0`.
*   **LLR-PSF03 (Ref: HLR-016, HLR-017, HLR-024):** If any of the conditions in LLR-PSF02 are met (i.e., an invalid argument is detected), the function shall print the error message "Error: Invalid arguments to print_source_function.\n" to `stderr` using `fprintf()`.
*   **LLR-PSF04 (Ref: HLR-016, HLR-017):** If any of the conditions in LLR-PSF02 are met, the function shall immediately return (void function).
*   **LLR-PSF05 (Ref: HLR-017):** The function shall declare a local character array `command` of size `MAX_LINE_LENGTH` (defined in `/home/john/Projects/Valgrind_Parser/vgp.h`).
*   **LLR-PSF06 (Ref: HLR-017):** The function shall declare a local character array `ctags_output` of size `MAX_LINE_LENGTH`.
*   **LLR-PSF07 (Ref: HLR-017):** The function shall declare local integer variables `start_line` and `end_line`, initializing them to `0`.
*   **LLR-PSF08 (Ref: HLR-017):** The function shall construct a `ctags` command string using `snprintf()` and store it in the `command` buffer. The format of the command shall be: `"ctags -o - --c-kinds=f --fields=+ne %s | grep '^%s'"`, where the first `%s` is replaced by `source_file` and the second `%s` by `function_name`.
*   **LLR-PSF09 (Ref: HLR-017):** The function shall call `execute_command()` with the `command` string, the `ctags_output` buffer, and `sizeof(ctags_output)` as arguments.
*   **LLR-PSF10 (Ref: HLR-017, HLR-024):** If `execute_command()` returns `false` (indicating failure to execute or capture output):
    *   A formatted error message "Error: Could not find function '%s' in file '%s'.\n" shall be printed to `stderr` using `fprintf()`, where the placeholders are filled with `function_name` and `source_file` respectively.
    *   The function shall then immediately return.
*   **LLR-PSF11 (Ref: HLR-017):** The function shall call `parse_ctags_output()` with the `ctags_output` string, and pointers to `start_line` and `end_line` as arguments.
*   **LLR-PSF12 (Ref: HLR-017, HLR-024):** If `parse_ctags_output()` returns `false` (indicating failure to parse the ctags output):
    *   A formatted error message "Error: Failed to parse ctags output for function '%s'.\n" shall be printed to `stderr` using `fprintf()`, where the placeholder is filled with `function_name`.
    *   The function shall then immediately return.
*   **LLR-PSF13 (Ref: HLR-017):** The function shall increment the `end_line` variable by `1` (to make the loop condition for printing inclusive of the ctags end line).
*   **LLR-PSF14 (Ref: HLR-017, HLR-024):** The function shall validate the adjusted line range. It will check if `start_line` is less than or equal to `0`, OR if `end_line` (after increment) is less than or equal to `0`, OR if `start_line` is greater than `end_line`.
*   **LLR-PSF15 (Ref: HLR-017, HLR-024):** If any of the conditions in LLR-PSF14 are true (indicating an invalid line range):
    *   A formatted error message "Error: Invalid line range for function '%s' (start: %d, end: %d).\n" shall be printed to `stderr` using `fprintf()`, where the placeholders are filled with `function_name`, `start_line`, and the (incremented) `end_line` respectively.
    *   The function shall then immediately return.
*   **LLR-PSF16 (Ref: HLR-016):** The function shall attempt to open the file specified by `source_file` in read-only mode (`"r"`) using `fopen()`. The result shall be stored in a local `FILE *file` variable.
*   **LLR-PSF17 (Ref: HLR-016, HLR-024):** If `fopen()` returns `NULL` (indicating an error opening the source file):
    *   The function shall call `perror("fopen")` to print a system error message to `stderr`.
    *   The function shall then immediately return.
*   **LLR-PSF18 (Ref: HLR-018):** The function shall declare a local character array `line` of size `MAX_LINE_LENGTH`.
*   **LLR-PSF19 (Ref: HLR-018):** The function shall declare a local integer variable `current_line` and initialize it to `1`.
*   **LLR-PSF20 (Ref: HLR-018):** The function shall enter a `while` loop that continues as long as `current_line` (incremented before comparison) is less than `start_line` AND `fgets()` successfully reads a line from `file` into the `line` buffer. This loop advances the file pointer to the start of the function.
*   **LLR-PSF21 (Ref: HLR-018):** The function shall enter a second `while` loop that continues as long as `current_line` (incremented before comparison) is less than or equal to `end_line` AND `fgets()` successfully reads a line from `file` into the `line` buffer.
*   **LLR-PSF22 (Ref: HLR-018):** Inside the second `while` loop (LLR-PSF21), the function shall print to `stdout` using `printf()`.
    *   The first character printed shall be `'>'` if `current_line` is equal to the input `line_number` (the line to highlight).
    *   Otherwise, the first character printed shall be a space `' '`.
    *   This character shall be followed by the `current_line` number, formatted to take 4 spaces (e.g., using `"%4d "`).
    *   Finally, the content of the `line` buffer (the source code line itself) shall be printed.
*   **LLR-PSF23 (Ref: HLR-016):** After the loops complete (or if `fopen` failed earlier and the function returned), if `file` is not `NULL`, the function shall call `fclose(file)` to close the source file stream.

## Low-Level Requirements (LLRs) for the initialize_parse_state() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific operations for initializing a `ParseState` structure to its default values at the beginning of log file processing.

*   **LLR-IPS01 (Ref: HLR-023):** The `initialize_parse_state()` function shall be defined with the signature `void initialize_parse_state(ParseState *state)`.
*   **LLR-IPS02 (Ref: HLR-023):** The function shall first check if the input `state` pointer is `NULL`.
*   **LLR-IPS03 (Ref: HLR-023):** If the `state` pointer is `NULL`, the function shall immediately return without performing any further actions.
*   **LLR-IPS04 (Ref: HLR-023):** If the `state` pointer is not `NULL`, the function shall set the `state->in_error_block` member to `false`.
*   **LLR-IPS05 (Ref: HLR-023):** The function shall set the `state->print_function` member to `false`.
*   **LLR-IPS06 (Ref: HLR-023):** The function shall set the `state->stack_lines_shown` member to `0`.
*   **LLR-IPS07 (Ref: HLR-023):** The function shall set the `state->user_code_found_for_error` member to `false`.
*   **LLR-IPS08 (Ref: HLR-023):** The function shall set the first character of the `state->current_error_type` character array to the null terminator (`\0`), effectively initializing it as an empty string.
*   **LLR-IPS09 (Ref: HLR-023):** The function shall set the first character of the `state->error_filename` character array to the null terminator (`\0`), effectively initializing it as an empty string.
*   **LLR-IPS10 (Ref: HLR-023):** The function shall set the first character of the `state->error_function_name` character array to the null terminator (`\0`), effectively initializing it as an empty string.
*   **LLR-IPS11 (Ref: HLR-023):** The function shall set the `state->error_line_number` member to `-1`.
*   **LLR-IPS12 (Ref: HLR-023, HLR-021):** The function shall set the `state->error_count` member to `0`.

## Low-Level Requirements (LLRs) for the check_start_new_error() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific logic for determining if a given log line signifies the beginning of a new Valgrind error report block and for initializing the parser state accordingly.

*   **LLR-CSNE01 (Ref: HLR-011):** The `check_start_new_error()` function shall be defined with the signature `bool check_start_new_error(const char *line_content, ParseState *state)`.
*   **LLR-CSNE02 (Ref: HLR-011):** The function shall first check if either the input `line_content` pointer is `NULL` OR the input `state` pointer is `NULL`.
*   **LLR-CSNE03 (Ref: HLR-011):** If the condition in LLR-CSNE02 is true (i.e., an invalid argument is detected), the function shall immediately return `false`.
*   **LLR-CSNE04 (Ref: HLR-011, HLR-023):** The function shall check if the `state->in_error_block` member is `false`. If `state->in_error_block` is `true`, the function shall immediately return `false` (as a new error cannot start if already within an error block).
*   **LLR-CSNE05 (Ref: HLR-011):** If `state->in_error_block` is `false`, the function shall iterate through the `ERROR_KEYWORDS` global array (defined in `/home/john/Projects/Valgrind_Parser/vgp.c`). The iteration shall continue as long as the current array element is not `NULL`.
*   **LLR-CSNE06 (Ref: HLR-011):** For each keyword string in the `ERROR_KEYWORDS` array, the function shall use `strstr(line_content, ERROR_KEYWORDS[i])` to check if the keyword is present as a substring within the input `line_content`.
*   **LLR-CSNE07 (Ref: HLR-011, HLR-012):** If a keyword is found in `line_content` (i.e., `strstr()` returns a non-`NULL` pointer):
    *   The function shall call `print_error_header(line_content, state)` to print the standard error block header using the current `line_content`.
*   **LLR-CSNE08 (Ref: HLR-011, HLR-023):** If a keyword is found, the function shall copy the matched `ERROR_KEYWORDS[i]` string into the `state->current_error_type` character array using `strncpy()`. The maximum number of characters to copy shall be `sizeof(state->current_error_type) - 1` to ensure space for a null terminator.
*   **LLR-CSNE09 (Ref: HLR-011, HLR-023):** After copying the keyword with `strncpy()`, the function shall ensure null termination by setting `state->current_error_type[sizeof(state->current_error_type) - 1]` to `'\0'`.
*   **LLR-CSNE10 (Ref: HLR-011, HLR-023):** If a keyword is found, the function shall set the `state->in_error_block` member to `true`.
*   **LLR-CSNE11 (Ref: HLR-011, HLR-023):** If a keyword is found, the function shall reset `state->stack_lines_shown` to `0`.
*   **LLR-CSNE12 (Ref: HLR-011, HLR-023):** If a keyword is found, the function shall reset `state->user_code_found_for_error` to `false`.
*   **LLR-CSNE13 (Ref: HLR-011, HLR-023):** If a keyword is found, the function shall reset `state->print_function` to `false`.
*   **LLR-CSNE14 (Ref: HLR-011):** If a keyword is found, the function shall immediately return `true` (indicating a new error block has started).
*   **LLR-CSNE15 (Ref: HLR-011):** If the loop through all `ERROR_KEYWORDS` completes without finding any matching keyword in `line_content`, the function shall return `false`.

## Low-Level Requirements (LLRs) for the process_stack_trace_line() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific logic for processing a single line that has been identified as part of a Valgrind stack trace, determining whether to display it, and extracting information if it's user code.

*   **LLR-PSTL01 (Ref: HLR-013, HLR-023):** The `process_stack_trace_line()` function shall be defined with the signature `void process_stack_trace_line(const char *line_content, ParseState *state)`.
*   **LLR-PSTL02 (Ref: HLR-013, HLR-023):** The function shall first check if either the input `line_content` pointer is `NULL` OR the input `state` pointer is `NULL`.
*   **LLR-PSTL03 (Ref: HLR-013, HLR-023):** If the condition in LLR-PSTL02 is true (i.e., an invalid argument is detected), the function shall immediately return without performing any further actions.
*   **LLR-PSTL04 (Ref: HLR-013, HLR-014, HLR-015, HLR-023):** The function shall evaluate a primary condition: if `state->stack_lines_shown` is less than `STACK_TRACE_CONTEXT_LINES` (defined in `/home/john/Projects/Valgrind_Parser/vgp.h`) OR if `is_user_code_stack_trace(line_content)` returns `true`.
*   **LLR-PSTL05 (Ref: HLR-013, HLR-023, HLR-025):** If the condition in LLR-PSTL04 is true:
    *   A local character array `stack_entry` of size `MAX_LINE_LENGTH` (defined in `/home/john/Projects/Valgrind_Parser/vgp.h`) shall be declared.
    *   The function shall check if `app_config.print_stack` is `true` OR `app_config.verbose` is `true`.
        *   If this sub-condition is true, the function shall call `get_function_name(line_content, stack_entry)` to format the stack trace line.
        *   The function shall then print the content of `stack_entry` to `stdout`, prefixed with "  - " (e.g., using `printf("  - %s", stack_entry)`).
        *   The `state->stack_lines_shown` member shall be incremented by one.
*   **LLR-PSTL06 (Ref: HLR-014, HLR-016, HLR-023):** If the condition in LLR-PSTL04 is true, the function shall then evaluate a secondary condition: if `state->user_code_found_for_error` is `false` AND `is_user_code_stack_trace(line_content)` returns `true`.
    *   If this secondary condition is true:
        *   The `state->user_code_found_for_error` member shall be set to `true`.
        *   The function shall call `extract_file_and_line(line_content, state->error_filename, state->error_function_name, &state->error_line_number)`.
        *   The boolean result of the `extract_file_and_line()` call shall be assigned to the `state->print_function` member.
*   **LLR-PSTL07 (Ref: HLR-015, HLR-023, HLR-025):** If the condition in LLR-PSTL04 is `false`, the function shall then evaluate an alternative condition: if `state->user_code_found_for_error` is `false` AND `state->stack_lines_shown` is equal to `STACK_TRACE_CONTEXT_LINES`.
    *   If this alternative condition is true:
        *   The function shall check if `app_config.print_stack` is `true` OR `app_config.verbose` is `true`.
            *   If this sub-condition is true, the function shall print the string "  - ...\n" to `stdout`.
            *   The `state->stack_lines_shown` member shall be incremented by one (to prevent printing the ellipsis multiple times for subsequent non-user-code lines).

## Low-Level Requirements (LLRs) for the finalize_error_block() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific operations for concluding the processing of a Valgrind error block, which includes potentially printing source code and resetting relevant parser state flags.

*   **LLR-FEB01 (Ref: HLR-023):** The `finalize_error_block()` function shall be defined with the signature `void finalize_error_block(ParseState *state)`.
*   **LLR-FEB02 (Ref: HLR-023):** The function shall first check if the input `state` pointer is `NULL`.
*   **LLR-FEB03 (Ref: HLR-023):** If the `state` pointer is `NULL`, the function shall immediately return without performing any further actions.
*   **LLR-FEB04 (Ref: HLR-015, HLR-023, HLR-024):** If the `state` pointer is not `NULL`, the function shall check if `state->user_code_found_for_error` is `false` AND `state->stack_lines_shown` is greater than `0`.
    *   If this condition is true, the function shall print a formatted message to `stdout` using `printf()`. The message shall be: `"  (-> Check stack trace above for user code related to '%s')\n"`, where `%s` is replaced by the content of `state->current_error_type`.
*   **LLR-FEB05 (Ref: HLR-016, HLR-018, HLR-023):** The function shall then check if `state->print_function` is `true`.
    *   If `state->print_function` is `true`:
        *   A header message shall be printed to `stdout` using `printf()`. The message shall be: `"Source (%s:%d)\n"`, where the first `%s` is replaced by `state->error_filename` and `%d` is replaced by `state->error_line_number`.
*   **LLR-FEB06 (Ref: HLR-018, HLR-025):** If `state->print_function` is `true` (continuing from LLR-FEB05), the function shall then check if `app_config.print_source` is `true` OR `app_config.verbose` is `true`.
    *   If this sub-condition is true:
        *   The function shall call `print_source_function(state->error_filename, state->error_function_name, state->error_line_number)`.
        *   A newline character (`\n`) shall be printed to `stdout` using `printf()`.
*   **LLR-FEB07 (Ref: HLR-023):** After the conditional printing logic, the function shall set the `state->in_error_block` member to `false`.
*   **LLR-FEB08 (Ref: HLR-023):** The function shall set the `state->print_function` member to `false`.

## Low-Level Requirements (LLRs) for the process_in_error_block() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific logic for handling a line of text when the parser is currently within an identified Valgrind error block.

*   **LLR-PIEB01 (Ref: HLR-023):** The `process_in_error_block()` function shall be defined with the signature `void process_in_error_block(const char *line_content, ParseState *state)`.
*   **LLR-PIEB02 (Ref: HLR-023):** The function shall first check if either the input `line_content` pointer is `NULL` OR the input `state` pointer is `NULL`.
*   **LLR-PIEB03 (Ref: HLR-023):** If the condition in LLR-PIEB02 is true (i.e., an invalid argument is detected), the function shall immediately return without performing any further actions.
*   **LLR-PIEB04 (Ref: HLR-011, HLR-013):** The function shall check if the `line_content` string starts with the prefix "   at " (6 characters) using `strncmp()`.
*   **LLR-PIEB05 (Ref: HLR-011, HLR-013):** If the condition in LLR-PIEB04 is false, the function shall then check if the `line_content` string starts with the prefix "   by " (6 characters) using `strncmp()`.
*   **LLR-PIEB06 (Ref: HLR-013, HLR-023):** If either the condition in LLR-PIEB04 is true OR the condition in LLR-PIEB05 is true (indicating the line is part of a stack trace):
    *   The function shall call `process_stack_trace_line(line_content, state)` to handle the specific processing of this stack trace line.
*   **LLR-PIEB07 (Ref: HLR-011, HLR-016, HLR-018, HLR-023):** If both the condition in LLR-PIEB04 is false AND the condition in LLR-PIEB05 is false (indicating the line is not part of a stack trace and thus signifies the end of the current error's stack trace information):
    *   The function shall call `finalize_error_block(state)` to conclude the processing of the current error block (which may include printing source code and resetting state flags).

## Low-Level Requirements (LLRs) for the process_summary_lines() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific logic for identifying and processing lines that belong to Valgrind's summary sections, such as leak summaries and the final error summary.

*   **LLR-PSL01 (Ref: HLR-019, HLR-020, HLR-021):** The `process_summary_lines()` function shall be defined with the signature `void process_summary_lines(const char *line_content, ParseState *state)`.
*   **LLR-PSL02 (Ref: N/A - Defensive Programming):** The function shall first check if the input `line_content` pointer is `NULL`. If it is `NULL`, the function shall immediately return without performing any further actions. (Note: The `state` pointer is also an argument, but it's only passed through to `print_final_error_summary`; a `NULL` check for `state` here would also be good practice, though the current code doesn't explicitly do it before its potential use in the called function).
*   **LLR-PSL03 (Ref: HLR-019):** The function shall use `strstr(line_content, "LEAK SUMMARY:")` to check if the `line_content` contains the "LEAK SUMMARY:" keyword.
*   **LLR-PSL04 (Ref: HLR-019, HLR-025):** If the condition in LLR-PSL03 is true:
    *   The function shall check if the `app_config.print_leak_summary` global boolean flag is `true` OR the `app_config.verbose` global boolean flag is `true`.
    *   If this sub-condition is true, the function shall print the string "\n--- LEAK SUMMARY ---\n" to `stdout` using `printf()`.
*   **LLR-PSL05 (Ref: HLR-020):** If the condition in LLR-PSL03 is false, the function shall then use `strstr(line_content, "definitely lost:")` to check if the `line_content` contains the "definitely lost:" keyword.
*   **LLR-PSL06 (Ref: HLR-020, HLR-025):** If the condition in LLR-PSL05 is true:
    *   The function shall check if the `app_config.print_leak_summary` global boolean flag is `true` OR the `app_config.verbose` global boolean flag is `true`.
    *   If this sub-condition is true, the function shall call `print_leak_summary_line(line_content, "Definitely Lost")`.
*   **LLR-PSL07 (Ref: HLR-020):** If the conditions in LLR-PSL03 and LLR-PSL05 are false, the function shall then use `strstr(line_content, "indirectly lost:")` to check if the `line_content` contains the "indirectly lost:" keyword.
*   **LLR-PSL08 (Ref: HLR-020, HLR-025):** If the condition in LLR-PSL07 is true:
    *   The function shall check if the `app_config.print_leak_summary` global boolean flag is `true` OR the `app_config.verbose` global boolean flag is `true`.
    *   If this sub-condition is true, the function shall call `print_leak_summary_line(line_content, "Indirectly Lost")`.
*   **LLR-PSL09 (Ref: HLR-020):** If the conditions in LLR-PSL03, LLR-PSL05, and LLR-PSL07 are false, the function shall then use `strstr(line_content, "possibly lost:")` to check if the `line_content` contains the "possibly lost:" keyword.
*   **LLR-PSL10 (Ref: HLR-020, HLR-025):** If the condition in LLR-PSL09 is true:
    *   The function shall check if the `app_config.print_leak_summary` global boolean flag is `true` OR the `app_config.verbose` global boolean flag is `true`.
    *   If this sub-condition is true, the function shall call `print_leak_summary_line(line_content, "Possibly Lost")`.
*   **LLR-PSL11 (Ref: HLR-020):** If the conditions in LLR-PSL03, LLR-PSL05, LLR-PSL07, and LLR-PSL09 are false, the function shall then use `strstr(line_content, "still reachable:")` to check if the `line_content` contains the "still reachable:" keyword.
*   **LLR-PSL12 (Ref: HLR-020, HLR-025):** If the condition in LLR-PSL11 is true:
    *   The function shall check if the `app_config.print_leak_summary` global boolean flag is `true` OR the `app_config.verbose` global boolean flag is `true`.
    *   If this sub-condition is true, the function shall call `print_leak_summary_line(line_content, "Still Reachable")`.
*   **LLR-PSL13 (Ref: HLR-021):** If the conditions in LLR-PSL03, LLR-PSL05, LLR-PSL07, LLR-PSL09, and LLR-PSL11 are false, the function shall then use `strstr(line_content, "ERROR SUMMARY:")` to check if the `line_content` contains the "ERROR SUMMARY:" keyword.
*   **LLR-PSL14 (Ref: HLR-021):** If the condition in LLR-PSL13 is true, the function shall call `print_final_error_summary(line_content, state)`.

## Low-Level Requirements (LLRs) for the process_log_file() function in /home/john/Projects/Valgrind_Parser/vgp.c

These LLRs detail the specific operations for reading and processing a Valgrind log file line by line, orchestrating the parsing logic based on the current state and line content.

*   **LLR-PLF01 (Ref: HLR-005):** The `process_log_file()` function shall be defined with the signature `void process_log_file(FILE *file)`.
*   **LLR-PLF02 (Ref: HLR-005):** The function shall first check if the input `file` pointer is `NULL`.
*   **LLR-PLF03 (Ref: HLR-005):** If the `file` pointer is `NULL`, the function shall immediately return without performing any further actions.
*   **LLR-PLF04 (Ref: HLR-023):** The function shall declare a local character array `line` of size `MAX_LINE_LENGTH` (defined in `/home/john/Projects/Valgrind_Parser/vgp.h`).
*   **LLR-PLF05 (Ref: HLR-023):** The function shall declare a local `ParseState` variable named `state`.
*   **LLR-PLF06 (Ref: HLR-023):** The function shall call `initialize_parse_state(&state)` to set the `state` variable to its default initial values.
*   **LLR-PLF07 (Ref: HLR-005, HLR-010, HLR-011, HLR-013, HLR-019, HLR-020, HLR-021):** The function shall enter a `while` loop that continues as long as `fgets(line, sizeof(line), file)` successfully reads a line from the input `file` stream into the `line` buffer.
*   **LLR-PLF08 (Ref: HLR-010):** Inside the `while` loop, the function shall call `strip_valgrind_pid_prefix(line)` and assign the returned pointer to a local `char *line_content` variable.
*   **LLR-PLF09 (Ref: N/A - Internal Logic):** Inside the `while` loop, the function shall check if `state.in_error_block` is `false` AND if the `line_content` consists only of whitespace characters (space, tab, newline, carriage return) using `strspn(line_content, " \t\n\r") == strlen(line_content)`.
    *   If this condition is true, the loop shall `continue` to the next iteration, effectively skipping empty or whitespace-only lines when not inside an error block.
*   **LLR-PLF10 (Ref: HLR-011, HLR-013, HLR-016, HLR-018, HLR-023):** Inside the `while` loop, the function shall check if `state.in_error_block` is `true`.
    *   If `state.in_error_block` is `true`, the function shall call `process_in_error_block(line_content, &state)`.
*   **LLR-PLF11 (Ref: HLR-011, HLR-012, HLR-023):** Inside the `while` loop, after the check in LLR-PLF10 (or if `state.in_error_block` was initially false), the function shall check if `state.in_error_block` is `false` (it might have been set to false by `process_in_error_block` calling `finalize_error_block`).
    *   If `state.in_error_block` is `false`, the function shall call `check_start_new_error(line_content, &state)` and store its boolean result (e.g., in a local `bool new_error_started` variable).
    *   If `check_start_new_error()` returned `true`, the loop shall `continue` to the next iteration.
*   **LLR-PLF12 (Ref: HLR-019, HLR-020, HLR-021, HLR-023):** Inside the `while` loop, after the checks in LLR-PLF10 and LLR-PLF11 (or if `state.in_error_block` was false and `check_start_new_error` returned false), the function shall check if `state.in_error_block` is `false`.
    *   If `state.in_error_block` is `false`, the function shall call `process_summary_lines(line_content, &state)`.
*   **LLR-PLF13 (Ref: HLR-011, HLR-016, HLR-018, HLR-023):** After the `while` loop terminates (i.e., `fgets()` returns `NULL`, indicating end-of-file or an error), the function shall check if `state.in_error_block` is `true`.
    *   If `state.in_error_block` is `true`, the function shall call `finalize_error_block(&state)`.
    *   If `state.in_error_block` was `true` (and `finalize_error_block` was called), the function shall print "----------------------------------------\n\n" to `stdout` using `printf()`.
