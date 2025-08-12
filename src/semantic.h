#ifndef SEMANTIC_H
#define SEMANTIC_H

#include <stdbool.h>
#include "ast.h"
#include "symbol_table.h"
#include "error.h"
#include "type.h"

// Semantic analyzer with comprehensive type checking
typedef struct {
    SymbolTable* symbols;
    ErrorList* errors;
    bool success;
    
    // Current analysis context
    int in_loop_count;            // Track nested loop depth
    Type* current_function_type;   // Current function return type
    const char* current_struct;    // Current struct for impl blocks
    
    // Analysis statistics
    size_t functions_analyzed;
    size_t structs_analyzed;
    size_t variables_analyzed;
    
    // Source code for error reporting
    const char* source_code;
    const char* filename;
} SemanticAnalyzer;

// Analyzer lifecycle
SemanticAnalyzer* semantic_create(void);
void semantic_destroy(SemanticAnalyzer* analyzer);
bool semantic_analyze(SemanticAnalyzer* analyzer, AstNode* ast);
void semantic_set_source(SemanticAnalyzer* analyzer, const char* source, const char* filename);

// Type checking and inference
Type* semantic_check_expression(SemanticAnalyzer* analyzer, AstNode* expr);
Type* semantic_infer_type(SemanticAnalyzer* analyzer, AstNode* expr);
bool semantic_check_types_compatible(Type* expected, Type* actual);

// Statement analysis
void semantic_check_statement(SemanticAnalyzer* analyzer, AstNode* stmt);
void semantic_check_block(SemanticAnalyzer* analyzer, AstNode* block);

// Declaration analysis  
void semantic_check_function(SemanticAnalyzer* analyzer, AstNode* func);
void semantic_check_struct(SemanticAnalyzer* analyzer, AstNode* struct_def);
void semantic_check_impl(SemanticAnalyzer* analyzer, AstNode* impl);

// Type comparison utilities
bool types_equal(Type* a, Type* b);
bool type_is_numeric(Type* type);
bool type_is_integral(Type* type);
bool type_is_signed(Type* type);
bool type_is_reference(Type* type);
bool type_is_pointer(Type* type);
Type* type_dereference(Type* type);

// Error reporting
void semantic_error(SemanticAnalyzer* analyzer, const char* format, ...);
void semantic_error_at(SemanticAnalyzer* analyzer, size_t line, size_t column, const char* format, ...);
void semantic_error_node(SemanticAnalyzer* analyzer, AstNode* node, const char* format, ...);

#endif