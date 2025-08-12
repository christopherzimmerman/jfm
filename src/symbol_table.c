#include "symbol_table.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_TABLE_SIZE 127  // Prime number for better hash distribution
#define INITIAL_TYPE_CAPACITY 32

/**
 * Computes hash value for string keys using djb2 algorithm.
 * 
 * @param str The string to hash
 * @param table_size Size of the hash table
 * @return Hash value modulo table_size
 */
static size_t hash_string(const char* str, size_t table_size) {
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return hash % table_size;
}

/**
 * Creates a new scope with the specified type and parent.
 * 
 * @param type The type of scope to create
 * @param parent Parent scope (NULL for global scope)
 * @return Newly allocated scope
 */
static Scope* scope_create(ScopeType type, Scope* parent) {
    Scope* scope = calloc(1, sizeof(Scope));
    scope->type = type;
    scope->parent = parent;
    scope->table_size = INITIAL_TABLE_SIZE;
    scope->symbols = calloc(scope->table_size, sizeof(Symbol*));
    scope->level = parent ? parent->level + 1 : 0;
    return scope;
}

/**
 * Destroys a scope and all its symbols, freeing associated memory.
 * 
 * @param scope The scope to destroy
 */
static void scope_destroy(Scope* scope) {
    if (!scope) return;
    
    for (size_t i = 0; i < scope->table_size; i++) {
        Symbol* sym = scope->symbols[i];
        while (sym) {
            Symbol* next = sym->next;
            free(sym->name);
            if (sym->kind == SYMBOL_FUNCTION) {
                for (size_t j = 0; j < sym->info.function.param_count; j++) {
                    free(sym->info.function.param_names[j]);
                }
                free(sym->info.function.param_names);
                free(sym->info.function.param_mutability);
            } else if (sym->kind == SYMBOL_STRUCT) {
            }
            free(sym);
            sym = next;
        }
    }
    
    free(scope->symbols);
    free(scope->struct_name);
    free(scope);
}

/**
 * Defines a symbol in the given scope.
 * Checks for redefinition conflicts within the same scope.
 * 
 * @param scope The scope to define the symbol in
 * @param symbol The symbol to define
 * @return The defined symbol on success, NULL if already defined
 */
static Symbol* scope_define(Scope* scope, Symbol* symbol) {
    size_t index = hash_string(symbol->name, scope->table_size);
    
    Symbol* existing = scope->symbols[index];
    while (existing) {
        if (strcmp(existing->name, symbol->name) == 0) {
            return NULL;  // Already defined
        }
        existing = existing->next;
    }
    
    symbol->next = scope->symbols[index];
    scope->symbols[index] = symbol;
    symbol->scope = scope;
    
    return symbol;
}

/**
 * Looks up a symbol by name within a single scope.
 * Does not search parent scopes.
 * 
 * @param scope The scope to search
 * @param name The symbol name to find
 * @return The found symbol, or NULL if not found
 */
static Symbol* scope_lookup(Scope* scope, const char* name) {
    size_t index = hash_string(name, scope->table_size);
    Symbol* sym = scope->symbols[index];
    
    while (sym) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
        sym = sym->next;
    }
    
    return NULL;
}

/**
 * Creates a new symbol table with global scope.
 * Initializes type storage and error tracking.
 * 
 * @return Newly allocated symbol table
 */
SymbolTable* symbol_table_create(void) {
    SymbolTable* table = calloc(1, sizeof(SymbolTable));
    table->global = scope_create(SCOPE_GLOBAL, NULL);
    table->current = table->global;
    table->type_capacity = INITIAL_TYPE_CAPACITY;
    table->types = calloc(table->type_capacity, sizeof(Symbol*));
    table->has_errors = false;
    return table;
}

/**
 * Destroys a symbol table and all its scopes.
 * Frees all symbols and associated memory.
 * 
 * @param table The symbol table to destroy
 */
void symbol_table_destroy(SymbolTable* table) {
    if (!table) return;
    
    while (table->current != table->global) {
        Scope* parent = table->current->parent;
        scope_destroy(table->current);
        table->current = parent;
    }
    
    scope_destroy(table->global);
    
    free(table->types);
    free(table);
}

/**
 * Enters a new scope of the specified type.
 * 
 * @param table The symbol table
 * @param type The type of scope to enter
 */
void symbol_table_enter_scope(SymbolTable* table, ScopeType type) {
    table->current = scope_create(type, table->current);
}

/**
 * Enters a new function scope with specified return type.
 * 
 * @param table The symbol table
 * @param return_type The function's return type
 */
void symbol_table_enter_function_scope(SymbolTable* table, Type* return_type) {
    table->current = scope_create(SCOPE_FUNCTION, table->current);
    table->current->return_type = return_type;
}

/**
 * Enters a new struct scope for impl blocks.
 * Associates the scope with a specific struct name.
 * 
 * @param table The symbol table
 * @param struct_name The name of the struct being implemented
 */
void symbol_table_enter_struct_scope(SymbolTable* table, const char* struct_name) {
    table->current = scope_create(SCOPE_STRUCT, table->current);
    table->current->struct_name = string_duplicate(struct_name);
}

/**
 * Exits the current scope and returns to the parent scope.
 * Does not exit the global scope.
 * 
 * @param table The symbol table
 */
void symbol_table_exit_scope(SymbolTable* table) {
    if (table->current && table->current != table->global) {
        Scope* parent = table->current->parent;
        scope_destroy(table->current);
        table->current = parent;
    }
}

/**
 * Defines a new symbol in the current scope.
 * Checks for redefinition conflicts.
 * 
 * @param table The symbol table
 * @param name The symbol name
 * @param kind The kind of symbol
 * @param type The symbol's type
 * @param is_mutable Whether the symbol is mutable
 * @return The newly defined symbol, or NULL on conflict
 */
Symbol* symbol_table_define(SymbolTable* table, const char* name, SymbolKind kind,
                            Type* type, bool is_mutable) {
    if (scope_lookup(table->current, name)) {
        table->has_errors = true;
        return NULL;
    }
    
    Symbol* symbol = calloc(1, sizeof(Symbol));
    symbol->name = string_duplicate(name);
    symbol->kind = kind;
    symbol->type = type;
    symbol->is_mutable = is_mutable;
    symbol->is_initialized = false;
    
    Symbol* result = scope_define(table->current, symbol);
    if (!result) {
        free(symbol->name);
        free(symbol);
        table->has_errors = true;
        return NULL;
    }
    
    return result;
}

/**
 * Looks up a symbol by name, searching through the scope chain.
 * Searches from current scope up to global scope.
 * 
 * @param table The symbol table
 * @param name The symbol name to find
 * @return The found symbol, or NULL if not found
 */
Symbol* symbol_table_lookup(SymbolTable* table, const char* name) {
    Scope* scope = table->current;
    
    while (scope) {
        Symbol* sym = scope_lookup(scope, name);
        if (sym) {
            return sym;
        }
        scope = scope->parent;
    }
    
    return NULL;
}

/**
 * Looks up a symbol only in the current scope.
 * Does not search parent scopes.
 * 
 * @param table The symbol table
 * @param name The symbol name to find
 * @return The found symbol, or NULL if not found
 */
Symbol* symbol_table_lookup_current_scope(SymbolTable* table, const char* name) {
    return scope_lookup(table->current, name);
}

/**
 * Looks up a function symbol by name.
 * Searches through scope chain and filters for function symbols.
 * 
 * @param table The symbol table
 * @param name The function name to find
 * @return The function symbol, or NULL if not found
 */
Symbol* symbol_table_lookup_function(SymbolTable* table, const char* name) {
    Symbol* sym = symbol_table_lookup(table, name);
    if (sym && sym->kind == SYMBOL_FUNCTION) {
        return sym;
    }
    return NULL;
}

/**
 * Looks up a struct type symbol by name.
 * 
 * @param table The symbol table
 * @param name The struct name to find
 * @return The struct symbol, or NULL if not found
 */
Symbol* symbol_table_lookup_struct(SymbolTable* table, const char* name) {
    Symbol* sym = symbol_table_lookup_type(table, name);
    if (sym && sym->kind == SYMBOL_STRUCT) {
        return sym;
    }
    return NULL;
}

/**
 * Registers a user-defined type symbol.
 * Expands type storage if needed and checks for duplicates.
 * 
 * @param table The symbol table
 * @param name The type name
 * @param type_symbol The type symbol to register
 * @return true on success, false if already exists
 */
bool symbol_table_register_type(SymbolTable* table, const char* name, Symbol* type_symbol) {
    for (size_t i = 0; i < table->type_count; i++) {
        if (strcmp(table->types[i]->name, name) == 0) {
            table->has_errors = true;
            return false;  // Already registered
        }
    }
    
    if (table->type_count >= table->type_capacity) {
        table->type_capacity *= 2;
        table->types = realloc(table->types, table->type_capacity * sizeof(Symbol*));
    }
    
    table->types[table->type_count++] = type_symbol;
    return true;
}

/**
 * Looks up a type symbol by name.
 * Searches the registered types list.
 * 
 * @param table The symbol table
 * @param name The type name to find
 * @return The type symbol, or NULL if not found
 */
Symbol* symbol_table_lookup_type(SymbolTable* table, const char* name) {
    for (size_t i = 0; i < table->type_count; i++) {
        if (strcmp(table->types[i]->name, name) == 0) {
            return table->types[i];
        }
    }
    return NULL;
}

/**
 * Checks if currently inside a loop scope.
 * Walks up the scope chain looking for loop scopes.
 * 
 * @param table The symbol table
 * @return true if in a loop, false otherwise
 */
bool symbol_table_in_loop(SymbolTable* table) {
    Scope* scope = table->current;
    
    while (scope) {
        if (scope->type == SCOPE_LOOP) {
            return true;
        }
        if (scope->type == SCOPE_BLOCK && scope->parent && scope->parent->type == SCOPE_LOOP) {
            return true;
        }
        scope = scope->parent;
    }
    
    return false;
}

/**
 * Checks if currently inside a function scope.
 * 
 * @param table The symbol table
 * @return true if in a function, false otherwise
 */
bool symbol_table_in_function(SymbolTable* table) {
    Scope* scope = table->current;
    
    while (scope) {
        if (scope->type == SCOPE_FUNCTION) {
            return true;
        }
        scope = scope->parent;
    }
    
    return false;
}

/**
 * Gets the return type of the current function.
 * Searches up the scope chain for the enclosing function.
 * 
 * @param table The symbol table
 * @return The function's return type, or NULL if not in a function
 */
Type* symbol_table_get_return_type(SymbolTable* table) {
    Scope* scope = table->current;
    
    while (scope) {
        if (scope->type == SCOPE_FUNCTION) {
            return scope->return_type;
        }
        scope = scope->parent;
    }
    
    return NULL;
}

/**
 * Gets the name of the current struct being implemented.
 * Used within impl blocks to identify the target struct.
 * 
 * @param table The symbol table
 * @return The struct name, or NULL if not in an impl block
 */
const char* symbol_table_get_current_struct(SymbolTable* table) {
    Scope* scope = table->current;
    
    while (scope) {
        if (scope->type == SCOPE_STRUCT) {
            return scope->struct_name;
        }
        scope = scope->parent;
    }
    
    return NULL;
}

/**
 * Creates a new variable symbol with the given properties.
 * 
 * @param name The variable name
 * @param type The variable's type
 * @param is_mutable Whether the variable is mutable
 * @return Newly created variable symbol
 */
Symbol* symbol_create_variable(const char* name, Type* type, bool is_mutable) {
    Symbol* sym = calloc(1, sizeof(Symbol));
    sym->name = string_duplicate(name);
    sym->kind = SYMBOL_VARIABLE;
    sym->type = type;
    sym->is_mutable = is_mutable;
    sym->is_initialized = false;
    return sym;
}

/**
 * Creates a new function symbol with parameter information.
 * 
 * @param name The function name
 * @param return_type The function's return type
 * @param param_count Number of parameters
 * @param param_types Array of parameter types
 * @param param_names Array of parameter names
 * @param param_mutability Array of parameter mutability flags
 * @return Newly created function symbol
 */
Symbol* symbol_create_function(const char* name, Type* return_type,
                               size_t param_count, Type** param_types,
                               char** param_names, bool* param_mutability) {
    Symbol* sym = calloc(1, sizeof(Symbol));
    sym->name = string_duplicate(name);
    sym->kind = SYMBOL_FUNCTION;
    sym->type = return_type;
    sym->is_initialized = true;
    
    sym->info.function.param_count = param_count;
    sym->info.function.param_types = param_types;
    sym->info.function.param_names = param_names;
    sym->info.function.param_mutability = param_mutability;
    
    return sym;
}

/**
 * Creates a new struct symbol with field information.
 * 
 * @param name The struct name
 * @param field_count Number of fields
 * @param fields Array of field symbols
 * @return Newly created struct symbol
 */
Symbol* symbol_create_struct(const char* name, size_t field_count, Symbol** fields) {
    Symbol* sym = calloc(1, sizeof(Symbol));
    sym->name = string_duplicate(name);
    sym->kind = SYMBOL_STRUCT;
    sym->type = type_create(TYPE_STRUCT);
    sym->type->data.struct_type.name = string_duplicate(name);
    sym->is_initialized = true;
    
    sym->info.struct_def.field_count = field_count;
    sym->info.struct_def.fields = fields;
    
    return sym;
}