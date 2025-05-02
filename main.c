/*
 * main.c
 *
 * Valgrind output parser
 * 
 * Entry point for the Valgrind output parser program.
 *  
 * Created: 04/01/2025
 * Author : john anderson
 *
 * Copyright (C) 2025 by John Anderson <racerxr650r@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any 
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */ 
#include "vgp.h"

// --- Main Entry Point ---
int main(int argc, char *argv[])
{
    setlocale(LC_NUMERIC, "");
    
    if (argc != 2)
    {
        printF("Usage: %s <valgrind_log_file>\n", argv[0]);
        printF("Parses a Valgrind log and shows the call stack and source code for the functions with errors.\n");
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        // Use strerror for better error reporting
        fprintf(stderr, "Error opening file '%s': %s\n", filename, strerror(errno));
        return EXIT_FAILURE;
    }

    printF("--- Valgrind Log Summary ---\n");
    printF("Parsing Log File: %s\n", filename);

    process_log_file(file); // Call the main processing function

    fclose(file);
    printF("\n--- End of Summary ---\n");
    return EXIT_SUCCESS;
}
