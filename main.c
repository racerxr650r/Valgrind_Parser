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

void parse_command_line(int argc, char *argv[])
{
    for(int i = 1; i < argc; i++)
    {
        if(argv[i][0] == '-')
        {
            switch(argv[i][1])
            {
                case 'v':
                    app_config.verbose = true;
                    break;
                case 's':
                    app_config.print_source = true;
                    break;
                case 'l':
                    app_config.print_leak_summary = true;
                    break;
                case 't':
                    app_config.print_stack = true;
                    break;
                case 'h':
                    printF("Usage: %s [options] <valgrind_log_file>\n", argv[0]);
                    printF("Options:\n");
                    printF("  -v : Enable verbose output\n");
                    printF("  -s : Print source code\n");
                    printF("  -l : Print leak summary\n");
                    printF("  -t : Print stack trace\n");
                    printF("  -h : Show this help message\n");
                    exit(EXIT_SUCCESS);
                default:
                    fprintf(stderr, "Unknown option: %s\n", argv[i]);
                    exit(EXIT_FAILURE);
            }
        }
        else
        {
            if(!app_config.log_file)
                app_config.log_file = argv[i];
            else
            {
                fprintf(stderr, "Multiple log files specified: %s and %s\n", app_config.log_file, argv[i]);
                exit(EXIT_FAILURE);
            }
        }
    }
    if(!app_config.log_file)
    {
        fprintf(stderr, "No log file specified. Use -h for help.\n");
        exit(EXIT_FAILURE);
    }
}

// --- Main Entry Point ---
int main(int argc, char *argv[])
{
    setlocale(LC_NUMERIC, "");

    parse_command_line(argc, argv); // Parse command line arguments

    const char *filename = app_config.log_file;
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        // Use strerror for better error reporting
        fprintf(stderr, "Error opening file '%s': %s\n", filename, strerror(errno));
        return EXIT_FAILURE;
    }

    printF("Parsing Valgrind Log File: %s\n", filename);

    process_log_file(file); // Call the main processing function

    fclose(file);
    return EXIT_SUCCESS;
}
