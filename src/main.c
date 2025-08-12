#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"
#include "utils.h"

void print_usage(const char* program_name) {
    printf("Usage: %s <input.jfm> [options]\n", program_name);
    printf("  Transpiles JFM source code to C\n");
    printf("\nOptions:\n");
    printf("  -o <file>    Output C file (default: compile to exe)\n");
    printf("  -e, --exe    Compile to executable (requires gcc)\n");
    printf("  -l <lib>     Link with library (e.g., -lGL -lglut)\n");
    printf("  -h, --help   Show this help message\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    
    const char* input_file = argv[1];
    const char* output_file = NULL;
    bool compile_to_exe = false;
    
    // Collect linker flags
    char linker_flags[1024] = "";
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--exe") == 0) {
            compile_to_exe = true;
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            strcat(linker_flags, " -l");
            strcat(linker_flags, argv[++i]);
        } else if (strncmp(argv[i], "-l", 2) == 0) {
            strcat(linker_flags, " ");
            strcat(linker_flags, argv[i]);
        }
    }
    
    char* source = read_file(input_file);
    if (!source) {
        fprintf(stderr, "Error: Could not read file '%s'\n", input_file);
        return 1;
    }
    
    Lexer* lexer = lexer_create(source);
    Token* tokens = lexer_scan_tokens(lexer);
    
    bool lexer_error = false;
    for (size_t i = 0; i < lexer->token_count; i++) {
        if (tokens[i].type == TOKEN_ERROR) {
            fprintf(stderr, "Lexical error: %s\n", tokens[i].start);
            lexer_error = true;
        }
    }
    
    if (lexer_error) {
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    Parser* parser = parser_create(tokens, lexer->token_count);
    AstNode* ast = parser_parse(parser);
    
    if (parser->had_error) {
        parser_print_errors(parser);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    SemanticAnalyzer* analyzer = semantic_create();
    if (!semantic_analyze(analyzer, ast)) {
        error_list_print(analyzer->errors);
        semantic_destroy(analyzer);
        ast_destroy(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        free(source);
        return 1;
    }
    
    // Default behavior: compile to executable
    // Unless -o is specified for C output only
    char temp_c_file[256];
    char exe_name[256];
    bool using_temp = false;
    
    if (!output_file && !compile_to_exe) {
        // Default: compile to exe with same name as input
        using_temp = true;
        compile_to_exe = true;
        
        // Extract base name from input file
        const char* base = strrchr(input_file, '/');
        if (!base) base = strrchr(input_file, '\\');
        if (!base) base = input_file;
        else base++;
        
        // Create exe name by removing .jfm extension
        strncpy(exe_name, base, sizeof(exe_name) - 1);
        char* dot = strrchr(exe_name, '.');
        if (dot && strcmp(dot, ".jfm") == 0) {
            *dot = '\0';
        }
        
        // Create temp C file name
        snprintf(temp_c_file, sizeof(temp_c_file), "%s_temp.c", exe_name);
        output_file = temp_c_file;
    }
    
    FILE* output = stdout;
    if (output_file) {
        output = fopen(output_file, "w");
        if (!output) {
            fprintf(stderr, "Error: Could not open output file '%s'\n", output_file);
            semantic_destroy(analyzer);
            ast_destroy(ast);
            parser_destroy(parser);
            lexer_destroy(lexer);
            free(source);
            return 1;
        }
    }
    
    CodeGenerator* gen = codegen_create(output);
    codegen_generate(gen, ast, analyzer->symbols);
    codegen_destroy(gen);
    
    if (output_file) {
        fclose(output);
    }
    
    // Compile to executable if requested
    if (compile_to_exe && output_file) {
        char gcc_cmd[512];
        if (!using_temp) {
            // User specified output file, create exe with same base name
            strncpy(exe_name, output_file, sizeof(exe_name) - 1);
            char* dot = strrchr(exe_name, '.');
            if (dot) *dot = '\0';
        }
        
        snprintf(gcc_cmd, sizeof(gcc_cmd), "gcc -o %s.exe %s -lm%s", exe_name, output_file, linker_flags);
        int result = system(gcc_cmd);
        
        // Clean up temp file if we created one
        if (using_temp) {
            remove(output_file);
        }
        
        if (result != 0) {
            fprintf(stderr, "Error: Failed to compile C code with gcc\n");
            semantic_destroy(analyzer);
            ast_destroy(ast);
            parser_destroy(parser);
            lexer_destroy(lexer);
            free(source);
            return 1;
        }
    }
    
    semantic_destroy(analyzer);
    ast_destroy(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    free(source);
    
    return 0;
}