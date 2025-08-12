#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdbool.h>
#include <stddef.h>
#include "type.h"

// Symbol kinds for different types of identifiers
typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_STRUCT,
    SYMBOL_PARAMETER,
    SYMBOL_FIELD
} SymbolKind;

// Scope types
typedef enum {
    SCOPE_GLOBAL,
    SCOPE_FUNCTION,
    SCOPE_BLOCK,
    SCOPE_LOOP,     // For break/continue validation
    SCOPE_STRUCT    // For impl blocks
} ScopeType;

// Symbol information
typedef struct Symbol {
    char* name;
    SymbolKind kind;
    Type* type;
    bool is_mutable;
    bool is_initialized;
    
    // Location in source
    size_t line;
    size_t column;
    
    // Scope information
    struct Scope* scope;
    int offset;  // For stack allocation
    
    // Additional metadata
    union {
        struct {
            size_t param_count;
            Type** param_types;
            char** param_names;
            bool* param_mutability;
        } function;
        struct {
            size_t field_count;
            struct Symbol** fields;
        } struct_def;
        struct {
            size_t index;
        } param;
    } info;
    
    struct Symbol* next;  // For hash collision chaining
} Symbol;

// Scope for managing nested scopes
typedef struct Scope {
    struct Scope* parent;
    ScopeType type;
    
    // Hash table for symbols
    Symbol** symbols;
    size_t table_size;
    
    // Scope metadata
    Type* return_type;      // For function scopes
    char* struct_name;      // For struct impl blocks
    size_t level;          // Nesting level
} Scope;

// Symbol table with scope management
typedef struct {
    Scope* current;
    Scope* global;
    
    // Type registry for user-defined types
    Symbol** types;
    size_t type_count;
    size_t type_capacity;
    
    // Error tracking
    bool has_errors;
} SymbolTable;

// Symbol table creation and destruction
SymbolTable* symbol_table_create(void);
void symbol_table_destroy(SymbolTable* table);

// Scope management
void symbol_table_enter_scope(SymbolTable* table, ScopeType type);
void symbol_table_enter_function_scope(SymbolTable* table, Type* return_type);
void symbol_table_enter_struct_scope(SymbolTable* table, const char* struct_name);
void symbol_table_exit_scope(SymbolTable* table);

// Symbol definition and lookup
Symbol* symbol_table_define(SymbolTable* table, const char* name, SymbolKind kind, 
                            Type* type, bool is_mutable);
Symbol* symbol_table_lookup(SymbolTable* table, const char* name);
Symbol* symbol_table_lookup_current_scope(SymbolTable* table, const char* name);
Symbol* symbol_table_lookup_function(SymbolTable* table, const char* name);
Symbol* symbol_table_lookup_struct(SymbolTable* table, const char* name);

// Type management
bool symbol_table_register_type(SymbolTable* table, const char* name, Symbol* type_symbol);
Symbol* symbol_table_lookup_type(SymbolTable* table, const char* name);

// Scope queries
bool symbol_table_in_loop(SymbolTable* table);
bool symbol_table_in_function(SymbolTable* table);
Type* symbol_table_get_return_type(SymbolTable* table);
const char* symbol_table_get_current_struct(SymbolTable* table);

// Symbol creation helpers
Symbol* symbol_create_variable(const char* name, Type* type, bool is_mutable);
Symbol* symbol_create_function(const char* name, Type* return_type, 
                               size_t param_count, Type** param_types, 
                               char** param_names, bool* param_mutability);
Symbol* symbol_create_struct(const char* name, size_t field_count, Symbol** fields);

#endif