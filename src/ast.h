#ifndef AST_H
#define AST_H

#include <stddef.h>
#include "lexer.h"

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_STRUCT,
    AST_IMPL,
    AST_BLOCK,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_LOOP,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_LET,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_CALL,
    AST_FIELD,
    AST_INDEX,
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_ASSIGNMENT,
    AST_STRUCT_LITERAL,
    AST_ARRAY_LITERAL,
    AST_INCLUDE,
    AST_EXTERN_FUNCTION,
    AST_CAST,
} AstNodeType;

typedef struct Type Type;
typedef struct AstNode AstNode;

typedef struct {
    char* name;
    Type* type;
} Field;

typedef struct {
    char* name;
    Type* type;
} Param;

typedef struct {
    size_t line;
    size_t column;
} Location;

struct AstNode {
    AstNodeType type;
    Location location;
    Type* data_type;
    
    union {
        struct {
            AstNode** items;
            size_t count;
        } program;
        
        struct {
            char* name;
            Param* params;
            size_t param_count;
            AstNode* body;
            Type* return_type;
        } function;
        
        struct {
            char* name;
            Field* fields;
            size_t field_count;
            bool is_extern;
        } struct_def;
        
        struct {
            char* struct_name;
            AstNode** functions;
            size_t function_count;
        } impl_block;
        
        struct {
            AstNode** statements;
            size_t statement_count;
            AstNode* final_expr;
        } block;
        
        struct {
            AstNode* condition;
            AstNode* then_branch;
            AstNode* else_branch;
        } if_stmt;
        
        struct {
            AstNode* condition;
            AstNode* body;
        } while_loop;
        
        struct {
            char* iterator;
            AstNode* start;
            AstNode* end;
            AstNode* body;
        } for_loop;
        
        struct {
            AstNode* body;
        } loop_stmt;
        
        struct {
            AstNode* value;
        } return_stmt;
        
        struct {
            char* name;
            Type* type;
            AstNode* value;
            bool is_mutable;
        } let_stmt;
        
        struct {
            AstNode* left;
            AstNode* right;
            TokenType op;
        } binary;
        
        struct {
            AstNode* operand;
            TokenType op;
            bool is_mut_ref;  // For & and &mut distinction
        } unary;
        
        struct {
            AstNode* target;
            AstNode* value;
            TokenType op;
        } assignment;
        
        struct {
            AstNode* function;
            AstNode** arguments;
            size_t argument_count;
        } call;
        
        struct {
            AstNode* object;
            char* field_name;
        } field;
        
        struct {
            AstNode* array;
            AstNode* index;
        } index;
        
        struct {
            long long int_value;
            double float_value;
            char* string_value;
            char char_value;
            bool bool_value;
        } literal;
        
        struct {
            char* name;
        } identifier;
        
        struct {
            char* struct_name;
            char** field_names;
            AstNode** field_values;
            size_t field_count;
        } struct_literal;
        
        struct {
            AstNode** elements;
            size_t element_count;
        } array_literal;
        
        struct {
            char* path;
            bool is_system;
        } include;
        
        struct {
            char* name;
            Param* params;
            size_t param_count;
            Type* return_type;
            bool is_extern;
        } extern_function;
        
        struct {
            AstNode* expression;
            Type* target_type;
        } cast;
    } data;
};

AstNode* ast_create_node(AstNodeType type);
void ast_destroy(AstNode* node);
void ast_print(AstNode* node, int indent);

#endif