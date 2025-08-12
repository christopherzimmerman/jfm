#include "ast.h"
#include "type.h"
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

/**
 * Creates a new AST node of the specified type.
 * 
 * @param type The type of AST node to create
 * @return Newly allocated AST node
 */
AstNode* ast_create_node(AstNodeType type) {
    AstNode* node = calloc(1, sizeof(AstNode));
    node->type = type;
    return node;
}

/**
 * Destroys an AST node and frees its memory.
 * Note: This is a shallow destroy - child nodes are not recursively freed.
 * 
 * @param node The AST node to destroy
 */
void ast_destroy(AstNode* node) {
    if (!node) return;
    free(node);
}

/**
 * Converts an AST node type to its string representation.
 * Used for debugging and pretty-printing.
 * 
 * @param type The AST node type
 * @return String representation of the type
 */
static const char* ast_type_to_string(AstNodeType type) {
    switch (type) {
        case AST_PROGRAM: return "Program";
        case AST_FUNCTION: return "Function";
        case AST_STRUCT: return "Struct";
        case AST_IMPL: return "Impl";
        case AST_BLOCK: return "Block";
        case AST_IF: return "If";
        case AST_WHILE: return "While";
        case AST_FOR: return "For";
        case AST_LOOP: return "Loop";
        case AST_RETURN: return "Return";
        case AST_BREAK: return "Break";
        case AST_CONTINUE: return "Continue";
        case AST_LET: return "Let";
        case AST_ASSIGNMENT: return "Assignment";
        case AST_BINARY_OP: return "BinaryOp";
        case AST_UNARY_OP: return "UnaryOp";
        case AST_CALL: return "Call";
        case AST_FIELD: return "Field";
        case AST_INDEX: return "Index";
        case AST_LITERAL: return "Literal";
        case AST_IDENTIFIER: return "Identifier";
        case AST_ARRAY_LITERAL: return "ArrayLiteral";
        case AST_STRUCT_LITERAL: return "StructLiteral";
        case AST_INCLUDE: return "Include";
        case AST_EXTERN_FUNCTION: return "ExternFunction";
        case AST_CAST: return "Cast";
        default: return "Unknown";
    }
}

/**
 * Converts a token type to its operator string representation.
 * Used for pretty-printing operators in AST dumps.
 * 
 * @param type The token type representing an operator
 * @return String representation of the operator
 */
static const char* token_type_to_op_string(TokenType type) {
    switch (type) {
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_STAR: return "*";
        case TOKEN_SLASH: return "/";
        case TOKEN_PERCENT: return "%";
        case TOKEN_EQ_EQ: return "==";
        case TOKEN_NOT_EQ: return "!=";
        case TOKEN_LT: return "<";
        case TOKEN_GT: return ">";
        case TOKEN_LT_EQ: return "<=";
        case TOKEN_GT_EQ: return ">=";
        case TOKEN_AND_AND: return "&&";
        case TOKEN_OR_OR: return "||";
        case TOKEN_NOT: return "!";
        case TOKEN_AND: return "&";
        case TOKEN_OR: return "|";
        case TOKEN_XOR: return "^";
        case TOKEN_LT_LT: return "<<";
        case TOKEN_GT_GT: return ">>";
        case TOKEN_EQ: return "=";
        case TOKEN_PLUS_EQ: return "+=";
        case TOKEN_MINUS_EQ: return "-=";
        case TOKEN_STAR_EQ: return "*=";
        case TOKEN_SLASH_EQ: return "/=";
        default: return "?";
    }
}

/**
 * Prints indentation with tree-like formatting for AST display.
 * 
 * @param indent The indentation level
 */
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        if (i == indent - 1) {
            printf("+- ");
        } else {
            printf("|  ");
        }
    }
}

/**
 * Pretty-prints an AST node and its children with indentation.
 * Provides detailed information about each node type and its data.
 * 
 * @param node The AST node to print
 * @param indent The current indentation level
 */
void ast_print(AstNode* node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("(null)\n");
        return;
    }
    
    print_indent(indent);
    printf("%s", ast_type_to_string(node->type));
    
    switch (node->type) {
        case AST_PROGRAM:
            printf(" (%lu items)\n", (unsigned long)node->data.program.count);
            for (size_t i = 0; i < node->data.program.count; i++) {
                ast_print(node->data.program.items[i], indent + 1);
            }
            break;
            
        case AST_FUNCTION:
            printf(" '%s'", node->data.function.name);
            if (node->data.function.param_count > 0) {
                printf(" (");
                for (size_t i = 0; i < node->data.function.param_count; i++) {
                    if (i > 0) printf(", ");
                    printf("%s", node->data.function.params[i].name);
                }
                printf(")");
            }
            printf("\n");
            ast_print(node->data.function.body, indent + 1);
            break;
            
        case AST_STRUCT:
            printf(" '%s' (%lu fields)\n", node->data.struct_def.name, 
                   (unsigned long)node->data.struct_def.field_count);
            for (size_t i = 0; i < node->data.struct_def.field_count; i++) {
                print_indent(indent + 1);
                printf("Field '%s'\n", node->data.struct_def.fields[i].name);
            }
            break;
            
        case AST_IMPL:
            printf(" for '%s' (%lu methods)\n", node->data.impl_block.struct_name,
                   (unsigned long)node->data.impl_block.function_count);
            for (size_t i = 0; i < node->data.impl_block.function_count; i++) {
                ast_print(node->data.impl_block.functions[i], indent + 1);
            }
            break;
            
        case AST_BLOCK:
            printf(" (%lu statements)\n", (unsigned long)node->data.block.statement_count);
            for (size_t i = 0; i < node->data.block.statement_count; i++) {
                ast_print(node->data.block.statements[i], indent + 1);
            }
            if (node->data.block.final_expr) {
                print_indent(indent + 1);
                printf("Final expression:\n");
                ast_print(node->data.block.final_expr, indent + 2);
            }
            break;
            
        case AST_IF:
            printf("\n");
            print_indent(indent + 1);
            printf("Condition:\n");
            ast_print(node->data.if_stmt.condition, indent + 2);
            print_indent(indent + 1);
            printf("Then:\n");
            ast_print(node->data.if_stmt.then_branch, indent + 2);
            if (node->data.if_stmt.else_branch) {
                print_indent(indent + 1);
                printf("Else:\n");
                ast_print(node->data.if_stmt.else_branch, indent + 2);
            }
            break;
            
        case AST_WHILE:
            printf("\n");
            print_indent(indent + 1);
            printf("Condition:\n");
            ast_print(node->data.while_loop.condition, indent + 2);
            print_indent(indent + 1);
            printf("Body:\n");
            ast_print(node->data.while_loop.body, indent + 2);
            break;
            
        case AST_FOR:
            printf(" '%s' in range\n", node->data.for_loop.iterator);
            print_indent(indent + 1);
            printf("Start:\n");
            ast_print(node->data.for_loop.start, indent + 2);
            print_indent(indent + 1);
            printf("End:\n");
            ast_print(node->data.for_loop.end, indent + 2);
            print_indent(indent + 1);
            printf("Body:\n");
            ast_print(node->data.for_loop.body, indent + 2);
            break;
            
        case AST_LOOP:
            printf("\n");
            ast_print(node->data.loop_stmt.body, indent + 1);
            break;
            
        case AST_RETURN:
            printf("\n");
            if (node->data.return_stmt.value) {
                ast_print(node->data.return_stmt.value, indent + 1);
            }
            break;
            
        case AST_BREAK:
        case AST_CONTINUE:
            printf("\n");
            break;
            
        case AST_LET:
            printf(" '%s'", node->data.let_stmt.name);
            if (node->data.let_stmt.is_mutable) printf(" (mutable)");
            printf("\n");
            if (node->data.let_stmt.value) {
                print_indent(indent + 1);
                printf("Value:\n");
                ast_print(node->data.let_stmt.value, indent + 2);
            }
            break;
            
        case AST_ASSIGNMENT:
            printf(" %s\n", token_type_to_op_string(node->data.assignment.op));
            print_indent(indent + 1);
            printf("Target:\n");
            ast_print(node->data.assignment.target, indent + 2);
            print_indent(indent + 1);
            printf("Value:\n");
            ast_print(node->data.assignment.value, indent + 2);
            break;
            
        case AST_BINARY_OP:
            printf(" %s\n", token_type_to_op_string(node->data.binary.op));
            print_indent(indent + 1);
            printf("Left:\n");
            ast_print(node->data.binary.left, indent + 2);
            print_indent(indent + 1);
            printf("Right:\n");
            ast_print(node->data.binary.right, indent + 2);
            break;
            
        case AST_UNARY_OP:
            printf(" %s\n", token_type_to_op_string(node->data.unary.op));
            ast_print(node->data.unary.operand, indent + 1);
            break;
            
        case AST_CAST:
            printf(" as ");
            if (node->data.cast.target_type) {
                printf("[Type]\n");
            } else {
                printf("[Unknown Type]\n");
            }
            print_indent(indent + 1);
            printf("Expression:\n");
            ast_print(node->data.cast.expression, indent + 2);
            break;
            
        case AST_CALL:
            printf("\n");
            print_indent(indent + 1);
            printf("Function:\n");
            ast_print(node->data.call.function, indent + 2);
            if (node->data.call.argument_count > 0) {
                print_indent(indent + 1);
                printf("Arguments (%lu):\n", (unsigned long)node->data.call.argument_count);
                for (size_t i = 0; i < node->data.call.argument_count; i++) {
                    ast_print(node->data.call.arguments[i], indent + 2);
                }
            }
            break;
            
        case AST_FIELD:
            printf(" .%s\n", node->data.field.field_name);
            print_indent(indent + 1);
            printf("Object:\n");
            ast_print(node->data.field.object, indent + 2);
            break;
            
        case AST_INDEX:
            printf("\n");
            print_indent(indent + 1);
            printf("Array:\n");
            ast_print(node->data.index.array, indent + 2);
            print_indent(indent + 1);
            printf("Index:\n");
            ast_print(node->data.index.index, indent + 2);
            break;
            
        case AST_LITERAL:
            if (node->data_type) {
                switch (node->data_type->kind) {
                    case TYPE_I32:
                    case TYPE_I64:
                        printf(" %"PRId64"\n", node->data.literal.int_value);
                        break;
                    case TYPE_F32:
                    case TYPE_F64:
                        printf(" %f\n", node->data.literal.float_value);
                        break;
                    case TYPE_BOOL:
                        printf(" %s\n", node->data.literal.bool_value ? "true" : "false");
                        break;
                    case TYPE_CHAR:
                        printf(" '%c'\n", node->data.literal.char_value);
                        break;
                    case TYPE_STR:
                        printf(" \"%s\"\n", node->data.literal.string_value);
                        break;
                    default:
                        printf("\n");
                }
            } else {
                printf("\n");
            }
            break;
            
        case AST_IDENTIFIER:
            printf(" '%s'\n", node->data.identifier.name);
            break;
            
        case AST_ARRAY_LITERAL:
            printf(" (%lu elements)\n", (unsigned long)node->data.array_literal.element_count);
            for (size_t i = 0; i < node->data.array_literal.element_count; i++) {
                ast_print(node->data.array_literal.elements[i], indent + 1);
            }
            break;
            
        case AST_STRUCT_LITERAL:
            printf(" %s\n", node->data.struct_literal.struct_name);
            for (size_t i = 0; i < node->data.struct_literal.field_count; i++) {
                print_indent(indent + 1);
                printf("Field '%s':\n", node->data.struct_literal.field_names[i]);
                ast_print(node->data.struct_literal.field_values[i], indent + 2);
            }
            break;
            
        case AST_INCLUDE:
            printf(" \"%s\"\n", node->data.include.path);
            break;
            
        case AST_EXTERN_FUNCTION:
            printf(" '%s'\n", node->data.extern_function.name);
            break;
            
        default:
            printf("\n");
            break;
    }
}