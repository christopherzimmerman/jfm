#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Reads the entire contents of a file into a null-terminated string.
 * Allocates memory for the file contents.
 * 
 * @param path The file path to read
 * @return Dynamically allocated string with file contents, or NULL on error
 */
char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    size_t read = fread(buffer, 1, size, file);
    buffer[read] = '\0';
    
    fclose(file);
    return buffer;
}

/**
 * Creates a duplicate of a string by allocating new memory.
 * Equivalent to POSIX strdup but portable.
 * 
 * @param str The string to duplicate
 * @return Newly allocated copy of the string, or NULL on error
 */
char* string_duplicate(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* copy = malloc(len + 1);
    if (copy) {
        strcpy(copy, str);
    }
    return copy;
}

/**
 * Creates a duplicate of up to n characters from a string.
 * Equivalent to POSIX strndup but portable.
 * 
 * @param str The string to duplicate
 * @param n Maximum number of characters to copy
 * @return Newly allocated copy of the string, or NULL on error
 */
char* string_n_duplicate(const char* str, size_t n) {
    if (!str) return NULL;
    char* copy = malloc(n + 1);
    if (copy) {
        strncpy(copy, str, n);
        copy[n] = '\0';
    }
    return copy;
}