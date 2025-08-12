#include "parser.h"
#include "type.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * Checks if the parser has reached the end of the token stream.
 * 
 * @param parser The parser instance
 * @return true if at EOF token, false otherwise
 */
static bool is_at_end(Parser* parser) {
    return parser->tokens[parser->current].type == TOKEN_EOF;
}

/**
 * Returns the current token without consuming it.
 * 
 * @param parser The parser instance
 * @return Pointer to the current token
 */
static Token* peek(Parser* parser) {
    return &parser->tokens[parser->current];
}

/**
 * Returns the previously consumed token.
 * 
 * @param parser The parser instance
 * @return Pointer to the previous token
 */
static Token* previous(Parser* parser) {
    return &parser->tokens[parser->current - 1];
}

/**
 * Advances to the next token and returns the consumed one.
 * 
 * @param parser The parser instance
 * @return Pointer to the token that was just consumed
 */
static Token* advance(Parser* parser) {
    if (!is_at_end(parser)) {
        parser->current++;
    }
    return previous(parser);
}

/**
 * Checks if the current token is of the given type.
 * 
 * @param parser The parser instance
 * @param type The token type to check for
 * @return true if current token matches type, false otherwise
 */
static bool check(Parser* parser, TokenType type) {
    if (is_at_end(parser)) return false;
    return peek(parser)->type == type;
}

/**
 * Checks if current token matches type and consumes it if so.
 * 
 * @param parser The parser instance
 * @param type The token type to match
 * @return true if matched and consumed, false otherwise
 */
static bool match(Parser* parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return true;
    }
    return false;
}

/**
 * Reports a parse error at the given token.
 * Enters panic mode to suppress cascading errors.
 * 
 * @param parser The parser instance
 * @param token The token where the error occurred
 * @param message The error message
 */
static void error_at(Parser* parser, Token* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;
    
    error_list_add(parser->errors, message, "input", token->line, token->column);
}

/**
 * Reports a parse error at the current token.
 * 
 * @param parser The parser instance
 * @param message The error message
 */
static void error_at_current(Parser* parser, const char* message) {
    error_at(parser, peek(parser), message);
}

/**
 * Consumes a token of the expected type or reports an error.
 * 
 * @param parser The parser instance
 * @param type The expected token type
 * @param message Error message if expectation not met
 * @return The consumed token, or NULL if error
 */
static Token* consume(Parser* parser, TokenType type, const char* message) {
    if (check(parser, type)) {
        return advance(parser);
    }
    
    error_at_current(parser, message);
    return NULL;
}

/**
 * Creates an AST node with source location information.
 * 
 * @param parser The parser instance
 * @param type The type of AST node to create
 * @param token Token to use for location (optional)
 * @return The created AST node with location set
 */
static AstNode* create_node_with_location(Parser* parser, AstNodeType type, Token* token) {
    AstNode* node = ast_create_node(type);
    if (token) {
        node->location.line = token->line;
        node->location.column = token->column;
    } else if (parser->current > 0) {
        Token* prev = &parser->tokens[parser->current - 1];
        node->location.line = prev->line;
        node->location.column = prev->column;
    }
    return node;
}

/**
 * Synchronizes the parser after an error by finding the next statement boundary.
 * This prevents cascading errors and allows parsing to continue.
 * 
 * @param parser The parser instance
 */
static void synchronize(Parser* parser) {
    parser->panic_mode = false;
    
    while (!is_at_end(parser)) {
        if (previous(parser)->type == TOKEN_SEMICOLON) return;
        
        switch (peek(parser)->type) {
            case TOKEN_FN:
            case TOKEN_LET:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_FOR:
            case TOKEN_LOOP:
            case TOKEN_RETURN:
            case TOKEN_BREAK:
            case TOKEN_CONTINUE:
            case TOKEN_STRUCT:
            case TOKEN_IMPL:
                return;
            default:
                break;
        }
        
        advance(parser);
    }
}

static AstNode* expression(Parser* parser);
static AstNode* statement(Parser* parser);
static AstNode* declaration(Parser* parser);
static Type* parse_type(Parser* parser);

static AstNode* primary(Parser* parser) {
    // Check for error tokens and skip them
    if (peek(parser)->type == TOKEN_ERROR) {
        error_at_current(parser, peek(parser)->start);
        advance(parser);  // Skip the error token
        return NULL;
    }
    
    // Debug: print current token
    #if 0
    Token* current = peek(parser);
    printf("DEBUG primary: token type=%d lexeme='%.*s' at %lu:%lu\n", 
           current->type, (int)current->length, current->start, 
           (unsigned long)current->line, (unsigned long)current->column);
    #endif
    
    if (match(parser, TOKEN_TRUE)) {
        Token* token = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_LITERAL, token);
        node->data.literal.bool_value = true;
        node->data_type = type_create(TYPE_BOOL);
        return node;
    }
    
    if (match(parser, TOKEN_FALSE)) {
        Token* token = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_LITERAL, token);
        node->data.literal.bool_value = false;
        node->data_type = type_create(TYPE_BOOL);
        return node;
    }
    
    if (match(parser, TOKEN_INT_LITERAL)) {
        Token* lit_token = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_LITERAL, lit_token);
        node->data.literal.int_value = lit_token->value.int_value;
        node->data_type = type_create(TYPE_I32);
        return node;
    }
    
    if (match(parser, TOKEN_FLOAT_LITERAL)) {
        Token* token = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_LITERAL, token);
        node->data.literal.float_value = token->value.float_value;
        node->data_type = type_create(TYPE_F64);
        return node;
    }
    
    if (match(parser, TOKEN_STRING_LITERAL)) {
        Token* str_token = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_LITERAL, str_token);
        node->data.literal.string_value = string_n_duplicate(str_token->start + 1, str_token->length - 2);
        node->data_type = type_create(TYPE_STR);
        return node;
    }
    
    if (match(parser, TOKEN_CHAR_LITERAL)) {
        Token* token = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_LITERAL, token);
        node->data.literal.char_value = previous(parser)->value.char_value;
        node->data_type = type_create(TYPE_CHAR);
        return node;
    }
    
    if (match(parser, TOKEN_IDENTIFIER)) {
        Token* name_token = previous(parser);
        
        // Check if this might be a struct literal
        // We only parse as struct literal if followed by { then field: or }
        if (check(parser, TOKEN_LBRACE)) {
            // Save position before consuming {
            size_t saved_pos = parser->current;
            advance(parser);  // consume {
            
            bool is_struct_literal = false;
            if (check(parser, TOKEN_RBRACE)) {
                is_struct_literal = true;
            } else if (check(parser, TOKEN_IDENTIFIER)) {
                size_t saved_pos2 = parser->current;
                advance(parser);
                is_struct_literal = check(parser, TOKEN_COLON);
                parser->current = saved_pos2;
            }
            
            if (!is_struct_literal) {
                parser->current = saved_pos;
            } else {
                AstNode* node = create_node_with_location(parser, AST_STRUCT_LITERAL, name_token);
                node->data.struct_literal.struct_name = string_n_duplicate(name_token->start, name_token->length);
                
                size_t capacity = 8;
                node->data.struct_literal.field_names = malloc(sizeof(char*) * capacity);
                node->data.struct_literal.field_values = malloc(sizeof(AstNode*) * capacity);
                node->data.struct_literal.field_count = 0;
                
                while (!check(parser, TOKEN_RBRACE) && !is_at_end(parser)) {
                    Token* field_name = consume(parser, TOKEN_IDENTIFIER, "Expected field name");
                    if (!field_name) break;
                    
                    consume(parser, TOKEN_COLON, "Expected ':' after field name");
                    AstNode* value = expression(parser);
                    
                    if (node->data.struct_literal.field_count >= capacity) {
                        capacity *= 2;
                        node->data.struct_literal.field_names = realloc(node->data.struct_literal.field_names, sizeof(char*) * capacity);
                        node->data.struct_literal.field_values = realloc(node->data.struct_literal.field_values, sizeof(AstNode*) * capacity);
                    }
                    
                    node->data.struct_literal.field_names[node->data.struct_literal.field_count] = string_n_duplicate(field_name->start, field_name->length);
                    node->data.struct_literal.field_values[node->data.struct_literal.field_count] = value;
                    node->data.struct_literal.field_count++;
                    
                    if (!match(parser, TOKEN_COMMA)) break;
                }
                
                consume(parser, TOKEN_RBRACE, "Expected '}' after struct fields");
                return node;
            }
        }
        
        AstNode* node = create_node_with_location(parser, AST_IDENTIFIER, name_token);
        node->data.identifier.name = string_n_duplicate(name_token->start, name_token->length);
        return node;
    }
    
    if (match(parser, TOKEN_LBRACKET)) {
        Token* lbracket = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_ARRAY_LITERAL, lbracket);
        
        size_t capacity = 8;
        node->data.array_literal.elements = malloc(sizeof(AstNode*) * capacity);
        node->data.array_literal.element_count = 0;
        
        while (!check(parser, TOKEN_RBRACKET) && !is_at_end(parser)) {
            if (node->data.array_literal.element_count >= capacity) {
                capacity *= 2;
                node->data.array_literal.elements = realloc(node->data.array_literal.elements, sizeof(AstNode*) * capacity);
            }
            
            node->data.array_literal.elements[node->data.array_literal.element_count++] = expression(parser);
            
            if (!match(parser, TOKEN_COMMA)) break;
        }
        
        consume(parser, TOKEN_RBRACKET, "Expected ']' after array elements");
        return node;
    }
    
    if (match(parser, TOKEN_LPAREN)) {
        AstNode* expr = expression(parser);
        consume(parser, TOKEN_RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    error_at_current(parser, "Expected expression");
    return NULL;
}

/**
 * Parses postfix expressions (function calls, field access, array indexing).
 * Includes loop guard to prevent infinite loops in deeply nested expressions.
 * 
 * @param parser The parser instance
 * @return AST node for the call/postfix expression
 */
static AstNode* call(Parser* parser) {
    AstNode* expr = primary(parser);
    
    int iteration_count = 0;
    const int MAX_CHAIN_DEPTH = 100;
    
    while (true) {
        if (++iteration_count > MAX_CHAIN_DEPTH) {
            error_at_current(parser, "Expression chain too deep (possible infinite loop)");
            break;
        }
        
        if (match(parser, TOKEN_LPAREN)) {
            Token* lparen = previous(parser);
            AstNode* node = create_node_with_location(parser, AST_CALL, lparen);
            node->data.call.function = expr;
            
            size_t capacity = 8;
            node->data.call.arguments = malloc(sizeof(AstNode*) * capacity);
            node->data.call.argument_count = 0;
            
            if (!check(parser, TOKEN_RPAREN)) {
                do {
                    if (node->data.call.argument_count >= capacity) {
                        capacity *= 2;
                        node->data.call.arguments = realloc(node->data.call.arguments, sizeof(AstNode*) * capacity);
                    }
                    node->data.call.arguments[node->data.call.argument_count++] = expression(parser);
                } while (match(parser, TOKEN_COMMA));
            }
            
            consume(parser, TOKEN_RPAREN, "Expected ')' after arguments");
            expr = node;
        } else if (match(parser, TOKEN_LBRACKET)) {
            Token* lbracket = previous(parser);
            AstNode* node = create_node_with_location(parser, AST_INDEX, lbracket);
            node->data.index.array = expr;
            node->data.index.index = expression(parser);
            consume(parser, TOKEN_RBRACKET, "Expected ']' after index");
            expr = node;
        } else if (match(parser, TOKEN_DOT)) {
            Token* dot = previous(parser);
            AstNode* node = create_node_with_location(parser, AST_FIELD, dot);
            node->data.field.object = expr;
            Token* field = consume(parser, TOKEN_IDENTIFIER, "Expected field name after '.'");
            if (field) {
                node->data.field.field_name = string_n_duplicate(field->start, field->length);
            }
            expr = node;
        } else if (match(parser, TOKEN_DOUBLE_COLON)) {
            Token* method = consume(parser, TOKEN_IDENTIFIER, "Expected method name after '::'");
            if (method) {
                AstNode* node = create_node_with_location(parser, AST_IDENTIFIER, method);
                size_t len = strlen(expr->data.identifier.name) + 2 + method->length + 1;
                char* full_name = malloc(len);
                snprintf(full_name, len, "%s::%.*s", expr->data.identifier.name, (int)method->length, method->start);
                node->data.identifier.name = full_name;
                free(expr->data.identifier.name);
                free(expr);
                expr = node;
            }
        } else {
            break;
        }
    }
    
    return expr;
}

/**
 * Parses unary expressions (!, -, *, &, &mut).
 * Handles reference and dereference operators.
 * 
 * @param parser The parser instance
 * @return AST node for the unary expression
 */
static AstNode* unary(Parser* parser) {
    if (match(parser, TOKEN_NOT) || match(parser, TOKEN_MINUS) || match(parser, TOKEN_STAR)) {
        Token* op = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_UNARY_OP, op);
        node->data.unary.op = op->type;
        node->data.unary.is_mut_ref = false;
        node->data.unary.operand = unary(parser);
        return node;
    }
    
    if (match(parser, TOKEN_AND)) {
        Token* op = previous(parser);
        bool is_mut = match(parser, TOKEN_MUT);
        
        AstNode* node = ast_create_node(AST_UNARY_OP);
        node->data.unary.op = op->type;
        node->data.unary.is_mut_ref = is_mut;
        node->data.unary.operand = unary(parser);
        return node;
    }
    
    return call(parser);
}

/**
 * Parses multiplicative expressions (*, /, %).
 * 
 * @param parser The parser instance
 * @return AST node for the factor expression
 */
static AstNode* factor(Parser* parser) {
    AstNode* expr = unary(parser);
    
    while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH) || match(parser, TOKEN_PERCENT)) {
        Token* op = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_BINARY_OP, op);
        node->data.binary.left = expr;
        node->data.binary.op = op->type;
        node->data.binary.right = unary(parser);
        expr = node;
    }
    
    return expr;
}

/**
 * Parses additive expressions (+, -).
 * 
 * @param parser The parser instance
 * @return AST node for the term expression
 */
static AstNode* term(Parser* parser) {
    AstNode* expr = factor(parser);
    
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        Token* op = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_BINARY_OP, op);
        node->data.binary.left = expr;
        node->data.binary.op = op->type;
        node->data.binary.right = factor(parser);
        expr = node;
    }
    
    return expr;
}

/**
 * Parses bitwise shift expressions (<<, >>).
 * 
 * @param parser The parser instance
 * @return AST node for the shift expression
 */
static AstNode* shift(Parser* parser) {
    AstNode* expr = term(parser);
    
    while (match(parser, TOKEN_LT_LT) || match(parser, TOKEN_GT_GT)) {
        Token* op = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_BINARY_OP, op);
        node->data.binary.left = expr;
        node->data.binary.op = op->type;
        node->data.binary.right = term(parser);
        expr = node;
    }
    
    return expr;
}

/**
 * Parses cast expressions (expr as Type).
 * 
 * @param parser The parser instance
 * @return AST node for the cast expression
 */
static AstNode* cast(Parser* parser) {
    AstNode* expr = shift(parser);
    
    while (match(parser, TOKEN_AS)) {
        Token* as_token = previous(parser);
        Type* target_type = parse_type(parser);
        if (!target_type) {
            error_at_current(parser, "Expected type after 'as'");
            return expr;
        }
        
        AstNode* node = create_node_with_location(parser, AST_CAST, as_token);
        node->data.cast.expression = expr;
        node->data.cast.target_type = target_type;
        expr = node;
    }
    
    return expr;
}

/**
 * Parses comparison expressions (<, >, <=, >=).
 * 
 * @param parser The parser instance
 * @return AST node for the comparison expression
 */
static AstNode* comparison(Parser* parser) {
    AstNode* expr = cast(parser);
    
    while (match(parser, TOKEN_LT) || match(parser, TOKEN_GT) || 
           match(parser, TOKEN_LT_EQ) || match(parser, TOKEN_GT_EQ)) {
        Token* op = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_BINARY_OP, op);
        node->data.binary.left = expr;
        node->data.binary.op = op->type;
        node->data.binary.right = cast(parser);
        expr = node;
    }
    
    return expr;
}

/**
 * Parses equality expressions (==, !=).
 * 
 * @param parser The parser instance
 * @return AST node for the equality expression
 */
static AstNode* equality(Parser* parser) {
    AstNode* expr = comparison(parser);
    
    while (match(parser, TOKEN_EQ_EQ) || match(parser, TOKEN_NOT_EQ)) {
        Token* op = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_BINARY_OP, op);
        node->data.binary.left = expr;
        node->data.binary.op = op->type;
        node->data.binary.right = comparison(parser);
        expr = node;
    }
    
    return expr;
}

/**
 * Parses bitwise AND expressions (&).
 * 
 * @param parser The parser instance
 * @return AST node for the bitwise AND expression
 */
static AstNode* bitwise_and(Parser* parser) {
    AstNode* expr = equality(parser);
    
    while (match(parser, TOKEN_AND)) {
        Token* op = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_BINARY_OP, op);
        node->data.binary.left = expr;
        node->data.binary.op = op->type;
        node->data.binary.right = equality(parser);
        expr = node;
    }
    
    return expr;
}

/**
 * Parses bitwise XOR expressions (^).
 * 
 * @param parser The parser instance
 * @return AST node for the bitwise XOR expression
 */
static AstNode* bitwise_xor(Parser* parser) {
    AstNode* expr = bitwise_and(parser);
    
    while (match(parser, TOKEN_XOR)) {
        Token* op = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_BINARY_OP, op);
        node->data.binary.left = expr;
        node->data.binary.op = op->type;
        node->data.binary.right = bitwise_and(parser);
        expr = node;
    }
    
    return expr;
}

/**
 * Parses bitwise OR expressions (|).
 * 
 * @param parser The parser instance
 * @return AST node for the bitwise OR expression
 */
static AstNode* bitwise_or(Parser* parser) {
    AstNode* expr = bitwise_xor(parser);
    
    while (match(parser, TOKEN_OR)) {
        Token* op = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_BINARY_OP, op);
        node->data.binary.left = expr;
        node->data.binary.op = op->type;
        node->data.binary.right = bitwise_xor(parser);
        expr = node;
    }
    
    return expr;
}

/**
 * Parses logical AND expressions (&&).
 * 
 * @param parser The parser instance
 * @return AST node for the logical AND expression
 */
static AstNode* logical_and(Parser* parser) {
    AstNode* expr = bitwise_or(parser);
    
    while (match(parser, TOKEN_AND_AND)) {
        Token* op = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_BINARY_OP, op);
        node->data.binary.left = expr;
        node->data.binary.op = op->type;
        node->data.binary.right = bitwise_or(parser);
        expr = node;
    }
    
    return expr;
}

/**
 * Parses logical OR expressions (||).
 * 
 * @param parser The parser instance
 * @return AST node for the logical OR expression
 */
static AstNode* logical_or(Parser* parser) {
    AstNode* expr = logical_and(parser);
    
    while (match(parser, TOKEN_OR_OR)) {
        Token* op = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_BINARY_OP, op);
        node->data.binary.left = expr;
        node->data.binary.op = op->type;
        node->data.binary.right = logical_and(parser);
        expr = node;
    }
    
    return expr;
}

/**
 * Parses assignment expressions (=, +=, -=, *=, /=).
 * Right-associative to allow chained assignments.
 * 
 * @param parser The parser instance
 * @return AST node for the assignment expression
 */
static AstNode* assignment(Parser* parser) {
    AstNode* expr = logical_or(parser);
    
    if (match(parser, TOKEN_EQ) || match(parser, TOKEN_PLUS_EQ) || 
        match(parser, TOKEN_MINUS_EQ) || match(parser, TOKEN_STAR_EQ) || 
        match(parser, TOKEN_SLASH_EQ)) {
        Token* op = previous(parser);
        AstNode* node = create_node_with_location(parser, AST_ASSIGNMENT, op);
        node->data.assignment.target = expr;
        node->data.assignment.op = op->type;
        node->data.assignment.value = assignment(parser);
        return node;
    }
    
    return expr;
}

/**
 * Entry point for expression parsing.
 * 
 * @param parser The parser instance
 * @return AST node for the expression
 */
static AstNode* expression(Parser* parser) {
    return assignment(parser);
}

/**
 * Parses type annotations including primitives, pointers, references, arrays, and structs.
 * 
 * @param parser The parser instance
 * @return Type structure representing the parsed type
 */
static Type* parse_type(Parser* parser) {
    if (match(parser, TOKEN_AND)) {
        bool is_mut = match(parser, TOKEN_MUT);
        Type* ref_type = type_create(TYPE_REFERENCE);
        ref_type->data.reference.referenced_type = parse_type(parser);
        ref_type->data.reference.is_mutable = is_mut;
        return ref_type;
    }
    
    if (match(parser, TOKEN_STAR)) {
        Type* ptr_type = type_create(TYPE_POINTER);
        ptr_type->data.pointer.pointed_type = parse_type(parser);
        return ptr_type;
    }
    
    if (match(parser, TOKEN_LBRACKET)) {
        // For arrays, we need to parse the element type
        // But we must be careful not to cause infinite recursion
        Type* elem_type = NULL;
        
        // Check for primitive types first
        Token* type_token = peek(parser);
        elem_type = type_from_token(type_token->type);
        if (elem_type) {
            advance(parser);
        } else if (check(parser, TOKEN_IDENTIFIER)) {
            // Struct type
            advance(parser);
            elem_type = type_create(TYPE_STRUCT);
            elem_type->data.struct_type.name = string_n_duplicate(previous(parser)->start, previous(parser)->length);
        } else {
            error_at_current(parser, "Expected element type in array");
            return NULL;
        }
        
        consume(parser, TOKEN_SEMICOLON, "Expected ';' in array type");
        Token* size_token = consume(parser, TOKEN_INT_LITERAL, "Expected array size");
        consume(parser, TOKEN_RBRACKET, "Expected ']' after array type");
        
        Type* array_type = type_create(TYPE_ARRAY);
        array_type->data.array.element_type = elem_type;
        array_type->data.array.size = size_token ? size_token->value.int_value : 0;
        return array_type;
    }
    
    Token* type_token = peek(parser);
    Type* type = type_from_token(type_token->type);
    if (type) {
        advance(parser);
        return type;
    }
    
    if (match(parser, TOKEN_IDENTIFIER)) {
        Type* struct_type = type_create(TYPE_STRUCT);
        struct_type->data.struct_type.name = string_n_duplicate(previous(parser)->start, previous(parser)->length);
        return struct_type;
    }
    
    error_at_current(parser, "Expected type");
    return NULL;
}

/**
 * Parses a block statement ({ ... }).
 * Handles both statements and optional final expression without semicolon.
 * Includes loop guard to prevent infinite loops.
 * 
 * @param parser The parser instance
 * @return AST node for the block statement
 */
static AstNode* block_statement(Parser* parser) {
    AstNode* node = ast_create_node(AST_BLOCK);
    
    size_t capacity = 16;
    node->data.block.statements = malloc(sizeof(AstNode*) * capacity);
    node->data.block.statement_count = 0;
    node->data.block.final_expr = NULL;
    
    int loop_guard = 0;
    const int MAX_STATEMENTS = 10000;
    
    while (!check(parser, TOKEN_RBRACE) && !is_at_end(parser)) {
        // Guard against infinite loops
        if (++loop_guard > MAX_STATEMENTS) {
            error_at_current(parser, "Block too large or parser stuck in loop");
            break;
        }
        
        
        if (node->data.block.statement_count >= capacity) {
            capacity *= 2;
            node->data.block.statements = realloc(node->data.block.statements, sizeof(AstNode*) * capacity);
        }
        
        AstNode* stmt = NULL;
        if (check(parser, TOKEN_LET) || check(parser, TOKEN_FN) || check(parser, TOKEN_STRUCT)) {
            stmt = declaration(parser);
            if (stmt) {
                node->data.block.statements[node->data.block.statement_count++] = stmt;
            }
        } else if (check(parser, TOKEN_IF) || check(parser, TOKEN_WHILE) || 
                   check(parser, TOKEN_FOR) || check(parser, TOKEN_LOOP) ||
                   check(parser, TOKEN_RETURN) || check(parser, TOKEN_BREAK) || 
                   check(parser, TOKEN_CONTINUE) || check(parser, TOKEN_LBRACE)) {
            stmt = statement(parser);
            if (stmt) {
                node->data.block.statements[node->data.block.statement_count++] = stmt;
            }
        } else {
            AstNode* expr = expression(parser);
            
            if (match(parser, TOKEN_SEMICOLON)) {
                // Has semicolon - it's a statement
                if (expr) {
                    node->data.block.statements[node->data.block.statement_count++] = expr;
                }
            } else if (check(parser, TOKEN_RBRACE)) {
                // No semicolon and at } - it's the final expression
                node->data.block.final_expr = expr;
                break;
            } else {
                // No semicolon but not at } - this is an error
                error_at_current(parser, "Expected ';' or '}' after expression");
            }
        }
    }
    
    consume(parser, TOKEN_RBRACE, "Expected '}' after block");
    return node;
}

/**
 * Parses an if statement with optional else/else-if branches.
 * 
 * @param parser The parser instance
 * @return AST node for the if statement
 */
static AstNode* if_statement(Parser* parser) {
    AstNode* node = ast_create_node(AST_IF);
    
    consume(parser, TOKEN_LPAREN, "Expected '(' after 'if'");
    node->data.if_stmt.condition = expression(parser);
    consume(parser, TOKEN_RPAREN, "Expected ')' after if condition");
    consume(parser, TOKEN_LBRACE, "Expected '{' after if condition");
    node->data.if_stmt.then_branch = block_statement(parser);
    
    if (match(parser, TOKEN_ELSE)) {
        if (match(parser, TOKEN_IF)) {
            node->data.if_stmt.else_branch = if_statement(parser);
        } else {
            consume(parser, TOKEN_LBRACE, "Expected '{' after 'else'");
            node->data.if_stmt.else_branch = block_statement(parser);
        }
    } else {
        node->data.if_stmt.else_branch = NULL;
    }
    
    return node;
}

/**
 * Parses a while loop statement.
 * 
 * @param parser The parser instance
 * @return AST node for the while statement
 */
static AstNode* while_statement(Parser* parser) {
    AstNode* node = ast_create_node(AST_WHILE);
    
    consume(parser, TOKEN_LPAREN, "Expected '(' after 'while'");
    node->data.while_loop.condition = expression(parser);
    consume(parser, TOKEN_RPAREN, "Expected ')' after while condition");
    consume(parser, TOKEN_LBRACE, "Expected '{' after while condition");
    node->data.while_loop.body = block_statement(parser);
    
    return node;
}

/**
 * Parses a for loop statement with range syntax (for i in 0..10).
 * 
 * @param parser The parser instance
 * @return AST node for the for statement
 */
static AstNode* for_statement(Parser* parser) {
    AstNode* node = ast_create_node(AST_FOR);
    
    Token* iter = consume(parser, TOKEN_IDENTIFIER, "Expected iterator name");
    if (iter) {
        node->data.for_loop.iterator = string_n_duplicate(iter->start, iter->length);
    }
    
    if (match(parser, TOKEN_COLON)) {
        parse_type(parser);
    }
    
    consume(parser, TOKEN_IN, "Expected 'in' in for loop");
    
    node->data.for_loop.start = expression(parser);
    consume(parser, TOKEN_DOT_DOT, "Expected '..' in for range");
    node->data.for_loop.end = expression(parser);
    
    consume(parser, TOKEN_LBRACE, "Expected '{' after for header");
    node->data.for_loop.body = block_statement(parser);
    
    return node;
}

/**
 * Parses an infinite loop statement (loop { ... }).
 * 
 * @param parser The parser instance
 * @return AST node for the loop statement
 */
static AstNode* loop_statement(Parser* parser) {
    AstNode* node = ast_create_node(AST_LOOP);
    
    consume(parser, TOKEN_LBRACE, "Expected '{' after 'loop'");
    node->data.loop_stmt.body = block_statement(parser);
    
    return node;
}

/**
 * Parses a return statement with optional return value.
 * 
 * @param parser The parser instance
 * @return AST node for the return statement
 */
static AstNode* return_statement(Parser* parser) {
    AstNode* node = ast_create_node(AST_RETURN);
    
    if (check(parser, TOKEN_SEMICOLON)) {
        node->data.return_stmt.value = NULL;
    } else {
        node->data.return_stmt.value = expression(parser);
    }
    
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after return value");
    return node;
}

/**
 * Parses a break statement.
 * 
 * @param parser The parser instance
 * @return AST node for the break statement
 */
static AstNode* break_statement(Parser* parser) {
    AstNode* node = ast_create_node(AST_BREAK);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after 'break'");
    return node;
}

/**
 * Parses a continue statement.
 * 
 * @param parser The parser instance
 * @return AST node for the continue statement
 */
static AstNode* continue_statement(Parser* parser) {
    AstNode* node = ast_create_node(AST_CONTINUE);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after 'continue'");
    return node;
}

/**
 * Parses a let statement for variable declarations.
 * Handles mutability, type annotations, and initializers.
 * 
 * @param parser The parser instance
 * @return AST node for the let statement
 */
static AstNode* let_statement(Parser* parser) {
    Token* let_token = previous(parser);
    AstNode* node = create_node_with_location(parser, AST_LET, let_token);
    
    node->data.let_stmt.is_mutable = match(parser, TOKEN_MUT);
    
    Token* name = consume(parser, TOKEN_IDENTIFIER, "Expected variable name");
    if (name) {
        node->data.let_stmt.name = string_n_duplicate(name->start, name->length);
    }
    
    if (match(parser, TOKEN_COLON)) {
        node->data.let_stmt.type = parse_type(parser);
    } else {
        node->data.let_stmt.type = NULL;
    }
    
    if (match(parser, TOKEN_EQ)) {
        node->data.let_stmt.value = expression(parser);
    } else {
        node->data.let_stmt.value = NULL;
    }
    
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration");
    return node;
}

/**
 * Parses an expression statement.
 * 
 * @param parser The parser instance
 * @return AST node for the expression statement
 */
static AstNode* expression_statement(Parser* parser) {
    AstNode* expr = expression(parser);
    
    if (check(parser, TOKEN_SEMICOLON)) {
        consume(parser, TOKEN_SEMICOLON, "Expected ';' after expression");
    }
    
    return expr;
}

/**
 * Dispatches to the appropriate statement parser based on current token.
 * 
 * @param parser The parser instance
 * @return AST node for the statement
 */
static AstNode* statement(Parser* parser) {
    if (match(parser, TOKEN_IF)) return if_statement(parser);
    if (match(parser, TOKEN_WHILE)) return while_statement(parser);
    if (match(parser, TOKEN_FOR)) return for_statement(parser);
    if (match(parser, TOKEN_LOOP)) return loop_statement(parser);
    if (match(parser, TOKEN_RETURN)) return return_statement(parser);
    if (match(parser, TOKEN_BREAK)) return break_statement(parser);
    if (match(parser, TOKEN_CONTINUE)) return continue_statement(parser);
    if (match(parser, TOKEN_LBRACE)) return block_statement(parser);
    
    return expression_statement(parser);
}

/**
 * Parses a function declaration including parameters and body.
 * 
 * @param parser The parser instance
 * @return AST node for the function declaration
 */
static AstNode* function_declaration(Parser* parser) {
    AstNode* node = ast_create_node(AST_FUNCTION);
    
    Token* name = consume(parser, TOKEN_IDENTIFIER, "Expected function name");
    if (name) {
        node->data.function.name = string_n_duplicate(name->start, name->length);
    }
    
    consume(parser, TOKEN_LPAREN, "Expected '(' after function name");
    
    size_t param_capacity = 8;
    node->data.function.params = malloc(sizeof(Param) * param_capacity);
    node->data.function.param_count = 0;
    
    if (!check(parser, TOKEN_RPAREN)) {
        do {
            if (node->data.function.param_count >= param_capacity) {
                param_capacity *= 2;
                node->data.function.params = realloc(node->data.function.params, sizeof(Param) * param_capacity);
            }
            
            Param* param = &node->data.function.params[node->data.function.param_count++];
            
            Token* param_name = consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
            if (param_name) {
                param->name = string_n_duplicate(param_name->start, param_name->length);
            }
            
            consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
            param->type = parse_type(parser);
        } while (match(parser, TOKEN_COMMA));
    }
    
    consume(parser, TOKEN_RPAREN, "Expected ')' after parameters");
    
    if (match(parser, TOKEN_ARROW)) {
        node->data.function.return_type = parse_type(parser);
    } else {
        node->data.function.return_type = type_create(TYPE_VOID);
    }
    
    consume(parser, TOKEN_LBRACE, "Expected '{' before function body");
    node->data.function.body = block_statement(parser);
    
    return node;
}

/**
 * Parses a struct declaration with fields.
 * Includes loop guard to prevent infinite loops.
 * 
 * @param parser The parser instance
 * @return AST node for the struct declaration
 */
static AstNode* struct_declaration(Parser* parser) {
    AstNode* node = ast_create_node(AST_STRUCT);
    node->data.struct_def.is_extern = false;
    
    Token* name = consume(parser, TOKEN_IDENTIFIER, "Expected struct name");
    if (name) {
        node->data.struct_def.name = string_n_duplicate(name->start, name->length);
    }
    
    consume(parser, TOKEN_LBRACE, "Expected '{' after struct name");
    
    size_t field_capacity = 8;
    node->data.struct_def.fields = malloc(sizeof(Field) * field_capacity);
    node->data.struct_def.field_count = 0;
    
    int loop_guard = 0;
    const int MAX_FIELDS = 1000;
    
    while (!check(parser, TOKEN_RBRACE) && !is_at_end(parser)) {
        if (++loop_guard > MAX_FIELDS) {
            error_at_current(parser, "Too many struct fields or parser stuck in loop");
            break;
        }
        
        if (node->data.struct_def.field_count >= field_capacity) {
            field_capacity *= 2;
            node->data.struct_def.fields = realloc(node->data.struct_def.fields, sizeof(Field) * field_capacity);
        }
        
        Field* field = &node->data.struct_def.fields[node->data.struct_def.field_count++];
        
        Token* field_name = consume(parser, TOKEN_IDENTIFIER, "Expected field name");
        if (field_name) {
            field->name = string_n_duplicate(field_name->start, field_name->length);
        } else {
            break;
        }
        
        if (!consume(parser, TOKEN_COLON, "Expected ':' after field name")) {
            break;
        }
        field->type = parse_type(parser);
        
        if (!match(parser, TOKEN_COMMA)) break;
    }
    
    consume(parser, TOKEN_RBRACE, "Expected '}' after struct fields");
    return node;
}

/**
 * Parses an impl block for struct methods.
 * Includes loop guard and progress tracking to prevent infinite loops.
 * 
 * @param parser The parser instance
 * @return AST node for the impl block
 */
static AstNode* impl_block(Parser* parser) {
    Token* impl_token = previous(parser);  // The 'impl' token
    AstNode* node = create_node_with_location(parser, AST_IMPL, impl_token);
    
    Token* name = consume(parser, TOKEN_IDENTIFIER, "Expected struct name after 'impl'");
    if (name) {
        node->data.impl_block.struct_name = string_n_duplicate(name->start, name->length);
    }
    
    consume(parser, TOKEN_LBRACE, "Expected '{' after struct name");
    
    size_t fn_capacity = 8;
    node->data.impl_block.functions = malloc(sizeof(AstNode*) * fn_capacity);
    node->data.impl_block.function_count = 0;
    
    int loop_guard = 0;
    const int MAX_IMPL_FUNCTIONS = 500;
    size_t prev_position = (size_t)-1;  // Initialize to invalid value
    
    
    while (!check(parser, TOKEN_RBRACE) && !is_at_end(parser)) {
        if (++loop_guard > MAX_IMPL_FUNCTIONS) {
            error_at_current(parser, "Too many functions in impl block or parser stuck in loop");
            break;
        }
        
        // Only check for stuck parser after first iteration
        if (prev_position != (size_t)-1 && parser->current == prev_position) {
            error_at_current(parser, "Parser stuck in impl block parsing");
            advance(parser);
            if (loop_guard > 10) break;
        }
        prev_position = parser->current;
        
        if (match(parser, TOKEN_FN)) {
            if (node->data.impl_block.function_count >= fn_capacity) {
                fn_capacity *= 2;
                node->data.impl_block.functions = realloc(node->data.impl_block.functions, sizeof(AstNode*) * fn_capacity);
            }
            
            node->data.impl_block.functions[node->data.impl_block.function_count++] = function_declaration(parser);
        } else {
            error_at_current(parser, "Expected 'fn' in impl block");
            synchronize(parser);
        }
    }
    
    consume(parser, TOKEN_RBRACE, "Expected '}' after impl block");
    return node;
}

/**
 * Parses extern declarations (extern fn or extern struct).
 * 
 * @param parser The parser instance
 * @return AST node for the extern declaration
 */
static AstNode* extern_declaration(Parser* parser) {
    if (match(parser, TOKEN_STRUCT)) {
        AstNode* node = ast_create_node(AST_STRUCT);
        node->data.struct_def.is_extern = true;
        
        Token* name = consume(parser, TOKEN_IDENTIFIER, "Expected struct name");
        if (name) {
            node->data.struct_def.name = string_n_duplicate(name->start, name->length);
        }
        
        consume(parser, TOKEN_SEMICOLON, "Expected ';' after extern struct declaration");
        
        node->data.struct_def.fields = NULL;
        node->data.struct_def.field_count = 0;
        
        return node;
    }
    
    consume(parser, TOKEN_FN, "Expected 'fn' or 'struct' after 'extern'");
    
    AstNode* node = ast_create_node(AST_EXTERN_FUNCTION);
    node->data.extern_function.is_extern = true;
    
    Token* name = consume(parser, TOKEN_IDENTIFIER, "Expected function name");
    if (name) {
        node->data.extern_function.name = string_n_duplicate(name->start, name->length);
    }
    
    consume(parser, TOKEN_LPAREN, "Expected '(' after function name");
    
    size_t param_capacity = 8;
    node->data.extern_function.params = malloc(sizeof(Param) * param_capacity);
    node->data.extern_function.param_count = 0;
    
    if (!check(parser, TOKEN_RPAREN)) {
        do {
            if (node->data.extern_function.param_count >= param_capacity) {
                param_capacity *= 2;
                node->data.extern_function.params = realloc(node->data.extern_function.params, sizeof(Param) * param_capacity);
            }
            
            Param* param = &node->data.extern_function.params[node->data.extern_function.param_count++];
            
            Token* param_name = consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
            if (param_name) {
                param->name = string_n_duplicate(param_name->start, param_name->length);
            }
            
            consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
            param->type = parse_type(parser);
        } while (match(parser, TOKEN_COMMA));
    }
    
    consume(parser, TOKEN_RPAREN, "Expected ')' after parameters");
    
    if (match(parser, TOKEN_ARROW)) {
        node->data.extern_function.return_type = parse_type(parser);
    } else {
        node->data.extern_function.return_type = type_create(TYPE_VOID);
    }
    
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after extern function declaration");
    
    return node;
}

/**
 * Parses an include directive for C headers.
 * 
 * @param parser The parser instance
 * @return AST node for the include directive
 */
static AstNode* include_directive(Parser* parser) {
    AstNode* node = ast_create_node(AST_INCLUDE);
    
    consume(parser, TOKEN_LPAREN, "Expected '(' after 'include'");
    
    Token* path_token = consume(parser, TOKEN_STRING_LITERAL, "Expected string literal for include path");
    if (path_token) {
        node->data.include.path = string_n_duplicate(path_token->start + 1, path_token->length - 2);
        node->data.include.is_system = true;
    }
    
    consume(parser, TOKEN_RPAREN, "Expected ')' after include path");
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after include statement");
    
    return node;
}

/**
 * Dispatches to the appropriate declaration parser based on current token.
 * Handles top-level declarations and falls back to statements.
 * 
 * @param parser The parser instance
 * @return AST node for the declaration
 */
static AstNode* declaration(Parser* parser) {
    if (match(parser, TOKEN_INCLUDE)) return include_directive(parser);
    if (match(parser, TOKEN_EXTERN)) return extern_declaration(parser);
    if (match(parser, TOKEN_FN)) return function_declaration(parser);
    if (match(parser, TOKEN_STRUCT)) return struct_declaration(parser);
    if (match(parser, TOKEN_IMPL)) return impl_block(parser);
    if (match(parser, TOKEN_LET)) return let_statement(parser);
    
    return statement(parser);
}

/**
 * Creates a new parser instance for the given token stream.
 * 
 * @param tokens Array of tokens to parse
 * @param token_count Number of tokens in the array
 * @return Newly allocated parser instance
 */
Parser* parser_create(Token* tokens, size_t token_count) {
    Parser* parser = malloc(sizeof(Parser));
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current = 0;
    parser->had_error = false;
    parser->panic_mode = false;
    parser->errors = error_list_create();
    return parser;
}

/**
 * Destroys a parser instance and frees its memory.
 * 
 * @param parser The parser instance to destroy
 */
void parser_destroy(Parser* parser) {
    if (parser) {
        error_list_destroy(parser->errors);
        free(parser);
    }
}

/**
 * Main parsing function that builds the complete AST.
 * Includes loop guards and progress tracking to prevent infinite loops.
 * 
 * @param parser The parser instance
 * @return AST node representing the entire program
 */
AstNode* parser_parse(Parser* parser) {
    AstNode* program = ast_create_node(AST_PROGRAM);
    
    size_t capacity = 16;
    program->data.program.items = malloc(sizeof(AstNode*) * capacity);
    program->data.program.count = 0;
    
    int loop_guard = 0;
    const int MAX_DECLARATIONS = 50000;
    size_t prev_position = parser->current;
    int stuck_count = 0;
    
    while (!is_at_end(parser)) {
        if (++loop_guard > MAX_DECLARATIONS) {
            error_at_current(parser, "Program too large or parser stuck in infinite loop");
            break;
        }
        
        if (parser->current == prev_position) {
            stuck_count++;
            if (stuck_count > 5) {
                error_at_current(parser, "Parser stuck at same position - forcing advance");
                advance(parser);
                if (stuck_count > 10) {
                    error_at_current(parser, "Parser repeatedly stuck - aborting parse");
                    break;
                }
            }
        } else {
            stuck_count = 0;
        }
        prev_position = parser->current;
        
        if (program->data.program.count >= capacity) {
            capacity *= 2;
            program->data.program.items = realloc(program->data.program.items, sizeof(AstNode*) * capacity);
        }
        
        AstNode* decl = declaration(parser);
        if (decl) {
            program->data.program.items[program->data.program.count++] = decl;
        }
        
        if (parser->panic_mode) {
            synchronize(parser);
        }
    }
    
    return program;
}

/**
 * Prints all accumulated parser errors to stderr.
 * 
 * @param parser The parser instance
 */
void parser_print_errors(Parser* parser) {
    error_list_print(parser->errors);
}