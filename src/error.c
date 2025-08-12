#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define isatty _isatty
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#endif

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_DIM     "\033[2m"

static bool colors_enabled = true;
static bool colors_initialized = false;

/**
 * Initialize Windows console for ANSI color support
 */
static void init_windows_console(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
    DWORD dwMode = 0;
    
    if (hOut != INVALID_HANDLE_VALUE) {
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }
    
    if (hErr != INVALID_HANDLE_VALUE) {
        GetConsoleMode(hErr, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hErr, dwMode);
    }
#endif
}

/**
 * Check if colors should be enabled based on terminal support
 */
static void init_colors(void) {
    if (colors_initialized) return;
    colors_initialized = true;

    #ifdef _WIN32
    // fileno is defined as a macro in MinGW's stdio.h, use it directly
    if (!isatty(stderr->_file)) {
    #else
    if (!isatty(fileno(stderr))) {
    #endif
        colors_enabled = false;
        return;
    }

    init_windows_console();

    if (getenv("NO_COLOR")) {
        colors_enabled = false;
    }
}

/**
 * Enable colored output
 */
void enable_colors(void) {
    colors_enabled = true;
    init_colors();
}

/**
 * Disable colored output
 */
void disable_colors(void) {
    colors_enabled = false;
}

/**
 * Creates a new error list for collecting compilation errors.
 * 
 * @return Newly allocated error list
 */
ErrorList* error_list_create(void) {
    ErrorList* list = malloc(sizeof(ErrorList));
    list->errors = NULL;
    list->error_count = 0;
    list->error_capacity = 0;
    list->source_code = NULL;
    init_colors();
    return list;
}

/**
 * Destroys an error list and frees all associated memory.
 * Frees all error message strings and the list structure.
 * 
 * @param list The error list to destroy
 */
void error_list_destroy(ErrorList* list) {
    if (!list) return;
    for (size_t i = 0; i < list->error_count; i++) {
        free((char*)list->errors[i].message);
    }
    free(list->errors);
    free(list);
}

/**
 * Adds a new error to the error list.
 * Automatically expands the list capacity as needed.
 * 
 * @param list The error list
 * @param message The error message (will be duplicated)
 * @param file The source file name
 * @param line The line number
 * @param column The column number
 */
void error_list_add(ErrorList* list, const char* message, const char* file, size_t line, size_t column) {
    if (list->error_count >= list->error_capacity) {
        list->error_capacity = list->error_capacity == 0 ? 8 : list->error_capacity * 2;
        list->errors = realloc(list->errors, sizeof(Error) * list->error_capacity);
    }

    char* msg_copy = malloc(strlen(message) + 1);
    strcpy(msg_copy, message);
    
    Error error = {
        .message = msg_copy,
        .file = file,
        .line = line,
        .column = column
    };
    
    list->errors[list->error_count++] = error;
}

/**
 * Prints all errors in the list to stderr.
 * Formats each error with message and location information.
 * 
 * @param list The error list to print
 */
void error_list_print(ErrorList* list) {
    for (size_t i = 0; i < list->error_count; i++) {
        Error* e = &list->errors[i];
        fprintf(stderr, "Error: %s\n", e->message);
        if (e->file) {
            fprintf(stderr, "  --> %s:%lu:%lu\n", e->file, (unsigned long)e->line, (unsigned long)e->column);
        }
    }
}

/**
 * Reports a single error directly to stderr.
 * Utility function for immediate error reporting.
 * 
 * @param message The error message
 * @param file The source file name
 * @param line The line number
 * @param column The column number
 */
/**
 * Set the source code for the error list
 */
void error_list_set_source(ErrorList* list, const char* source) {
    if (list) {
        list->source_code = source;
    }
}

/**
 * Extract a line from source code
 */
static const char* get_line_from_source(const char* source, size_t line_num, size_t* line_length) {
    if (line_num == 0) return NULL;
    
    size_t current_line = 1;
    const char* line_start;
    const char* p = source;
    
    while (*p) {
        if (current_line == line_num) {
            line_start = p;
            while (*p && *p != '\n' && *p != '\r') p++;
            *line_length = p - line_start;
            return line_start;
        }
        
        if (*p == '\n') {
            current_line++;
            p++;
            if (*p == '\r') p++;
        } else if (*p == '\r') {
            current_line++;
            p++;
            if (*p == '\n') p++;
        } else {
            p++;
        }
    }
    
    return NULL;
}

/**
 * Print beautiful error with source code snippet
 */
void error_report_beautiful(const char* message, const char* file, size_t line, size_t column, const char* source) {
    init_colors();

    if (colors_enabled) {
        fprintf(stderr, "%s%serror%s: %s\n", COLOR_BOLD, COLOR_RED, COLOR_RESET, message);
    } else {
        fprintf(stderr, "error: %s\n", message);
    }

    if (file) {
        if (colors_enabled) {
            fprintf(stderr, " %s-->%s %s%s:%lu:%lu%s\n", 
                    COLOR_BLUE, COLOR_RESET,
                    COLOR_CYAN, file, (unsigned long)line, (unsigned long)column, COLOR_RESET);
        } else {
            fprintf(stderr, " --> %s:%lu:%lu\n", file, (unsigned long)line, (unsigned long)column);
        }
    }

    if (source && line > 0) {
        size_t line_length = 0;
        const char* line_text = get_line_from_source(source, line, &line_length);
        
        if (line_text) {
            char line_str[32];
            snprintf(line_str, sizeof(line_str), "%lu", (unsigned long)line);
            size_t line_num_width = strlen(line_str);

            if (colors_enabled) {
                fprintf(stderr, " %s%*s |%s\n", COLOR_BLUE, (int)line_num_width, "", COLOR_RESET);
            } else {
                fprintf(stderr, " %*s |\n", (int)line_num_width, "");
            }

            if (colors_enabled) {
                fprintf(stderr, " %s%*lu |%s ", COLOR_BLUE, (int)line_num_width, (unsigned long)line, COLOR_RESET);
            } else {
                fprintf(stderr, " %*lu | ", (int)line_num_width, (unsigned long)line);
            }

            fwrite(line_text, 1, line_length, stderr);
            fprintf(stderr, "\n");

            if (column > 0) {
                if (colors_enabled) {
                    fprintf(stderr, " %s%*s |%s ", COLOR_BLUE, (int)line_num_width, "", COLOR_RESET);
                } else {
                    fprintf(stderr, " %*s | ", (int)line_num_width, "");
                }

                for (size_t i = 1; i < column && i <= line_length; i++) {
                    if (line_text[i-1] == '\t') {
                        fprintf(stderr, "\t");
                    } else {
                        fprintf(stderr, " ");
                    }
                }

                if (colors_enabled) {
                    fprintf(stderr, "%s%s^%s\n", COLOR_BOLD, COLOR_RED, COLOR_RESET);
                } else {
                    fprintf(stderr, "^\n");
                }
            }
        }
    }
    
    fprintf(stderr, "\n");
}

/**
 * Print all errors in beautiful format
 */
void error_list_print_beautiful(ErrorList* list) {
    if (!list || list->error_count == 0) return;
    
    for (size_t i = 0; i < list->error_count; i++) {
        Error* e = &list->errors[i];
        error_report_beautiful(e->message, e->file, e->line, e->column, 
                             list->source_code ? list->source_code : e->source_code);
    }

    if (list->error_count > 1) {
        if (colors_enabled) {
            fprintf(stderr, "%s%serror%s: aborting due to %lu previous errors\n",
                    COLOR_BOLD, COLOR_RED, COLOR_RESET, (unsigned long)list->error_count);
        } else {
            fprintf(stderr, "error: aborting due to %lu previous errors\n", 
                    (unsigned long)list->error_count);
        }
    }
}

void error_report(const char* message, const char* file, size_t line, size_t column) {
    fprintf(stderr, "Error: %s\n", message);
    if (file) {
        fprintf(stderr, "  --> %s:%lu:%lu\n", file, (unsigned long)line, (unsigned long)column);
    }
}