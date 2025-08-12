#ifndef ERROR_H
#define ERROR_H

#include <stddef.h>

typedef struct {
    const char* message;
    const char* file;
    size_t line;
    size_t column;
    const char* source_code;
} Error;

typedef struct {
    Error* errors;
    size_t error_count;
    size_t error_capacity;
    const char* source_code;
} ErrorList;

ErrorList* error_list_create(void);
void error_list_destroy(ErrorList* list);
void error_list_add(ErrorList* list, const char* message, const char* file, size_t line, size_t column);
void error_list_print(ErrorList* list);
void error_list_print_beautiful(ErrorList* list);
void error_list_set_source(ErrorList* list, const char* source);

void error_report(const char* message, const char* file, size_t line, size_t column);
void error_report_beautiful(const char* message, const char* file, size_t line, size_t column, const char* source);

void enable_colors(void);
void disable_colors(void);

#endif