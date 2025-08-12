#include "type.h"
#include "lexer.h"
#include <stdlib.h>
#include <string.h>

/**
 * Creates a new type with the specified kind.
 * 
 * @param kind The type kind to create
 * @return Newly allocated type
 */
Type* type_create(TypeKind kind) {
    Type* type = calloc(1, sizeof(Type));
    type->kind = kind;
    return type;
}

/**
 * Destroys a type and frees its memory.
 * 
 * @param type The type to destroy
 */
void type_destroy(Type* type) {
    if (!type) return;
    free(type);
}

/**
 * Checks if two types are equal.
 * Currently only compares type kinds.
 * 
 * @param a First type to compare
 * @param b Second type to compare
 * @return true if types are equal, false otherwise
 */
bool type_equals(Type* a, Type* b) {
    if (!a || !b) return false;
    return a->kind == b->kind;
}

/**
 * Converts a type to its string representation.
 * 
 * @param type The type to convert
 * @return String representation of the type
 */
const char* type_to_string(Type* type) {
    if (!type) return "unknown";
    
    switch (type->kind) {
        case TYPE_I8: return "i8";
        case TYPE_I16: return "i16";
        case TYPE_I32: return "i32";
        case TYPE_I64: return "i64";
        case TYPE_U8: return "u8";
        case TYPE_U16: return "u16";
        case TYPE_U32: return "u32";
        case TYPE_U64: return "u64";
        case TYPE_F32: return "f32";
        case TYPE_F64: return "f64";
        case TYPE_BOOL: return "bool";
        case TYPE_CHAR: return "char";
        case TYPE_STR: return "str";
        case TYPE_VOID: return "void";
        default: return "unknown";
    }
}

/**
 * Creates a type from a token representing a primitive type.
 * 
 * @param token The token representing the type
 * @return Newly created type, or NULL if token is not a type
 */
Type* type_from_token(TokenType token) {
    switch (token) {
        case TOKEN_I8: return type_create(TYPE_I8);
        case TOKEN_I16: return type_create(TYPE_I16);
        case TOKEN_I32: return type_create(TYPE_I32);
        case TOKEN_I64: return type_create(TYPE_I64);
        case TOKEN_U8: return type_create(TYPE_U8);
        case TOKEN_U16: return type_create(TYPE_U16);
        case TOKEN_U32: return type_create(TYPE_U32);
        case TOKEN_U64: return type_create(TYPE_U64);
        case TOKEN_F32: return type_create(TYPE_F32);
        case TOKEN_F64: return type_create(TYPE_F64);
        case TOKEN_BOOL: return type_create(TYPE_BOOL);
        case TOKEN_CHAR: return type_create(TYPE_CHAR);
        case TOKEN_STR: return type_create(TYPE_STR);
        default: return NULL;
    }
}