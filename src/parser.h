#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include "error.h"

typedef struct {
    Token* tokens;
    size_t token_count;
    size_t current;
    bool had_error;
    bool panic_mode;
    ErrorList* errors;
} Parser;

Parser* parser_create(Token* tokens, size_t token_count);
void parser_destroy(Parser* parser);
AstNode* parser_parse(Parser* parser);
void parser_print_errors(Parser* parser);

#endif