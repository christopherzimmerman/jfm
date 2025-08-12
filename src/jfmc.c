/*
 * JFM Compiler - A Rust-like language that transpiles to C
 * 
 * Usage: jfmc [options] <input.jfm>
 * 
 * Options:
 *   -o <output>   Output file (default: <input>.c)
 *   -ast          Print AST to stdout and exit
 *   -tokens       Print tokens to stdout and exit
 *   -check        Only perform semantic analysis (no code generation)
 *   -v, -verbose  Verbose output
 *   -h, -help     Show this help message
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#ifdef _WIN32
#include <process.h>  // For _getpid on Windows
#define getpid _getpid
#else
#include <unistd.h>   // For getpid on Unix
#endif
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"
#include "ast.h"
#include "utils.h"

// Version information
#define VERSION "1.0.0"
#define AUTHOR "JFM Compiler Team"

// Command-line options
typedef struct {
    char* input_file;
    char* output_file;
    bool print_tokens;
    bool print_ast;
    bool print_semantic;
    bool print_c;
    bool print_all;  // Print all intermediate steps
    bool check_only;
    bool compile_exe;  // Compile to executable
    bool keep_c_file;  // Keep intermediate C file
    char* cc_flags;    // Additional flags for C compiler
    bool verbose;
} Options;

// Print usage information
static void print_usage(const char* program_name) {
    printf("JFM Compiler v%s\n", VERSION);
    printf("A Rust-like language that transpiles to C\n\n");
    printf("Usage: %s [options] <input.jfm>\n\n", program_name);
    printf("Options:\n");
    printf("  -o <output>     Output file (default: <input>.c or <input>.exe)\n");
    printf("  -e, --exe       Compile to executable (default)\n");
    printf("  --c-only        Only generate C code, don't compile\n");
    printf("  --keep-c        Keep intermediate C file when compiling to exe\n");
    printf("  --cc-flags <f>  Additional flags for C compiler (e.g., '-O2 -Wall')\n");
    printf("  --tokens        Print tokens to stdout\n");
    printf("  --ast           Print AST to stdout\n");
    printf("  --semantic      Print semantic analysis results\n");
    printf("  --c             Print generated C code to stdout\n");
    printf("  --all           Print all intermediate steps\n");
    printf("  --check         Only perform semantic analysis (no code generation)\n");
    printf("  -v, --verbose   Verbose output\n");
    printf("  -h, --help      Show this help message\n");
    printf("  --version       Show version information\n");
}

// Print version information
static void print_version(void) {
    printf("JFM Compiler v%s\n", VERSION);
    printf("Copyright (C) 2024 %s\n", AUTHOR);
    printf("License: MIT\n");
}

// Read entire file into memory
static char* read_source_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer
    char* buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, size, file);
    buffer[bytes_read] = '\0';
    
    fclose(file);
    return buffer;
}

// Generate default output filename
static char* get_default_output(const char* input_file, bool for_exe) {
    size_t len = strlen(input_file);
    const char* extension = for_exe ? ".exe" : ".c";
    size_t ext_len = strlen(extension);
    char* output = malloc(len + ext_len + 1);
    
    strcpy(output, input_file);
    
    // Replace .jfm extension with target extension
    if (len > 4 && strcmp(output + len - 4, ".jfm") == 0) {
        strcpy(output + len - 4, extension);
    } else {
        strcat(output, extension);
    }
    
    return output;
}

// Print tokens in a readable format
static void print_tokens_formatted(Token* tokens, size_t count) {
    printf("=== TOKENS ===\n");
    printf("%-20s %-15s %-10s %s\n", "Type", "Lexeme", "Line:Col", "Value");
    printf("--------------------------------------------------------------------------------\n");
    
    for (size_t i = 0; i < count; i++) {
        Token* t = &tokens[i];
        
        // Get token type name
        const char* type_name = "UNKNOWN";
        switch (t->type) {
            case TOKEN_EOF: type_name = "EOF"; break;
            case TOKEN_ERROR: type_name = "ERROR"; break;
            case TOKEN_FN: type_name = "FN"; break;
            case TOKEN_LET: type_name = "LET"; break;
            case TOKEN_MUT: type_name = "MUT"; break;
            case TOKEN_IF: type_name = "IF"; break;
            case TOKEN_ELSE: type_name = "ELSE"; break;
            case TOKEN_WHILE: type_name = "WHILE"; break;
            case TOKEN_FOR: type_name = "FOR"; break;
            case TOKEN_LOOP: type_name = "LOOP"; break;
            case TOKEN_BREAK: type_name = "BREAK"; break;
            case TOKEN_CONTINUE: type_name = "CONTINUE"; break;
            case TOKEN_RETURN: type_name = "RETURN"; break;
            case TOKEN_STRUCT: type_name = "STRUCT"; break;
            case TOKEN_IMPL: type_name = "IMPL"; break;
            case TOKEN_IN: type_name = "IN"; break;
            case TOKEN_INCLUDE: type_name = "INCLUDE"; break;
            case TOKEN_EXTERN: type_name = "EXTERN"; break;
            case TOKEN_TRUE: type_name = "TRUE"; break;
            case TOKEN_FALSE: type_name = "FALSE"; break;
            case TOKEN_I8: type_name = "I8"; break;
            case TOKEN_I16: type_name = "I16"; break;
            case TOKEN_I32: type_name = "I32"; break;
            case TOKEN_I64: type_name = "I64"; break;
            case TOKEN_U8: type_name = "U8"; break;
            case TOKEN_U16: type_name = "U16"; break;
            case TOKEN_U32: type_name = "U32"; break;
            case TOKEN_U64: type_name = "U64"; break;
            case TOKEN_F32: type_name = "F32"; break;
            case TOKEN_F64: type_name = "F64"; break;
            case TOKEN_BOOL: type_name = "BOOL"; break;
            case TOKEN_CHAR: type_name = "CHAR"; break;
            case TOKEN_STR: type_name = "STR"; break;
            case TOKEN_IDENTIFIER: type_name = "IDENTIFIER"; break;
            case TOKEN_INT_LITERAL: type_name = "INT_LITERAL"; break;
            case TOKEN_FLOAT_LITERAL: type_name = "FLOAT_LITERAL"; break;
            case TOKEN_STRING_LITERAL: type_name = "STRING_LITERAL"; break;
            case TOKEN_CHAR_LITERAL: type_name = "CHAR_LITERAL"; break;
            case TOKEN_PLUS: type_name = "PLUS"; break;
            case TOKEN_MINUS: type_name = "MINUS"; break;
            case TOKEN_STAR: type_name = "STAR"; break;
            case TOKEN_SLASH: type_name = "SLASH"; break;
            case TOKEN_PERCENT: type_name = "PERCENT"; break;
            case TOKEN_EQ: type_name = "EQ"; break;
            case TOKEN_EQ_EQ: type_name = "EQ_EQ"; break;
            case TOKEN_NOT_EQ: type_name = "NOT_EQ"; break;
            case TOKEN_LT: type_name = "LT"; break;
            case TOKEN_GT: type_name = "GT"; break;
            case TOKEN_LT_EQ: type_name = "LT_EQ"; break;
            case TOKEN_GT_EQ: type_name = "GT_EQ"; break;
            case TOKEN_AND_AND: type_name = "AND_AND"; break;
            case TOKEN_OR_OR: type_name = "OR_OR"; break;
            case TOKEN_NOT: type_name = "NOT"; break;
            case TOKEN_AND: type_name = "AND"; break;
            case TOKEN_OR: type_name = "OR"; break;
            case TOKEN_XOR: type_name = "XOR"; break;
            case TOKEN_LPAREN: type_name = "LPAREN"; break;
            case TOKEN_RPAREN: type_name = "RPAREN"; break;
            case TOKEN_LBRACE: type_name = "LBRACE"; break;
            case TOKEN_RBRACE: type_name = "RBRACE"; break;
            case TOKEN_LBRACKET: type_name = "LBRACKET"; break;
            case TOKEN_RBRACKET: type_name = "RBRACKET"; break;
            case TOKEN_SEMICOLON: type_name = "SEMICOLON"; break;
            case TOKEN_COLON: type_name = "COLON"; break;
            case TOKEN_COMMA: type_name = "COMMA"; break;
            case TOKEN_DOT: type_name = "DOT"; break;
            case TOKEN_ARROW: type_name = "ARROW"; break;
            case TOKEN_DOT_DOT: type_name = "DOT_DOT"; break;
            default: break;
        }
        
        // Get lexeme
        char lexeme[32];
        if (t->length > 0 && t->length < 31) {
            strncpy(lexeme, t->start, t->length);
            lexeme[t->length] = '\0';
        } else if (t->length > 0) {
            strncpy(lexeme, t->start, 28);
            strcpy(lexeme + 28, "...");
        } else {
            strcpy(lexeme, "");
        }
        
        // Print token info
        printf("%-20s %-15s %3zu:%-6zu ", type_name, lexeme, t->line, t->column);
        
        // Print value if applicable
        switch (t->type) {
            case TOKEN_INT_LITERAL:
                printf("%lld", t->value.int_value);
                break;
            case TOKEN_FLOAT_LITERAL:
                printf("%f", t->value.float_value);
                break;
            case TOKEN_CHAR_LITERAL:
                printf("'%c'", t->value.char_value);
                break;
            default:
                break;
        }
        
        printf("\n");
        
        if (t->type == TOKEN_EOF) break;
    }
    
    printf("\nTotal tokens: %zu\n", count);
}

// Compile a JFM file
static int compile(Options* opts) {
    // Read source file
    if (opts->verbose) {
        printf("Reading %s...\n", opts->input_file);
    }
    
    char* source = read_source_file(opts->input_file);
    if (!source) {
        fprintf(stderr, "Error: Could not read file '%s'\n", opts->input_file);
        return 1;
    }
    
    // Lexical analysis
    if (opts->verbose) {
        printf("Performing lexical analysis...\n");
    }
    
    Lexer* lexer = lexer_create(source);
    Token* tokens = lexer_scan_tokens(lexer);
    
    // Check for lexer errors (tokens will include ERROR tokens if there were issues)
    bool had_lexer_error = false;
    for (size_t i = 0; tokens[i].type != TOKEN_EOF; i++) {
        if (tokens[i].type == TOKEN_ERROR) {
            had_lexer_error = true;
            break;
        }
    }
    
    if (had_lexer_error) {
        fprintf(stderr, "Error: Lexical analysis failed\n");
        lexer_destroy(lexer);
        free(tokens);
        free(source);
        return 1;
    }
    
    // Count tokens
    size_t token_count = 0;
    while (tokens[token_count].type != TOKEN_EOF) {
        token_count++;
    }
    token_count++;  // Include EOF token
    
    // Print tokens if requested
    if (opts->print_tokens) {
        print_tokens_formatted(tokens, token_count);
        if (!opts->print_ast && !opts->print_semantic && !opts->print_c && !opts->check_only) {
            // Only tokens requested, exit early
            lexer_destroy(lexer);
            free(tokens);
            free(source);
            return 0;
        }
        printf("\n");  // Add spacing between outputs
    }
    
    // Parsing
    if (opts->verbose) {
        printf("Parsing...\n");
    }
    
    Parser* parser = parser_create(tokens, token_count);
    AstNode* ast = parser_parse(parser);
    
    if (!ast) {
        fprintf(stderr, "Error: Parsing failed\n");
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(tokens);
        free(source);
        return 1;
    }
    
    // Print AST if requested
    if (opts->print_ast) {
        printf("=== ABSTRACT SYNTAX TREE ===\n");
        ast_print(ast, 0);
        if (!opts->print_semantic && !opts->print_c && !opts->check_only) {
            // Only AST requested, exit early
            ast_destroy(ast);
            parser_destroy(parser);
            lexer_destroy(lexer);
            free(tokens);
            free(source);
            return 0;
        }
        printf("\n");  // Add spacing between outputs
    }
    
    // Semantic analysis
    if (opts->verbose) {
        printf("Performing semantic analysis...\n");
    }
    
    SemanticAnalyzer* analyzer = semantic_create();
    semantic_set_source(analyzer, source, opts->input_file);
    bool semantic_ok = semantic_analyze(analyzer, ast);
    
    if (!semantic_ok) {
        // Use beautiful error reporting
        if (analyzer->errors->error_count > 0) {
            error_list_print_beautiful(analyzer->errors);
        } else {
            fprintf(stderr, "Error: Semantic analysis failed\n");
        }
        semantic_destroy(analyzer);
        ast_destroy(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(tokens);
        free(source);
        return 1;
    }
    
    if (opts->verbose || opts->print_semantic) {
        if (opts->print_semantic) {
            printf("=== SEMANTIC ANALYSIS ===\n");
        }
        printf("Semantic analysis complete:\n");
        printf("  Functions: %zu\n", analyzer->functions_analyzed);
        printf("  Structs: %zu\n", analyzer->structs_analyzed);
        printf("  Variables: %zu\n", analyzer->variables_analyzed);
        if (opts->print_semantic && !opts->print_c && !opts->check_only) {
            // Only semantic analysis requested, exit early
            semantic_destroy(analyzer);
            ast_destroy(ast);
            parser_destroy(parser);
            lexer_destroy(lexer);
            free(tokens);
            free(source);
            return 0;
        }
        if (opts->print_semantic) {
            printf("\n");  // Add spacing between outputs
        }
    }
    
    // Check-only mode - stop here
    if (opts->check_only) {
        printf("Semantic analysis successful - no errors found\n");
        semantic_destroy(analyzer);
        ast_destroy(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(tokens);
        free(source);
        return 0;
    }
    
    // Code generation
    if (opts->verbose) {
        printf("Generating C code...\n");
    }
    
    // Determine C output file (might be temporary)
    char c_file[256] = "";
    bool c_file_is_temp = false;
    
    if (opts->compile_exe && !opts->keep_c_file) {
        // Generate temporary C file name
        sprintf(c_file, "jfm_temp_%d.c", (int)getpid());
        c_file_is_temp = true;
    } else if (!opts->compile_exe && opts->output_file) {
        // User specified C output file
        strcpy(c_file, opts->output_file);
    } else if (!opts->compile_exe) {
        // Default C output file
        char* default_c = get_default_output(opts->input_file, false);
        strcpy(c_file, default_c);
        free(default_c);
    } else if (opts->keep_c_file) {
        // Compiling to exe but keeping C file
        char* default_c = get_default_output(opts->input_file, false);
        strcpy(c_file, default_c);
        free(default_c);
    }
    
    // Open C file for writing
    FILE* output = fopen(c_file, "w");
    if (!output) {
        fprintf(stderr, "Error: Could not create C file '%s'\n", c_file);
        semantic_destroy(analyzer);
        ast_destroy(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(tokens);
        free(source);
        return 1;
    }
    
    CodeGenerator* gen = codegen_create(output);
    bool codegen_ok = codegen_generate(gen, ast, analyzer->symbols);
    
    if (!codegen_ok) {
        fprintf(stderr, "Error: Code generation failed\n");
        fclose(output);
        if (c_file_is_temp) remove(c_file);
        codegen_destroy(gen);
        semantic_destroy(analyzer);
        ast_destroy(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(tokens);
        free(source);
        return 1;
    }
    
    fclose(output);
    
    // Print C code if requested
    if (opts->print_c) {
        printf("=== GENERATED C CODE ===\n");
        FILE* src = fopen(c_file, "r");
        if (src) {
            char buffer[1024];
            size_t bytes;
            while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                fwrite(buffer, 1, bytes, stdout);
            }
            fclose(src);
            printf("\n");
        }
    }
    
    // Compile to executable if requested
    if (opts->compile_exe) {
        if (opts->verbose) {
            printf("Compiling to executable...\n");
        }
        
        // Determine executable output file
        char* exe_file = opts->output_file;
        bool allocated_exe = false;
        if (!exe_file) {
            exe_file = get_default_output(opts->input_file, true);
            allocated_exe = true;
        }
        
        // Build compiler command
        char command[1024];
        snprintf(command, sizeof(command), "gcc -o \"%s\" \"%s\" -lm", 
                 exe_file, c_file);
        
        // Add user-specified flags
        if (opts->cc_flags) {
            strcat(command, " ");
            strcat(command, opts->cc_flags);
        }
        
        if (opts->verbose) {
            printf("Running: %s\n", command);
        }
        
        // Execute compiler
        int result = system(command);
        
        if (result != 0) {
            fprintf(stderr, "Error: C compilation failed\n");
            if (c_file_is_temp) remove(c_file);
            if (allocated_exe) free(exe_file);
            codegen_destroy(gen);
            semantic_destroy(analyzer);
            ast_destroy(ast);
            parser_destroy(parser);
            lexer_destroy(lexer);
            free(tokens);
            free(source);
            return 1;
        }
        
        if (opts->verbose) {
            printf("Successfully generated executable: %s\n", exe_file);
        }
        
        // Clean up temporary C file
        if (c_file_is_temp) {
            remove(c_file);
        }
        
        if (allocated_exe) free(exe_file);
    } else {
        if (opts->verbose) {
            printf("Successfully generated C file: %s\n", c_file);
        }
    }
    
    // Cleanup
    codegen_destroy(gen);
    semantic_destroy(analyzer);
    ast_destroy(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    free(tokens);
    free(source);
    
    return 0;
}

int main(int argc, char* argv[]) {
    Options opts = {0};
    
    // Parse command-line options
    static struct option long_options[] = {
        {"tokens",   no_argument,       0, 't'},
        {"ast",      no_argument,       0, 'a'},
        {"semantic", no_argument,       0, 's'},
        {"c",        no_argument,       0, 'C'},
        {"all",      no_argument,       0, 'A'},
        {"check",    no_argument,       0, 'c'},
        {"exe",      no_argument,       0, 'e'},
        {"c-only",   no_argument,       0, 'O'},
        {"keep-c",   no_argument,       0, 'k'},
        {"cc-flags", required_argument, 0, 'f'},
        {"verbose",  no_argument,       0, 'v'},
        {"help",     no_argument,       0, 'h'},
        {"version",  no_argument,       0, 'V'},
        {"output",   required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    // Default to compiling to executable
    opts.compile_exe = true;
    
    while ((c = getopt_long(argc, argv, "o:evhV", long_options, &option_index)) != -1) {
        switch (c) {
            case 't':
                opts.print_tokens = true;
                break;
            case 'a':
                opts.print_ast = true;
                break;
            case 's':
                opts.print_semantic = true;
                break;
            case 'C':
                opts.print_c = true;
                break;
            case 'A':
                opts.print_all = true;
                opts.print_tokens = true;
                opts.print_ast = true;
                opts.print_semantic = true;
                opts.print_c = true;
                break;
            case 'c':
                opts.check_only = true;
                break;
            case 'e':
                opts.compile_exe = true;
                break;
            case 'O':
                opts.compile_exe = false;
                break;
            case 'k':
                opts.keep_c_file = true;
                break;
            case 'f':
                opts.cc_flags = optarg;
                break;
            case 'v':
                opts.verbose = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'V':
                print_version();
                return 0;
            case 'o':
                opts.output_file = optarg;
                break;
            case '?':
                return 1;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Check for input file
    if (optind >= argc) {
        fprintf(stderr, "Error: No input file specified\n\n");
        print_usage(argv[0]);
        return 1;
    }
    
    opts.input_file = argv[optind];
    
    // Check file extension
    size_t len = strlen(opts.input_file);
    if (len < 4 || strcmp(opts.input_file + len - 4, ".jfm") != 0) {
        fprintf(stderr, "Warning: Input file does not have .jfm extension\n");
    }
    
    // Compile
    return compile(&opts);
}