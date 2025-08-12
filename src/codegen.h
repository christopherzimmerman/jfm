#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    FILE* output;
    int indent_level;
    bool in_struct_init;
    SymbolTable* symbols;
} CodeGenerator;

CodeGenerator* codegen_create(FILE* output);
void codegen_destroy(CodeGenerator* gen);
bool codegen_generate(CodeGenerator* gen, AstNode* ast, SymbolTable* symbols);

void codegen_indent(CodeGenerator* gen);
void codegen_write(CodeGenerator* gen, const char* format, ...);
void codegen_writeln(CodeGenerator* gen, const char* format, ...);

#endif