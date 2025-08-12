#include "codegen.h"
#include "type.h"
#include "semantic.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static void generate_node(CodeGenerator* gen, AstNode* node);
static void generate_expression(CodeGenerator* gen, AstNode* expr);
static void generate_statement(CodeGenerator* gen, AstNode* stmt);
static void generate_type(CodeGenerator* gen, Type* type);
static const char* get_c_type(TypeKind kind);

/**
 * Creates a new code generator instance.
 * 
 * @param output File handle for generated C code output
 * @return Newly allocated code generator
 */
CodeGenerator* codegen_create(FILE* output) {
    CodeGenerator* gen = calloc(1, sizeof(CodeGenerator));
    gen->output = output;
    gen->indent_level = 0;
    gen->in_struct_init = false;
    return gen;
}

/**
 * Destroys a code generator instance and frees its memory.
 * 
 * @param gen The code generator to destroy
 */
void codegen_destroy(CodeGenerator* gen) {
    if (gen) {
        free(gen);
    }
}

/**
 * Writes appropriate indentation based on current indent level.
 * 
 * @param gen The code generator instance
 */
void codegen_indent(CodeGenerator* gen) {
    for (int i = 0; i < gen->indent_level; i++) {
        fprintf(gen->output, "    ");
    }
}

/**
 * Writes formatted output without newline or indentation.
 * 
 * @param gen The code generator instance
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void codegen_write(CodeGenerator* gen, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(gen->output, format, args);
    va_end(args);
}

/**
 * Writes formatted output with indentation and newline.
 * 
 * @param gen The code generator instance
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void codegen_writeln(CodeGenerator* gen, const char* format, ...) {
    codegen_indent(gen);
    va_list args;
    va_start(args, format);
    vfprintf(gen->output, format, args);
    va_end(args);
    fprintf(gen->output, "\n");
}

/**
 * Converts a JFM primitive type to its C equivalent string.
 * 
 * @param kind The type kind to convert
 * @return String representation of the C type
 */
static const char* get_c_type(TypeKind kind) {
    switch (kind) {
        case TYPE_I8:  return "int8_t";
        case TYPE_I16: return "int16_t";
        case TYPE_I32: return "int32_t";
        case TYPE_I64: return "int64_t";
        case TYPE_U8:  return "uint8_t";
        case TYPE_U16: return "uint16_t";
        case TYPE_U32: return "uint32_t";
        case TYPE_U64: return "uint64_t";
        case TYPE_F32: return "float";
        case TYPE_F64: return "double";
        case TYPE_BOOL: return "_Bool";
        case TYPE_CHAR: return "char";
        case TYPE_VOID: return "void";
        case TYPE_STR:  return "const char*";
        default: return "unknown";
    }
}

/**
 * Generates C type declaration for any JFM type.
 * Handles arrays, pointers, references, and structs.
 * 
 * @param gen The code generator instance
 * @param type The type to generate
 */
static void generate_type(CodeGenerator* gen, Type* type) {
    if (!type) {
        codegen_write(gen, "void");
        return;
    }
    
    switch (type->kind) {
        case TYPE_ARRAY:
            generate_type(gen, type->data.array.element_type);
            break;
            
        case TYPE_POINTER:
            generate_type(gen, type->data.pointer.pointed_type);
            codegen_write(gen, "*");
            break;
            
        case TYPE_REFERENCE:
            if (type->data.reference.is_mutable) {
                generate_type(gen, type->data.reference.referenced_type);
                codegen_write(gen, "*");
            } else {
                codegen_write(gen, "const ");
                generate_type(gen, type->data.reference.referenced_type);
                codegen_write(gen, "*");
            }
            break;
            
        case TYPE_STRUCT:
            codegen_write(gen, "%s", type->data.struct_type.name);
            break;
            
        default:
            codegen_write(gen, "%s", get_c_type(type->kind));
            break;
    }
}

/**
 * Generates C code for binary operations.
 * Wraps expressions in parentheses to preserve precedence.
 * 
 * @param gen The code generator instance
 * @param expr The binary operation AST node
 */
static void generate_binary_op(CodeGenerator* gen, AstNode* expr) {
    codegen_write(gen, "(");
    generate_expression(gen, expr->data.binary.left);
    
    switch (expr->data.binary.op) {
        case TOKEN_PLUS:          codegen_write(gen, " + "); break;
        case TOKEN_MINUS:         codegen_write(gen, " - "); break;
        case TOKEN_STAR:          codegen_write(gen, " * "); break;
        case TOKEN_SLASH:         codegen_write(gen, " / "); break;
        case TOKEN_PERCENT:       codegen_write(gen, " %% "); break;
        case TOKEN_EQ_EQ:         codegen_write(gen, " == "); break;
        case TOKEN_NOT_EQ:        codegen_write(gen, " != "); break;
        case TOKEN_LT:            codegen_write(gen, " < "); break;
        case TOKEN_LT_EQ:         codegen_write(gen, " <= "); break;
        case TOKEN_GT:            codegen_write(gen, " > "); break;
        case TOKEN_GT_EQ:         codegen_write(gen, " >= "); break;
        case TOKEN_AND_AND:       codegen_write(gen, " && "); break;
        case TOKEN_OR_OR:         codegen_write(gen, " || "); break;
        case TOKEN_AND:           codegen_write(gen, " & "); break;
        case TOKEN_OR:            codegen_write(gen, " | "); break;
        case TOKEN_XOR:           codegen_write(gen, " ^ "); break;
        case TOKEN_LT_LT:         codegen_write(gen, " << "); break;
        case TOKEN_GT_GT:         codegen_write(gen, " >> "); break;
        default:
            codegen_write(gen, " ? ");
            break;
    }
    
    generate_expression(gen, expr->data.binary.right);
    codegen_write(gen, ")");
}

/**
 * Generates C code for unary operations (-, !, &, *).
 * Handles special case where &array becomes just array in C.
 * 
 * @param gen The code generator instance
 * @param expr The unary operation AST node
 */
static void generate_unary_op(CodeGenerator* gen, AstNode* expr) {
    switch (expr->data.unary.op) {
        case TOKEN_MINUS:
            codegen_write(gen, "-");
            generate_expression(gen, expr->data.unary.operand);
            break;
        case TOKEN_NOT:
            codegen_write(gen, "!");
            generate_expression(gen, expr->data.unary.operand);
            break;
        case TOKEN_AND:
            if (expr->data.unary.operand->data_type && 
                expr->data.unary.operand->data_type->kind == TYPE_ARRAY) {
                generate_expression(gen, expr->data.unary.operand);
            } else {
                codegen_write(gen, "&");
                generate_expression(gen, expr->data.unary.operand);
            }
            break;
        case TOKEN_STAR:
            codegen_write(gen, "*");
            generate_expression(gen, expr->data.unary.operand);
            break;
        default:
            generate_expression(gen, expr->data.unary.operand);
            break;
    }
}

/**
 * Generates C code for function and method calls.
 * Handles built-in functions (print, println, sqrt) and struct methods.
 * Methods are translated to C-style functions with struct name prefix.
 * 
 * @param gen The code generator instance
 * @param expr The call expression AST node
 */
static void generate_call(CodeGenerator* gen, AstNode* expr) {
    if (expr->data.call.function->type == AST_FIELD) {
        AstNode* field = expr->data.call.function;
        Type* obj_type = field->data.field.object->data_type;
        
        const char* struct_name = NULL;
        if (obj_type && obj_type->kind == TYPE_STRUCT) {
            struct_name = obj_type->data.struct_type.name;
        }
        
        if (struct_name) {
            codegen_write(gen, "%s_%s(", struct_name, field->data.field.field_name);
            generate_expression(gen, field->data.field.object);
            if (expr->data.call.argument_count > 0) {
                codegen_write(gen, ", ");
            }
        } else {
            codegen_write(gen, "/* ERROR: method call on non-struct */");
            return;
        }
    } else if (expr->data.call.function->type == AST_IDENTIFIER) {
        const char* func_name = expr->data.call.function->data.identifier.name;
        
        if (strcmp(func_name, "println") == 0) {
            if (expr->data.call.argument_count > 0) {
                Type* arg_type = expr->data.call.arguments[0]->data_type;
                if (arg_type) {
                    if (arg_type->kind == TYPE_STR) {
                        codegen_write(gen, "printf(\"%%s\\n\", ");
                    } else if (type_is_integral(arg_type)) {
                        if (type_is_signed(arg_type)) {
                            codegen_write(gen, "printf(\"%%lld\\n\", (long long)");
                        } else {
                            codegen_write(gen, "printf(\"%%llu\\n\", (unsigned long long)");
                        }
                    } else if (arg_type->kind == TYPE_F32 || arg_type->kind == TYPE_F64) {
                        codegen_write(gen, "printf(\"%%f\\n\", ");
                    } else if (arg_type->kind == TYPE_BOOL) {
                        codegen_write(gen, "printf(\"%%s\\n\", ");
                        generate_expression(gen, expr->data.call.arguments[0]);
                        codegen_write(gen, " ? \"true\" : \"false\"");
                        codegen_write(gen, ")");
                        return;
                    } else if (arg_type->kind == TYPE_CHAR) {
                        codegen_write(gen, "printf(\"%%c\\n\", ");
                    } else {
                        codegen_write(gen, "printf(\"<unknown>\\n\")");
                        return;
                    }
                    generate_expression(gen, expr->data.call.arguments[0]);
                    codegen_write(gen, ")");
                } else {
                    codegen_write(gen, "printf(\"\\n\")");
                }
            } else {
                codegen_write(gen, "printf(\"\\n\")");
            }
            return;
        } else if (strcmp(func_name, "print") == 0) {
            if (expr->data.call.argument_count > 0) {
                Type* arg_type = expr->data.call.arguments[0]->data_type;
                if (arg_type) {
                    if (arg_type->kind == TYPE_STR) {
                        codegen_write(gen, "printf(\"%%s\", ");
                    } else if (type_is_integral(arg_type)) {
                        if (type_is_signed(arg_type)) {
                            codegen_write(gen, "printf(\"%%lld\", (long long)");
                        } else {
                            codegen_write(gen, "printf(\"%%llu\", (unsigned long long)");
                        }
                    } else if (arg_type->kind == TYPE_F32 || arg_type->kind == TYPE_F64) {
                        codegen_write(gen, "printf(\"%%f\", ");
                    } else if (arg_type->kind == TYPE_BOOL) {
                        codegen_write(gen, "printf(\"%%s\", ");
                        generate_expression(gen, expr->data.call.arguments[0]);
                        codegen_write(gen, " ? \"true\" : \"false\"");
                        codegen_write(gen, ")");
                        return;
                    } else if (arg_type->kind == TYPE_CHAR) {
                        codegen_write(gen, "printf(\"%%c\", ");
                    } else {
                        codegen_write(gen, "printf(\"<unknown>\")");
                        return;
                    }
                    generate_expression(gen, expr->data.call.arguments[0]);
                    codegen_write(gen, ")");
                }
            }
            return;
        } else if (strcmp(func_name, "sqrt") == 0) {
            codegen_write(gen, "sqrt(");
            if (expr->data.call.argument_count > 0) {
                generate_expression(gen, expr->data.call.arguments[0]);
            }
            codegen_write(gen, ")");
            return;
        }
        
        generate_expression(gen, expr->data.call.function);
        codegen_write(gen, "(");
    } else {
        generate_expression(gen, expr->data.call.function);
        codegen_write(gen, "(");
    }
    
    for (size_t i = 0; i < expr->data.call.argument_count; i++) {
        if (i > 0) codegen_write(gen, ", ");
        generate_expression(gen, expr->data.call.arguments[i]);
    }
    
    codegen_write(gen, ")");
}

/**
 * Main expression generation dispatcher.
 * Routes to appropriate generator based on expression type.
 * 
 * @param gen The code generator instance
 * @param expr The expression AST node
 */
static void generate_expression(CodeGenerator* gen, AstNode* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case AST_LITERAL:
            if (expr->data_type) {
                switch (expr->data_type->kind) {
                    case TYPE_I8:
                    case TYPE_I16:
                    case TYPE_I32:
                    case TYPE_I64:
                    case TYPE_U8:
                    case TYPE_U16:
                    case TYPE_U32:
                    case TYPE_U64:
                        codegen_write(gen, "%lld", expr->data.literal.int_value);
                        break;
                    case TYPE_F32:
                    case TYPE_F64:
                        codegen_write(gen, "%f", expr->data.literal.float_value);
                        break;
                    case TYPE_STR:
                        codegen_write(gen, "\"%s\"", expr->data.literal.string_value);
                        break;
                    case TYPE_BOOL:
                        codegen_write(gen, expr->data.literal.bool_value ? "1" : "0");
                        break;
                    case TYPE_CHAR:
                        codegen_write(gen, "'%c'", expr->data.literal.char_value);
                        break;
                    default:
                        codegen_write(gen, "/* unknown literal */");
                        break;
                }
            } else {
                codegen_write(gen, "/* untyped literal */");
            }
            break;
            
        case AST_IDENTIFIER: {
            const char* name = expr->data.identifier.name;
            const char* coloncolon = strstr(name, "::");
            if (coloncolon) {
                size_t prefix_len = coloncolon - name;
                codegen_write(gen, "%.*s_%s", (int)prefix_len, name, coloncolon + 2);
            } else {
                codegen_write(gen, "%s", name);
            }
            break;
        }
            
        case AST_BINARY_OP:
            generate_binary_op(gen, expr);
            break;
            
        case AST_UNARY_OP:
            generate_unary_op(gen, expr);
            break;
            
        case AST_CAST:
            codegen_write(gen, "(");
            generate_type(gen, expr->data.cast.target_type);
            codegen_write(gen, ")");
            generate_expression(gen, expr->data.cast.expression);
            break;
            
        case AST_CALL:
            generate_call(gen, expr);
            break;
            
        case AST_INDEX:
            generate_expression(gen, expr->data.index.array);
            codegen_write(gen, "[");
            generate_expression(gen, expr->data.index.index);
            codegen_write(gen, "]");
            break;
            
        case AST_FIELD:
            generate_expression(gen, expr->data.field.object);
            codegen_write(gen, ".%s", expr->data.field.field_name);
            break;
            
        case AST_ASSIGNMENT:
            generate_expression(gen, expr->data.assignment.target);
            codegen_write(gen, " = ");
            generate_expression(gen, expr->data.assignment.value);
            break;
            
        case AST_ARRAY_LITERAL:
            codegen_write(gen, "{");
            for (size_t i = 0; i < expr->data.array_literal.element_count; i++) {
                if (i > 0) codegen_write(gen, ", ");
                generate_expression(gen, expr->data.array_literal.elements[i]);
            }
            codegen_write(gen, "}");
            break;
            
        case AST_STRUCT_LITERAL:
            if (gen->in_struct_init) {
                codegen_write(gen, "{");
            } else {
                codegen_write(gen, "(%s){", expr->data.struct_literal.struct_name);
            }
            
            gen->in_struct_init = true;
            for (size_t i = 0; i < expr->data.struct_literal.field_count; i++) {
                if (i > 0) codegen_write(gen, ", ");
                codegen_write(gen, ".%s = ", expr->data.struct_literal.field_names[i]);
                generate_expression(gen, expr->data.struct_literal.field_values[i]);
            }
            gen->in_struct_init = false;
            
            codegen_write(gen, "}");
            break;
            
        default:
            codegen_write(gen, "/* unsupported expression */");
            break;
    }
}

/**
 * Generates C code for variable declarations (let statements).
 * Handles const qualification for immutable variables, type inference,
 * and special array declaration syntax.
 * 
 * @param gen The code generator instance
 * @param stmt The let statement AST node
 */
static void generate_let(CodeGenerator* gen, AstNode* stmt) {
    if (!stmt->data.let_stmt.is_mutable) {
        codegen_write(gen, "const ");
    }
    
    Type* type = stmt->data.let_stmt.type;
    if (!type && stmt->data.let_stmt.value) {
        type = stmt->data.let_stmt.value->data_type;
    }
    
    if (!type) {
        codegen_write(gen, "/* ERROR: missing type */ void");
        codegen_write(gen, " %s", stmt->data.let_stmt.name);
        if (stmt->data.let_stmt.value) {
            codegen_write(gen, " = ");
            generate_expression(gen, stmt->data.let_stmt.value);
        }
        codegen_write(gen, ";");
        return;
    }
    
    if (type->kind == TYPE_ARRAY) {
        generate_type(gen, type->data.array.element_type);
        codegen_write(gen, " %s[%zu]", 
                     stmt->data.let_stmt.name,
                     type->data.array.size);
    } else {
        generate_type(gen, type);
        codegen_write(gen, " %s", stmt->data.let_stmt.name);
    }
    
    if (stmt->data.let_stmt.value) {
        codegen_write(gen, " = ");
        generate_expression(gen, stmt->data.let_stmt.value);
    }
    
    codegen_write(gen, ";");
}

/**
 * Generates C code for if statements with optional else branches.
 * 
 * @param gen The code generator instance
 * @param stmt The if statement AST node
 */
static void generate_if(CodeGenerator* gen, AstNode* stmt) {
    codegen_write(gen, "if (");
    generate_expression(gen, stmt->data.if_stmt.condition);
    codegen_write(gen, ") ");
    
    generate_statement(gen, stmt->data.if_stmt.then_branch);
    
    if (stmt->data.if_stmt.else_branch) {
        codegen_indent(gen);
        codegen_write(gen, "else ");
        generate_statement(gen, stmt->data.if_stmt.else_branch);
    }
}

/**
 * Generates C code for while loops.
 * 
 * @param gen The code generator instance
 * @param stmt The while loop AST node
 */
static void generate_while(CodeGenerator* gen, AstNode* stmt) {
    codegen_write(gen, "while (");
    generate_expression(gen, stmt->data.while_loop.condition);
    codegen_write(gen, ") ");
    generate_statement(gen, stmt->data.while_loop.body);
}

/**
 * Generates C code for range-based for loops.
 * Converts JFM 'for i in start..end' to C-style for loop.
 * 
 * @param gen The code generator instance
 * @param stmt The for loop AST node
 */
static void generate_for(CodeGenerator* gen, AstNode* stmt) {
    // JFM for loops are range-based: for i in start..end
    // Convert to C: for (int i = start; i < end; i++)
    
    codegen_write(gen, "for (int %s = ", stmt->data.for_loop.iterator);
    generate_expression(gen, stmt->data.for_loop.start);
    codegen_write(gen, "; %s < ", stmt->data.for_loop.iterator);
    generate_expression(gen, stmt->data.for_loop.end);
    codegen_write(gen, "; %s++) ", stmt->data.for_loop.iterator);
    
    generate_statement(gen, stmt->data.for_loop.body);
}

/**
 * Generates C code for infinite loops.
 * Converts JFM 'loop' to C 'while(1)'.
 * 
 * @param gen The code generator instance
 * @param stmt The loop statement AST node
 */
static void generate_loop(CodeGenerator* gen, AstNode* stmt) {
    codegen_write(gen, "while (1) ");
    generate_statement(gen, stmt->data.loop_stmt.body);
}

/**
 * Main statement generation dispatcher.
 * Routes different statement types to their specific generators.
 * 
 * @param gen The code generator instance
 * @param stmt The statement AST node
 */
static void generate_statement(CodeGenerator* gen, AstNode* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_BLOCK:
            codegen_writeln(gen, "{");
            gen->indent_level++;
            
            for (size_t i = 0; i < stmt->data.block.statement_count; i++) {
                codegen_indent(gen);
                generate_statement(gen, stmt->data.block.statements[i]);
                if (stmt->data.block.statements[i]->type != AST_IF &&
                    stmt->data.block.statements[i]->type != AST_WHILE &&
                    stmt->data.block.statements[i]->type != AST_FOR &&
                    stmt->data.block.statements[i]->type != AST_LOOP &&
                    stmt->data.block.statements[i]->type != AST_BLOCK) {
                }
                codegen_write(gen, "\n");
            }
            
            gen->indent_level--;
            codegen_indent(gen);
            codegen_write(gen, "}");
            break;
            
        case AST_LET:
            generate_let(gen, stmt);
            break;
            
        case AST_IF:
            generate_if(gen, stmt);
            break;
            
        case AST_WHILE:
            generate_while(gen, stmt);
            break;
            
        case AST_FOR:
            generate_for(gen, stmt);
            break;
            
        case AST_LOOP:
            generate_loop(gen, stmt);
            break;
            
        case AST_RETURN:
            codegen_write(gen, "return");
            if (stmt->data.return_stmt.value) {
                codegen_write(gen, " ");
                generate_expression(gen, stmt->data.return_stmt.value);
            }
            codegen_write(gen, ";");
            break;
            
        case AST_BREAK:
            codegen_write(gen, "break;");
            break;
            
        case AST_CONTINUE:
            codegen_write(gen, "continue;");
            break;
            
        default:
            generate_expression(gen, stmt);
            codegen_write(gen, ";");
            break;
    }
}

/**
 * Generates C code for function definitions.
 * Includes return type, parameters, and function body.
 * 
 * @param gen The code generator instance
 * @param func The function AST node
 */
static void generate_function(CodeGenerator* gen, AstNode* func) {
    generate_type(gen, func->data.function.return_type);
    codegen_write(gen, " %s(", func->data.function.name);
    
    if (func->data.function.param_count == 0) {
        codegen_write(gen, "void");
    } else {
        for (size_t i = 0; i < func->data.function.param_count; i++) {
            if (i > 0) codegen_write(gen, ", ");
            generate_type(gen, func->data.function.params[i].type);
            codegen_write(gen, " %s", func->data.function.params[i].name);
        }
    }
    
    codegen_write(gen, ") ");
    
    generate_statement(gen, func->data.function.body);
    codegen_write(gen, "\n\n");
}

/**
 * Generates C code for struct definitions.
 * Creates typedef struct with all fields. Skips extern structs.
 * 
 * @param gen The code generator instance
 * @param struct_def The struct definition AST node
 */
static void generate_struct(CodeGenerator* gen, AstNode* struct_def) {
    if (struct_def->data.struct_def.is_extern) {
        return;
    }
    
    codegen_writeln(gen, "typedef struct %s {", struct_def->data.struct_def.name);
    gen->indent_level++;
    
    for (size_t i = 0; i < struct_def->data.struct_def.field_count; i++) {
        codegen_indent(gen);
        generate_type(gen, struct_def->data.struct_def.fields[i].type);
        codegen_write(gen, " %s;\n", struct_def->data.struct_def.fields[i].name);
    }
    
    gen->indent_level--;
    codegen_writeln(gen, "} %s;\n", struct_def->data.struct_def.name);
}

/**
 * Generates C code for impl blocks.
 * Converts methods to regular C functions with StructName_methodName naming.
 * 
 * @param gen The code generator instance
 * @param impl The impl block AST node
 */
static void generate_impl(CodeGenerator* gen, AstNode* impl) {
    for (size_t i = 0; i < impl->data.impl_block.function_count; i++) {
        AstNode* method = impl->data.impl_block.functions[i];
        
        generate_type(gen, method->data.function.return_type);
        
        codegen_write(gen, " %s_%s(", 
                     impl->data.impl_block.struct_name,
                     method->data.function.name);
        
        if (method->data.function.param_count == 0) {
            codegen_write(gen, "void");
        } else {
            for (size_t j = 0; j < method->data.function.param_count; j++) {
                if (j > 0) codegen_write(gen, ", ");
                generate_type(gen, method->data.function.params[j].type);
                codegen_write(gen, " %s", method->data.function.params[j].name);
            }
        }
        
        codegen_write(gen, ") ");
        
        generate_statement(gen, method->data.function.body);
        codegen_write(gen, "\n\n");
    }
}

/**
 * Generates C code for any AST node type.
 * Handles program-level organization and dispatches to specific generators.
 * 
 * @param gen The code generator instance
 * @param node The AST node to generate code for
 */
static void generate_node(CodeGenerator* gen, AstNode* node) {
    switch (node->type) {
        case AST_PROGRAM:
            codegen_writeln(gen, "/* Generated C code from JFM compiler */");
            codegen_writeln(gen, "#include <stdio.h>");
            codegen_writeln(gen, "#include <stdlib.h>");
            codegen_writeln(gen, "#include <stdint.h>");
            codegen_writeln(gen, "#include <stdbool.h>");
            codegen_writeln(gen, "#include <math.h>");
            
            for (size_t i = 0; i < node->data.program.count; i++) {
                if (node->data.program.items[i]->type == AST_INCLUDE) {
                    AstNode* inc = node->data.program.items[i];
                    if (inc->data.include.is_system) {
                        codegen_writeln(gen, "#include <%s>", inc->data.include.path);
                    } else {
                        codegen_writeln(gen, "#include \"%s\"", inc->data.include.path);
                    }
                }
            }
            codegen_writeln(gen, "");
            
            for (size_t i = 0; i < node->data.program.count; i++) {
                if (node->data.program.items[i]->type == AST_EXTERN_FUNCTION) {
                    continue;
                }
            }
            
            for (size_t i = 0; i < node->data.program.count; i++) {
                if (node->data.program.items[i]->type == AST_STRUCT) {
                    generate_struct(gen, node->data.program.items[i]);
                }
            }
            
            for (size_t i = 0; i < node->data.program.count; i++) {
                if (node->data.program.items[i]->type == AST_IMPL) {
                    generate_impl(gen, node->data.program.items[i]);
                }
            }
            
            for (size_t i = 0; i < node->data.program.count; i++) {
                if (node->data.program.items[i]->type == AST_FUNCTION) {
                    generate_function(gen, node->data.program.items[i]);
                }
            }
            break;
            
        case AST_FUNCTION:
            generate_function(gen, node);
            break;
            
        case AST_STRUCT:
            generate_struct(gen, node);
            break;
            
        case AST_IMPL:
            generate_impl(gen, node);
            break;
            
        default:
            generate_statement(gen, node);
            break;
    }
}

/**
 * Main entry point for code generation.
 * Generates complete C code from JFM AST.
 * 
 * @param gen The code generator instance
 * @param ast The root AST node (program)
 * @param symbols The symbol table for type information
 * @return true if generation succeeded, false otherwise
 */
bool codegen_generate(CodeGenerator* gen, AstNode* ast, SymbolTable* symbols) {
    if (!gen || !ast) return false;
    
    gen->symbols = symbols;
    generate_node(gen, ast);
    
    return true;
}