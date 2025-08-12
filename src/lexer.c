#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>

// Forward declarations
static void skip_whitespace(Lexer* lexer);

/**
 * Adds a token to the lexer's dynamic token array.
 * Automatically resizes the array when capacity is reached.
 * 
 * @param lexer The lexer instance
 * @param token The token to add to the array
 */
static void add_token(Lexer* lexer, Token token) {
    if (lexer->token_count >= lexer->token_capacity) {
        lexer->token_capacity = lexer->token_capacity == 0 ? 16 : lexer->token_capacity * 2;
        lexer->tokens = realloc(lexer->tokens, sizeof(Token) * lexer->token_capacity);
    }
    lexer->tokens[lexer->token_count++] = token;
}

/**
 * Checks if the lexer has reached the end of the source string.
 * 
 * @param lexer The lexer instance
 * @return true if at end of input, false otherwise
 */
static bool is_at_end(Lexer* lexer) {
    return *lexer->current == '\0';
}

/**
 * Advances the lexer by one character and returns it.
 * Updates line and column tracking for error reporting.
 * 
 * @param lexer The lexer instance
 * @return The character that was consumed
 */
static char advance(Lexer* lexer) {
    char c = *lexer->current++;
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

/**
 * Returns the current character without consuming it.
 * 
 * @param lexer The lexer instance
 * @return The current character
 */
static char peek(Lexer* lexer) {
    return *lexer->current;
}

/**
 * Returns the next character (lookahead by 1) without consuming.
 * 
 * @param lexer The lexer instance
 * @return The next character, or '\0' if at end
 */
static char peek_next(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

/**
 * Checks if the current character matches the expected character.
 * If it matches, consumes the character and returns true.
 * 
 * @param lexer The lexer instance
 * @param expected The character to match
 * @return true if matched and consumed, false otherwise
 */
static bool match(Lexer* lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->current != expected) return false;
    advance(lexer);
    return true;
}

/**
 * Skips whitespace and comments in the source.
 * Handles both single-line (//) and multi-line comments.
 * 
 * @param lexer The lexer instance
 */
static void skip_whitespace(Lexer* lexer) {
    while (true) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
            case '\n':
                advance(lexer);
                break;
            case '/':
                if (peek_next(lexer) == '/') {
                    while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                        advance(lexer);
                    }
                } else if (peek_next(lexer) == '*') {
                    advance(lexer);
                    advance(lexer);
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                            advance(lexer);
                            advance(lexer);
                            break;
                        }
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

/**
 * Checks if a character is alphabetic or underscore.
 * 
 * @param c The character to check
 * @return true if alphabetic or underscore, false otherwise
 */
static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/**
 * Checks if a character is a digit (0-9).
 * 
 * @param c The character to check
 * @return true if digit, false otherwise
 */
static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

/**
 * Checks if a character is alphanumeric (letter, digit, or underscore).
 * 
 * @param c The character to check
 * @return true if alphanumeric, false otherwise
 */
static bool is_alphanumeric(char c) {
    return is_alpha(c) || is_digit(c);
}

/**
 * Creates a token with explicit position information.
 * 
 * @param lexer The lexer instance
 * @param type The type of token to create
 * @param start Pointer to the start of the token in source
 * @param line Line number where token starts
 * @param column Column number where token starts
 * @return The created token
 */
static Token make_token_with_pos(Lexer* lexer, TokenType type, const char* start, size_t line, size_t column) {
    Token token = {
        .type = type,
        .start = start,
        .length = (size_t)(lexer->current - start),
        .line = line,
        .column = column
    };
    return token;
}

/**
 * Creates a token using the current lexer position.
 * 
 * @param lexer The lexer instance
 * @param type The type of token to create
 * @param start Pointer to the start of the token in source
 * @return The created token
 */
static Token make_token(Lexer* lexer, TokenType type, const char* start) {
    size_t col = lexer->column - (size_t)(lexer->current - start);
    return make_token_with_pos(lexer, type, start, lexer->line, col);
}

/**
 * Creates an error token with a descriptive message.
 * 
 * @param lexer The lexer instance
 * @param message The error message
 * @return An error token containing the message
 */
static Token error_token(Lexer* lexer, const char* message) {
    Token token = {
        .type = TOKEN_ERROR,
        .start = message,
        .length = strlen(message),
        .line = lexer->line,
        .column = lexer->column
    };
    return token;
}

/**
 * Scans a string literal from the source.
 * Handles escape sequences and detects unterminated strings.
 * 
 * @param lexer The lexer instance
 * @param start_line Line where the string started
 * @param start_column Column where the string started
 * @return A string literal token or error token
 */
static Token scan_string(Lexer* lexer, size_t start_line, size_t start_column) {
    const char* start = lexer->current - 1;
    
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\\') {
            advance(lexer);
            if (!is_at_end(lexer)) {
                advance(lexer);
            }
        } else {
            advance(lexer);
        }
    }
    
    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated string");
    }
    
    advance(lexer);
    return make_token_with_pos(lexer, TOKEN_STRING_LITERAL, start, start_line, start_column);
}

/**
 * Scans a character literal from the source.
 * Handles escape sequences (\n, \t, \r, \\, \', \", \0).
 * 
 * @param lexer The lexer instance  
 * @param start_line Line where the character literal started
 * @param start_column Column where the character literal started
 * @return A character literal token with parsed value or error token
 */
static Token scan_char(Lexer* lexer, size_t start_line, size_t start_column) {
    const char* start = lexer->current - 1;
    
    if (peek(lexer) == '\\') {
        advance(lexer);
        if (!is_at_end(lexer)) {
            advance(lexer);
        }
    } else if (!is_at_end(lexer)) {
        advance(lexer);
    }
    
    if (peek(lexer) != '\'') {
        return error_token(lexer, "Invalid character literal");
    }
    
    advance(lexer);
    
    Token token = make_token_with_pos(lexer, TOKEN_CHAR_LITERAL, start, start_line, start_column);
    
    if (token.length == 3) {
        token.value.char_value = start[1];
    } else if (token.length == 4 && start[1] == '\\') {
        switch (start[2]) {
            case 'n': token.value.char_value = '\n'; break;
            case 't': token.value.char_value = '\t'; break;
            case 'r': token.value.char_value = '\r'; break;
            case '\\': token.value.char_value = '\\'; break;
            case '\'': token.value.char_value = '\''; break;
            case '"': token.value.char_value = '"'; break;
            case '0': token.value.char_value = '\0'; break;
            default: token.value.char_value = start[2]; break;
        }
    }
    
    return token;
}

/**
 * Scans a numeric literal (integer or floating-point).
 * Supports decimal notation and scientific notation (e.g., 1.5e-10).
 * 
 * @param lexer The lexer instance
 * @param start_line Line where the number started
 * @param start_column Column where the number started
 * @return An integer or float literal token with parsed value
 */
static Token scan_number(Lexer* lexer, size_t start_line, size_t start_column) {
    const char* start = lexer->current - 1;
    bool is_float = false;
    
    while (is_digit(peek(lexer))) {
        advance(lexer);
    }
    
    if (peek(lexer) == '.' && is_digit(peek_next(lexer))) {
        is_float = true;
        advance(lexer);
        while (is_digit(peek(lexer))) {
            advance(lexer);
        }
    }
    
    if (peek(lexer) == 'e' || peek(lexer) == 'E') {
        is_float = true;
        advance(lexer);
        if (peek(lexer) == '+' || peek(lexer) == '-') {
            advance(lexer);
        }
        while (is_digit(peek(lexer))) {
            advance(lexer);
        }
    }
    
    Token token = make_token_with_pos(lexer, is_float ? TOKEN_FLOAT_LITERAL : TOKEN_INT_LITERAL, start, start_line, start_column);
    
    if (is_float) {
        token.value.float_value = strtod(start, NULL);
    } else {
        token.value.int_value = strtoll(start, NULL, 10);
    }
    
    return token;
}

/**
 * Helper function to check if a string matches a keyword.
 * 
 * @param start Pointer to the string to check
 * @param length Length of the string
 * @param rest The keyword to match against
 * @param type The token type to return if matched
 * @return The keyword token type if matched, TOKEN_IDENTIFIER otherwise
 */
static TokenType check_keyword(const char* start, size_t length, const char* rest, TokenType type) {
    if (strlen(rest) == length && memcmp(start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

/**
 * Determines if an identifier is a keyword or regular identifier.
 * Uses a trie-like structure for efficient keyword matching.
 * 
 * @param start Pointer to the identifier string
 * @param length Length of the identifier
 * @return The appropriate token type (keyword or identifier)
 */
static TokenType identifier_type(const char* start, size_t length) {
    switch (start[0]) {
        case 'a':
            if (length == 2) return check_keyword(start, length, "as", TOKEN_AS);
            break;
        case 'b':
            if (length == 4) return check_keyword(start, length, "bool", TOKEN_BOOL);
            if (length == 5) return check_keyword(start, length, "break", TOKEN_BREAK);
            break;
        case 'c':
            if (length == 4) return check_keyword(start, length, "char", TOKEN_CHAR);
            if (length == 8) return check_keyword(start, length, "continue", TOKEN_CONTINUE);
            break;
        case 'e':
            if (length == 4) return check_keyword(start, length, "else", TOKEN_ELSE);
            if (length == 6) return check_keyword(start, length, "extern", TOKEN_EXTERN);
            break;
        case 'f':
            if (length == 2) return check_keyword(start, length, "fn", TOKEN_FN);
            if (length == 3) {
                if (memcmp(start, "for", 3) == 0) return TOKEN_FOR;
                if (memcmp(start, "f32", 3) == 0) return TOKEN_F32;
                if (memcmp(start, "f64", 3) == 0) return TOKEN_F64;
            }
            if (length == 5) return check_keyword(start, length, "false", TOKEN_FALSE);
            break;
        case 'i':
            if (length == 2) {
                if (memcmp(start, "if", 2) == 0) return TOKEN_IF;
                if (memcmp(start, "in", 2) == 0) return TOKEN_IN;
                if (memcmp(start, "i8", 2) == 0) return TOKEN_I8;
            }
            if (length == 3) {
                if (memcmp(start, "i16", 3) == 0) return TOKEN_I16;
                if (memcmp(start, "i32", 3) == 0) return TOKEN_I32;
                if (memcmp(start, "i64", 3) == 0) return TOKEN_I64;
            }
            if (length == 4) return check_keyword(start, length, "impl", TOKEN_IMPL);
            if (length == 7) return check_keyword(start, length, "include", TOKEN_INCLUDE);
            break;
        case 'l':
            if (length == 3) return check_keyword(start, length, "let", TOKEN_LET);
            if (length == 4) return check_keyword(start, length, "loop", TOKEN_LOOP);
            break;
        case 'm':
            if (length == 3) return check_keyword(start, length, "mut", TOKEN_MUT);
            break;
        case 'r':
            if (length == 6) return check_keyword(start, length, "return", TOKEN_RETURN);
            break;
        case 's':
            if (length == 3) return check_keyword(start, length, "str", TOKEN_STR);
            if (length == 6) return check_keyword(start, length, "struct", TOKEN_STRUCT);
            break;
        case 't':
            if (length == 4) return check_keyword(start, length, "true", TOKEN_TRUE);
            break;
        case 'u':
            if (length == 2) return check_keyword(start, length, "u8", TOKEN_U8);
            if (length == 3) {
                if (memcmp(start, "u16", 3) == 0) return TOKEN_U16;
                if (memcmp(start, "u32", 3) == 0) return TOKEN_U32;
                if (memcmp(start, "u64", 3) == 0) return TOKEN_U64;
            }
            break;
        case 'w':
            if (length == 5) return check_keyword(start, length, "while", TOKEN_WHILE);
            break;
    }
    return TOKEN_IDENTIFIER;
}

/**
 * Scans an identifier or keyword from the source.
 * Identifiers start with a letter or underscore and can contain alphanumeric characters.
 * 
 * @param lexer The lexer instance
 * @param start_line Line where the identifier started
 * @param start_column Column where the identifier started
 * @return An identifier or keyword token
 */
static Token scan_identifier(Lexer* lexer, size_t start_line, size_t start_column) {
    const char* start = lexer->current - 1;
    
    while (is_alphanumeric(peek(lexer))) {
        advance(lexer);
    }
    
    size_t length = (size_t)(lexer->current - start);
    TokenType type = identifier_type(start, length);
    
    Token token = make_token_with_pos(lexer, type, start, start_line, start_column);
    
    if (type == TOKEN_TRUE) {
        token.value.bool_value = true;
    } else if (type == TOKEN_FALSE) {
        token.value.bool_value = false;
    }
    
    return token;
}

/**
 * Scans and returns the next token from the source.
 * This is the main lexer function that dispatches to specific scanners
 * based on the first character of the token.
 * 
 * @param lexer The lexer instance
 * @return The next token from the source
 */
static Token scan_token(Lexer* lexer) {
    skip_whitespace(lexer);
    
    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF, lexer->current);
    }
    
    const char* start = lexer->current;
    size_t start_column = lexer->column;
    size_t start_line = lexer->line;
    char c = advance(lexer);
    
    if (is_alpha(c)) {
        lexer->current = start;
        lexer->column = start_column;
        lexer->line = start_line;
        advance(lexer);
        return scan_identifier(lexer, start_line, start_column);
    }
    
    if (is_digit(c)) {
        lexer->current = start;
        lexer->column = start_column;
        lexer->line = start_line;
        advance(lexer);
        return scan_number(lexer, start_line, start_column);
    }
    
    switch (c) {
        case '(': return make_token_with_pos(lexer, TOKEN_LPAREN, start, start_line, start_column);
        case ')': return make_token_with_pos(lexer, TOKEN_RPAREN, start, start_line, start_column);
        case '{': return make_token_with_pos(lexer, TOKEN_LBRACE, start, start_line, start_column);
        case '}': return make_token_with_pos(lexer, TOKEN_RBRACE, start, start_line, start_column);
        case '[': return make_token_with_pos(lexer, TOKEN_LBRACKET, start, start_line, start_column);
        case ']': return make_token_with_pos(lexer, TOKEN_RBRACKET, start, start_line, start_column);
        case ';': return make_token_with_pos(lexer, TOKEN_SEMICOLON, start, start_line, start_column);
        case ',': return make_token_with_pos(lexer, TOKEN_COMMA, start, start_line, start_column);
        case '%': return make_token_with_pos(lexer, TOKEN_PERCENT, start, start_line, start_column);
        case '^': return make_token_with_pos(lexer, TOKEN_XOR, start, start_line, start_column);
        
        case ':':
            if (match(lexer, ':')) {
                return make_token_with_pos(lexer, TOKEN_DOUBLE_COLON, start, start_line, start_column);
            }
            return make_token_with_pos(lexer, TOKEN_COLON, start, start_line, start_column);
            
        case '.':
            if (match(lexer, '.')) {
                return make_token_with_pos(lexer, TOKEN_DOT_DOT, start, start_line, start_column);
            }
            return make_token_with_pos(lexer, TOKEN_DOT, start, start_line, start_column);
            
        case '+':
            if (match(lexer, '=')) {
                return make_token_with_pos(lexer, TOKEN_PLUS_EQ, start, start_line, start_column);
            }
            return make_token_with_pos(lexer, TOKEN_PLUS, start, start_line, start_column);
            
        case '-':
            if (match(lexer, '=')) {
                return make_token_with_pos(lexer, TOKEN_MINUS_EQ, start, start_line, start_column);
            } else if (match(lexer, '>')) {
                return make_token_with_pos(lexer, TOKEN_ARROW, start, start_line, start_column);
            }
            return make_token_with_pos(lexer, TOKEN_MINUS, start, start_line, start_column);
            
        case '*':
            if (match(lexer, '=')) {
                return make_token_with_pos(lexer, TOKEN_STAR_EQ, start, start_line, start_column);
            }
            return make_token_with_pos(lexer, TOKEN_STAR, start, start_line, start_column);
            
        case '/':
            if (match(lexer, '=')) {
                return make_token_with_pos(lexer, TOKEN_SLASH_EQ, start, start_line, start_column);
            }
            return make_token_with_pos(lexer, TOKEN_SLASH, start, start_line, start_column);
            
        case '!':
            if (match(lexer, '=')) {
                return make_token_with_pos(lexer, TOKEN_NOT_EQ, start, start_line, start_column);
            }
            return make_token_with_pos(lexer, TOKEN_NOT, start, start_line, start_column);
            
        case '=':
            if (match(lexer, '=')) {
                return make_token_with_pos(lexer, TOKEN_EQ_EQ, start, start_line, start_column);
            }
            return make_token_with_pos(lexer, TOKEN_EQ, start, start_line, start_column);
            
        case '<':
            if (match(lexer, '=')) {
                return make_token_with_pos(lexer, TOKEN_LT_EQ, start, start_line, start_column);
            } else if (match(lexer, '<')) {
                return make_token_with_pos(lexer, TOKEN_LT_LT, start, start_line, start_column);
            }
            return make_token_with_pos(lexer, TOKEN_LT, start, start_line, start_column);
            
        case '>':
            if (match(lexer, '=')) {
                return make_token_with_pos(lexer, TOKEN_GT_EQ, start, start_line, start_column);
            } else if (match(lexer, '>')) {
                return make_token_with_pos(lexer, TOKEN_GT_GT, start, start_line, start_column);
            }
            return make_token_with_pos(lexer, TOKEN_GT, start, start_line, start_column);
            
        case '&':
            if (match(lexer, '&')) {
                return make_token_with_pos(lexer, TOKEN_AND_AND, start, start_line, start_column);
            }
            return make_token_with_pos(lexer, TOKEN_AND, start, start_line, start_column);
            
        case '|':
            if (match(lexer, '|')) {
                return make_token_with_pos(lexer, TOKEN_OR_OR, start, start_line, start_column);
            }
            return make_token_with_pos(lexer, TOKEN_OR, start, start_line, start_column);
            
        case '"':
            return scan_string(lexer, start_line, start_column);
            
        case '\'':
            return scan_char(lexer, start_line, start_column);
            
        default:
            return error_token(lexer, "Unexpected character");
    }
}

/**
 * Creates a new lexer instance for the given source string.
 * 
 * @param source The source code to tokenize
 * @return A newly allocated lexer instance
 */
Lexer* lexer_create(const char* source) {
    Lexer* lexer = malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;
    lexer->tokens = NULL;
    lexer->token_count = 0;
    lexer->token_capacity = 0;
    return lexer;
}

/**
 * Destroys a lexer instance and frees its memory.
 * 
 * @param lexer The lexer instance to destroy
 */
void lexer_destroy(Lexer* lexer) {
    if (lexer) {
        free(lexer->tokens);
        free(lexer);
    }
}

/**
 * Scans the entire source and returns an array of tokens.
 * Stops scanning on first error. Always ends with an EOF token.
 * 
 * @param lexer The lexer instance
 * @return Array of tokens (owned by the lexer)
 */
Token* lexer_scan_tokens(Lexer* lexer) {
    while (!is_at_end(lexer)) {
        Token token = scan_token(lexer);
        add_token(lexer, token);
        if (token.type == TOKEN_ERROR) {
            break;
        }
    }
    
    if (lexer->token_count == 0 || lexer->tokens[lexer->token_count - 1].type != TOKEN_EOF) {
        add_token(lexer, make_token(lexer, TOKEN_EOF, lexer->current));
    }
    
    return lexer->tokens;
}

/**
 * Converts a token type to its string representation.
 * Used for debugging and error messages.
 * 
 * @param type The token type
 * @return String representation of the token type
 */
const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        case TOKEN_FN: return "FN";
        case TOKEN_LET: return "LET";
        case TOKEN_MUT: return "MUT";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_FOR: return "FOR";
        case TOKEN_LOOP: return "LOOP";
        case TOKEN_BREAK: return "BREAK";
        case TOKEN_CONTINUE: return "CONTINUE";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_STRUCT: return "STRUCT";
        case TOKEN_IMPL: return "IMPL";
        case TOKEN_IN: return "IN";
        case TOKEN_I8: return "I8";
        case TOKEN_I16: return "I16";
        case TOKEN_I32: return "I32";
        case TOKEN_I64: return "I64";
        case TOKEN_U8: return "U8";
        case TOKEN_U16: return "U16";
        case TOKEN_U32: return "U32";
        case TOKEN_U64: return "U64";
        case TOKEN_F32: return "F32";
        case TOKEN_F64: return "F64";
        case TOKEN_BOOL: return "BOOL";
        case TOKEN_CHAR: return "CHAR";
        case TOKEN_STR: return "STR";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_PERCENT: return "PERCENT";
        case TOKEN_EQ_EQ: return "EQ_EQ";
        case TOKEN_NOT_EQ: return "NOT_EQ";
        case TOKEN_LT: return "LT";
        case TOKEN_GT: return "GT";
        case TOKEN_LT_EQ: return "LT_EQ";
        case TOKEN_GT_EQ: return "GT_EQ";
        case TOKEN_AND_AND: return "AND_AND";
        case TOKEN_OR_OR: return "OR_OR";
        case TOKEN_NOT: return "NOT";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_XOR: return "XOR";
        case TOKEN_LT_LT: return "LT_LT";
        case TOKEN_GT_GT: return "GT_GT";
        case TOKEN_EQ: return "EQ";
        case TOKEN_PLUS_EQ: return "PLUS_EQ";
        case TOKEN_MINUS_EQ: return "MINUS_EQ";
        case TOKEN_STAR_EQ: return "STAR_EQ";
        case TOKEN_SLASH_EQ: return "SLASH_EQ";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COLON: return "COLON";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_DOT: return "DOT";
        case TOKEN_ARROW: return "ARROW";
        case TOKEN_DOT_DOT: return "DOT_DOT";
        case TOKEN_DOUBLE_COLON: return "DOUBLE_COLON";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_INT_LITERAL: return "INT_LITERAL";
        case TOKEN_FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TOKEN_STRING_LITERAL: return "STRING_LITERAL";
        case TOKEN_CHAR_LITERAL: return "CHAR_LITERAL";
        case TOKEN_TRUE: return "TRUE";
        case TOKEN_FALSE: return "FALSE";
        default: return "UNKNOWN";
    }
}

/**
 * Prints a token to stdout for debugging purposes.
 * Includes type, lexeme, position, and value (for literals).
 * 
 * @param token The token to print
 */
void token_print(Token* token) {
    printf("Token { type: %s, lexeme: \"%.*s\", line: %lu, column: %lu",
           token_type_to_string(token->type),
           (int)token->length, token->start,
           (unsigned long)token->line, (unsigned long)token->column);
    
    switch (token->type) {
        case TOKEN_INT_LITERAL:
            printf(", value: %" PRId64, token->value.int_value);
            break;
        case TOKEN_FLOAT_LITERAL:
            printf(", value: %f", token->value.float_value);
            break;
        case TOKEN_CHAR_LITERAL:
            printf(", value: '%c'", token->value.char_value);
            break;
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            printf(", value: %s", token->value.bool_value ? "true" : "false");
            break;
        default:
            break;
    }
    
    printf(" }\n");
}