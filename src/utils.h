#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

char* read_file(const char* path);
char* string_duplicate(const char* str);
char* string_n_duplicate(const char* str, size_t n);

#endif