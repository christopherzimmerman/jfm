#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ERROR,
    
    TOKEN_FN,
    TOKEN_LET,
    TOKEN_MUT,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_EXTERN,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_LOOP,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_RETURN,
    TOKEN_STRUCT,
    TOKEN_IMPL,
    TOKEN_IN,
    TOKEN_INCLUDE,
    TOKEN_AS,
    
    TOKEN_I8,
    TOKEN_I16,
    TOKEN_I32,
    TOKEN_I64,
    TOKEN_U8,
    TOKEN_U16,
    TOKEN_U32,
    TOKEN_U64,
    TOKEN_F32,
    TOKEN_F64,
    TOKEN_BOOL,
    TOKEN_CHAR,
    TOKEN_STR,
    
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    
    TOKEN_EQ_EQ,
    TOKEN_NOT_EQ,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LT_EQ,
    TOKEN_GT_EQ,
    
    TOKEN_AND_AND,
    TOKEN_OR_OR,
    TOKEN_NOT,
    
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_XOR,
    TOKEN_LT_LT,
    TOKEN_GT_GT,
    
    TOKEN_EQ,
    TOKEN_PLUS_EQ,
    TOKEN_MINUS_EQ,
    TOKEN_STAR_EQ,
    TOKEN_SLASH_EQ,
    
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_ARROW,
    TOKEN_DOT_DOT,
    TOKEN_DOUBLE_COLON,
    
    TOKEN_IDENTIFIER,
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_CHAR_LITERAL,
    TOKEN_TRUE,
    TOKEN_FALSE,
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    size_t length;
    size_t line;
    size_t column;
    
    union {
        long long int_value;
        double float_value;
        char char_value;
        bool bool_value;
    } value;
} Token;

typedef struct {
    const char* source;
    const char* current;
    size_t line;
    size_t column;
    Token* tokens;
    size_t token_count;
    size_t token_capacity;
} Lexer;

Lexer* lexer_create(const char* source);
void lexer_destroy(Lexer* lexer);
Token* lexer_scan_tokens(Lexer* lexer);
const char* token_type_to_string(TokenType type);
void token_print(Token* token);

#endif