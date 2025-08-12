#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>
#include <stddef.h>
#include "lexer.h"

typedef enum {
    TYPE_I8,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,
    TYPE_U8,
    TYPE_U16,
    TYPE_U32,
    TYPE_U64,
    TYPE_F32,
    TYPE_F64,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_STR,
    TYPE_VOID,
    TYPE_ARRAY,
    TYPE_POINTER,
    TYPE_REFERENCE,
    TYPE_STRUCT,
    TYPE_UNKNOWN,
} TypeKind;

typedef struct Type {
    TypeKind kind;
    
    union {
        struct {
            struct Type* element_type;
            size_t size;
        } array;
        
        struct {
            struct Type* pointed_type;
        } pointer;
        
        struct {
            struct Type* referenced_type;
            bool is_mutable;
        } reference;
        
        struct {
            char* name;
        } struct_type;
    } data;
} Type;

Type* type_create(TypeKind kind);
void type_destroy(Type* type);
bool type_equals(Type* a, Type* b);
const char* type_to_string(Type* type);
Type* type_from_token(TokenType token);

#endif