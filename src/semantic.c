#include "semantic.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


static void analyze_node(SemanticAnalyzer* analyzer, AstNode* node);
static Type* check_expression(SemanticAnalyzer* analyzer, AstNode* expr);
static void check_statement(SemanticAnalyzer* analyzer, AstNode* stmt);

/**
 * Creates a new semantic analyzer instance.
 * Initializes symbol table, error list, and analysis counters.
 * 
 * @return Newly allocated semantic analyzer
 */
SemanticAnalyzer* semantic_create(void) {
    SemanticAnalyzer* analyzer = calloc(1, sizeof(SemanticAnalyzer));
    analyzer->symbols = symbol_table_create();
    analyzer->errors = error_list_create();
    analyzer->success = true;
    analyzer->in_loop_count = 0;
    analyzer->functions_analyzed = 0;
    analyzer->structs_analyzed = 0;
    analyzer->variables_analyzed = 0;
    analyzer->source_code = NULL;
    analyzer->filename = NULL;
    return analyzer;
}

/**
 * Set source code and filename for error reporting
 */
void semantic_set_source(SemanticAnalyzer* analyzer, const char* source, const char* filename) {
    if (analyzer) {
        analyzer->source_code = source;
        analyzer->filename = filename;
        if (analyzer->errors) {
            error_list_set_source(analyzer->errors, source);
        }
    }
}

/**
 * Destroys a semantic analyzer and frees all associated memory.
 * 
 * @param analyzer The analyzer to destroy
 */
void semantic_destroy(SemanticAnalyzer* analyzer) {
    if (!analyzer) return;
    symbol_table_destroy(analyzer->symbols);
    error_list_destroy(analyzer->errors);
    free(analyzer);
}

/**
 * Reports a semantic error with formatted message.
 * Marks analysis as failed and adds error to the error list.
 * 
 * @param analyzer The semantic analyzer
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void semantic_error(SemanticAnalyzer* analyzer, const char* format, ...) {
    if (!analyzer || !format) return;
    
    analyzer->success = false;
    
    char buffer[512];
    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Debug: check for issues
    if (written < 0 || strlen(buffer) == 0) {
        fprintf(stderr, "WARNING: Error formatting message. Format='%s', written=%d\n", format, written);
        snprintf(buffer, sizeof(buffer), "Unknown error (format issue)");
    }
    
    error_list_add(analyzer->errors, buffer, analyzer->filename ? analyzer->filename : "semantic", 0, 0);
}

/**
 * Reports a semantic error at a specific source location.
 * 
 * @param analyzer The semantic analyzer
 * @param line Source line number
 * @param column Source column number
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void semantic_error_at(SemanticAnalyzer* analyzer, size_t line, size_t column, const char* format, ...) {
    analyzer->success = false;
    
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    error_list_add(analyzer->errors, buffer, analyzer->filename ? analyzer->filename : "semantic", line, column);
}

/**
 * Reports a semantic error with location extracted from AST node.
 * 
 * @param analyzer The semantic analyzer
 * @param node AST node to extract location from
 * @param format Printf-style format string
 * @param ... Format arguments
 */
void semantic_error_node(SemanticAnalyzer* analyzer, AstNode* node, const char* format, ...) {
    if (!analyzer || !format || !node) return;
    
    analyzer->success = false;
    
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    size_t line = node->location.line;
    size_t column = node->location.column;
    error_list_add(analyzer->errors, buffer, analyzer->filename ? analyzer->filename : "semantic", line, column);
}

/**
 * Checks if two types are exactly equal.
 * Performs deep comparison for complex types like arrays, pointers, and structs.
 * 
 * @param a First type to compare
 * @param b Second type to compare
 * @return true if types are equal, false otherwise
 */
bool types_equal(Type* a, Type* b) {
    if (!a || !b) return false;
    if (a->kind != b->kind) return false;
    
    switch (a->kind) {
        case TYPE_ARRAY:
            return types_equal(a->data.array.element_type, b->data.array.element_type) &&
                   a->data.array.size == b->data.array.size;
        
        case TYPE_POINTER:
            return types_equal(a->data.pointer.pointed_type, b->data.pointer.pointed_type);
        
        case TYPE_REFERENCE:
            return types_equal(a->data.reference.referenced_type, b->data.reference.referenced_type) &&
                   a->data.reference.is_mutable == b->data.reference.is_mutable;
        
        case TYPE_STRUCT:
            return strcmp(a->data.struct_type.name, b->data.struct_type.name) == 0;
        
        default:
            return true;  // Primitive types with same kind are equal
    }
}

/**
 * Checks if a type represents a numeric type (integer or floating-point).
 * 
 * @param type The type to check
 * @return true if type is numeric, false otherwise
 */
bool type_is_numeric(Type* type) {
    if (!type) return false;
    switch (type->kind) {
        case TYPE_I8:
        case TYPE_I16:
        case TYPE_I32:
        case TYPE_I64:
        case TYPE_U8:
        case TYPE_U16:
        case TYPE_U32:
        case TYPE_U64:
        case TYPE_F32:
        case TYPE_F64:
            return true;
        default:
            return false;
    }
}

/**
 * Checks if a type represents an integral type (any integer type).
 * 
 * @param type The type to check
 * @return true if type is integral, false otherwise
 */
bool type_is_integral(Type* type) {
    if (!type) return false;
    switch (type->kind) {
        case TYPE_I8:
        case TYPE_I16:
        case TYPE_I32:
        case TYPE_I64:
        case TYPE_U8:
        case TYPE_U16:
        case TYPE_U32:
        case TYPE_U64:
            return true;
        default:
            return false;
    }
}

/**
 * Checks if a type represents a signed type (signed integers and floats).
 * 
 * @param type The type to check
 * @return true if type is signed, false otherwise
 */
bool type_is_signed(Type* type) {
    if (!type) return false;
    switch (type->kind) {
        case TYPE_I8:
        case TYPE_I16:
        case TYPE_I32:
        case TYPE_I64:
        case TYPE_F32:
        case TYPE_F64:
            return true;
        default:
            return false;
    }
}

/**
 * Checks if a type is a reference type.
 * 
 * @param type The type to check
 * @return true if type is a reference, false otherwise
 */
bool type_is_reference(Type* type) {
    return type && type->kind == TYPE_REFERENCE;
}

/**
 * Checks if a type is a pointer type.
 * 
 * @param type The type to check
 * @return true if type is a pointer, false otherwise
 */
bool type_is_pointer(Type* type) {
    return type && type->kind == TYPE_POINTER;
}

/**
 * Dereferences a pointer or reference type to get the underlying type.
 * 
 * @param type The pointer or reference type to dereference
 * @return The dereferenced type, or NULL if not a pointer/reference
 */
Type* type_dereference(Type* type) {
    if (!type) return NULL;
    if (type->kind == TYPE_POINTER) {
        return type->data.pointer.pointed_type;
    }
    if (type->kind == TYPE_REFERENCE) {
        return type->data.reference.referenced_type;
    }
    return NULL;
}

/**
 * Checks if two types are compatible for assignment or parameter passing.
 * Allows integer widening and float conversions.
 * 
 * @param expected The expected/target type
 * @param actual The actual/source type
 * @return true if types are compatible, false otherwise
 */
bool semantic_check_types_compatible(Type* expected, Type* actual) {
    if (types_equal(expected, actual)) return true;

    if (type_is_integral(expected) && type_is_integral(actual)) {
        return true;
    }

    if ((expected->kind == TYPE_F64 || expected->kind == TYPE_F32) && 
        (actual->kind == TYPE_F64 || actual->kind == TYPE_F32)) {
        return true;
    }
    
    return false;
}

/**
 * Performs semantic analysis on binary operations.
 * Checks operand types and determines result type.
 * 
 * @param analyzer The semantic analyzer
 * @param expr The binary operation AST node
 * @return The result type of the operation, or NULL on error
 */
static Type* check_binary_op(SemanticAnalyzer* analyzer, AstNode* expr) {
    Type* left_type = check_expression(analyzer, expr->data.binary.left);
    Type* right_type = check_expression(analyzer, expr->data.binary.right);
    
    if (!left_type || !right_type) {
        return NULL;
    }
    
    TokenType op = expr->data.binary.op;
    
    if (op == TOKEN_PLUS || op == TOKEN_MINUS || op == TOKEN_STAR || op == TOKEN_SLASH || op == TOKEN_PERCENT) {
        if (!type_is_numeric(left_type) || !type_is_numeric(right_type)) {
            semantic_error_node(analyzer, expr, "Arithmetic operation requires numeric types");
            return NULL;
        }
        
        if (left_type->kind == TYPE_F64 || right_type->kind == TYPE_F64) {
            return type_create(TYPE_F64);
        }
        if (left_type->kind == TYPE_F32 || right_type->kind == TYPE_F32) {
            return type_create(TYPE_F32);
        }
        
        return type_create(TYPE_I32);
    }
    
    if (op == TOKEN_LT || op == TOKEN_GT || op == TOKEN_LT_EQ || op == TOKEN_GT_EQ) {
        if (!type_is_numeric(left_type) || !type_is_numeric(right_type)) {
            semantic_error_node(analyzer, expr, "Comparison requires numeric types");
            return NULL;
        }
        return type_create(TYPE_BOOL);
    }

    if (op == TOKEN_EQ_EQ || op == TOKEN_NOT_EQ) {
        if (!types_equal(left_type, right_type)) {
            semantic_error_node(analyzer, expr, "Equality comparison requires same types");
            return NULL;
        }
        return type_create(TYPE_BOOL);
    }

    if (op == TOKEN_AND_AND || op == TOKEN_OR_OR) {
        if (left_type->kind != TYPE_BOOL || right_type->kind != TYPE_BOOL) {
            semantic_error_node(analyzer, expr, "Logical operation requires boolean types");
            return NULL;
        }
        return type_create(TYPE_BOOL);
    }

    if (op == TOKEN_AND || op == TOKEN_OR || op == TOKEN_XOR || op == TOKEN_LT_LT || op == TOKEN_GT_GT) {
        if (!type_is_integral(left_type) || !type_is_integral(right_type)) {
            semantic_error_node(analyzer, expr, "Bitwise operation requires integral types");
            return NULL;
        }
        return left_type;  // Use left type for now
    }
    
    semantic_error_node(analyzer, expr, "Unknown binary operator");
    return NULL;
}

/**
 * Performs semantic analysis on unary operations.
 * Handles negation, logical NOT, dereference, and address-of operations.
 * 
 * @param analyzer The semantic analyzer
 * @param expr The unary operation AST node
 * @return The result type of the operation, or NULL on error
 */
static Type* check_unary_op(SemanticAnalyzer* analyzer, AstNode* expr) {
    Type* operand_type = check_expression(analyzer, expr->data.unary.operand);
    if (!operand_type) return NULL;
    
    TokenType op = expr->data.unary.op;
    
    if (op == TOKEN_MINUS) {
        if (!type_is_numeric(operand_type)) {
            semantic_error_node(analyzer, expr, "Negation requires numeric type");
            return NULL;
        }
        return operand_type;
    }
    
    if (op == TOKEN_NOT) {
        if (operand_type->kind != TYPE_BOOL) {
            semantic_error_node(analyzer, expr, "Logical NOT requires boolean type");
            return NULL;
        }
        return type_create(TYPE_BOOL);
    }
    
    if (op == TOKEN_STAR) {
        Type* deref = type_dereference(operand_type);
        if (!deref) {
            semantic_error_node(analyzer, expr, "Cannot dereference non-pointer type");
            return NULL;
        }
        return deref;
    }
    
    if (op == TOKEN_AND) {
        Type* ref_type = type_create(TYPE_REFERENCE);
        ref_type->data.reference.referenced_type = operand_type;
        ref_type->data.reference.is_mutable = expr->data.unary.is_mut_ref;
        return ref_type;
    }
    
    semantic_error_node(analyzer, expr, "Unknown unary operator");
    return NULL;
}

/**
 * Performs semantic analysis on function and method calls.
 * Handles built-in functions, regular functions, and method calls.
 * 
 * @param analyzer The semantic analyzer
 * @param expr The call expression AST node
 * @return The return type of the called function, or NULL on error
 */
static Type* check_call(SemanticAnalyzer* analyzer, AstNode* expr) {
    if (expr->data.call.function->type == AST_FIELD) {
        AstNode* field_expr = expr->data.call.function;
        Type* obj_type = check_expression(analyzer, field_expr->data.field.object);
        if (!obj_type) return NULL;

        if (type_is_reference(obj_type) || type_is_pointer(obj_type)) {
            obj_type = type_dereference(obj_type);
        }
        
        if (obj_type->kind != TYPE_STRUCT) {
            semantic_error_node(analyzer, expr, "Method call on non-struct type");
            return NULL;
        }

        char method_full_name[256];
        snprintf(method_full_name, sizeof(method_full_name), "%s::%s",
                obj_type->data.struct_type.name, field_expr->data.field.field_name);
        
        Symbol* method_sym = symbol_table_lookup_function(analyzer->symbols, method_full_name);
        if (!method_sym) {
            semantic_error_node(analyzer, expr, "Undefined method: %s", field_expr->data.field.field_name);
            return NULL;
        }

        if (expr->data.call.argument_count != method_sym->info.function.param_count - 1) {
            semantic_error_node(analyzer, expr, "Method %s expects %lu arguments, got %lu",
                          field_expr->data.field.field_name,
                          (unsigned long)(method_sym->info.function.param_count - 1),
                          (unsigned long)expr->data.call.argument_count);
            return NULL;
        }

        for (size_t i = 0; i < expr->data.call.argument_count; i++) {
            Type* arg_type = check_expression(analyzer, expr->data.call.arguments[i]);
            if (!arg_type) continue;
            
            Type* param_type = method_sym->info.function.param_types[i + 1]; // Skip self
            if (!semantic_check_types_compatible(param_type, arg_type)) {
                semantic_error_node(analyzer, expr, "Argument %lu type mismatch in method call to %s",
                              (unsigned long)(i + 1), field_expr->data.field.field_name);
            }
        }
        
        return method_sym->type;
    }

    if (expr->data.call.function->type != AST_IDENTIFIER) {
        semantic_error_node(analyzer, expr, "Can only call functions by name");
        return NULL;
    }
    
    const char* func_name = expr->data.call.function->data.identifier.name;

    if (strcmp(func_name, "println") == 0 || strcmp(func_name, "print") == 0) {
        for (size_t i = 0; i < expr->data.call.argument_count; i++) {
            check_expression(analyzer, expr->data.call.arguments[i]);
        }
        return type_create(TYPE_VOID);
    }
    
    if (strcmp(func_name, "sqrt") == 0) {
        if (expr->data.call.argument_count != 1) {
            semantic_error_node(analyzer, expr, "sqrt expects 1 argument");
            return NULL;
        }
        Type* arg_type = check_expression(analyzer, expr->data.call.arguments[0]);
        if (!type_is_numeric(arg_type)) {
            semantic_error_node(analyzer, expr, "sqrt requires numeric argument");
            return NULL;
        }
        return type_create(TYPE_F32);
    }

    Symbol* func_sym = symbol_table_lookup_function(analyzer->symbols, func_name);
    if (!func_sym) {
        if (strstr(func_name, "::")) {
            return type_create(TYPE_STRUCT);
        }
        
        semantic_error_node(analyzer, expr, "Undefined function: %s", func_name);
        return NULL;
    }

    if (expr->data.call.argument_count != func_sym->info.function.param_count) {
        semantic_error_node(analyzer, expr, "Function %s expects %lu arguments, got %lu",
                      func_name, 
                      (unsigned long)func_sym->info.function.param_count,
                      (unsigned long)expr->data.call.argument_count);
        return NULL;
    }

    for (size_t i = 0; i < expr->data.call.argument_count; i++) {
        Type* arg_type = check_expression(analyzer, expr->data.call.arguments[i]);
        if (!arg_type) continue;
        
        Type* param_type = func_sym->info.function.param_types[i];
        if (!semantic_check_types_compatible(param_type, arg_type)) {
            semantic_error_node(analyzer, expr, "Argument %lu type mismatch in call to %s",
                          (unsigned long)(i + 1), func_name);
        }
    }
    
    return func_sym->type;  // Return function's return type
}

/**
 * Performs semantic analysis on array indexing operations.
 * Checks that the indexed expression is an array and index is integral.
 * 
 * @param analyzer The semantic analyzer
 * @param expr The index expression AST node
 * @return The element type of the array, or NULL on error
 */
static Type* check_index(SemanticAnalyzer* analyzer, AstNode* expr) {
    Type* array_type = check_expression(analyzer, expr->data.index.array);
    Type* index_type = check_expression(analyzer, expr->data.index.index);
    
    if (!array_type || !index_type) return NULL;

    if (array_type->kind == TYPE_REFERENCE && 
        array_type->data.reference.referenced_type &&
        array_type->data.reference.referenced_type->kind == TYPE_ARRAY) {
        array_type = array_type->data.reference.referenced_type;
    }
    
    if (array_type->kind != TYPE_ARRAY && array_type->kind != TYPE_POINTER) {
        semantic_error_node(analyzer, expr, "Cannot index non-array or pointer type");
        return NULL;
    }
    
    if (!type_is_integral(index_type)) {
        semantic_error_node(analyzer, expr, "Array index must be integral type");
        return NULL;
    }
    
    return array_type->data.array.element_type;
}

/**
 * Performs semantic analysis on field access operations.
 * Handles struct field access with automatic dereferencing.
 * 
 * @param analyzer The semantic analyzer
 * @param expr The field access AST node
 * @return The type of the accessed field, or NULL on error
 */
static Type* check_field(SemanticAnalyzer* analyzer, AstNode* expr) {
    Type* object_type = check_expression(analyzer, expr->data.field.object);
    if (!object_type) return NULL;

    if (type_is_reference(object_type) || type_is_pointer(object_type)) {
        object_type = type_dereference(object_type);
    }
    
    if (object_type->kind != TYPE_STRUCT) {
        semantic_error_node(analyzer, expr, "Cannot access field of non-struct type");
        return NULL;
    }

    Symbol* struct_sym = symbol_table_lookup_struct(analyzer->symbols, object_type->data.struct_type.name);
    if (!struct_sym) {
        semantic_error_node(analyzer, expr, "Undefined struct: %s", object_type->data.struct_type.name);
        return NULL;
    }

    const char* field_name = expr->data.field.field_name;
    for (size_t i = 0; i < struct_sym->info.struct_def.field_count; i++) {
        Symbol* field = struct_sym->info.struct_def.fields[i];
        if (strcmp(field->name, field_name) == 0) {
            return field->type;
        }
    }
    
    semantic_error_node(analyzer, expr, "Struct %s has no field %s", 
                  object_type->data.struct_type.name, field_name);
    return NULL;
}

/**
 * Performs semantic analysis on assignment operations.
 * Checks mutability and type compatibility.
 * 
 * @param analyzer The semantic analyzer
 * @param expr The assignment expression AST node
 * @return The type of the assignment target, or NULL on error
 */
static Type* check_assignment(SemanticAnalyzer* analyzer, AstNode* expr) {
    Type* target_type = check_expression(analyzer, expr->data.assignment.target);
    Type* value_type = check_expression(analyzer, expr->data.assignment.value);
    
    if (!target_type || !value_type) return NULL;

    AstNode* target = expr->data.assignment.target;
    if (target->type == AST_IDENTIFIER) {
        Symbol* var = symbol_table_lookup(analyzer->symbols, target->data.identifier.name);
        if (var && !var->is_mutable) {
            semantic_error_node(analyzer, expr, "Cannot assign to immutable variable");
            return NULL;
        }
    }

    if (target->type == AST_INDEX) {
      Symbol* var = symbol_table_lookup(analyzer->symbols, target->data.index.array->data.identifier.name);
      if (var && !var->is_mutable) {
        semantic_error_node(analyzer, expr, "Cannot assign to read-only location");
        return NULL;
      }
    }
    
    if (!semantic_check_types_compatible(target_type, value_type)) {
        semantic_error_node(analyzer, expr, "Type mismatch in assignment");
        return NULL;
    }
    
    return target_type;
}

/**
 * Main expression type checker dispatcher.
 * Routes different expression types to their specific analyzers.
 * Caches results in the AST node for efficiency.
 * 
 * @param analyzer The semantic analyzer
 * @param expr The expression AST node
 * @return The type of the expression, or NULL on error
 */
static Type* check_expression(SemanticAnalyzer* analyzer, AstNode* expr) {
    if (!expr) return NULL;

    if (expr->data_type) {
        return expr->data_type;
    }
    
    Type* result_type = NULL;
    
    switch (expr->type) {
        case AST_LITERAL:
            if (expr->data_type) {
                result_type = expr->data_type;
            } else {
                result_type = type_create(TYPE_I32);
            }
            break;
        
        case AST_IDENTIFIER: {
            const char* name = expr->data.identifier.name;

            if (strcmp(name, "self") == 0) {
                const char* current_struct = symbol_table_get_current_struct(analyzer->symbols);
                if (current_struct) {
                    Type* self_type = type_create(TYPE_STRUCT);
                    self_type->data.struct_type.name = string_duplicate(current_struct);
                    result_type = self_type;
                    break;
                }
            }
            
            Symbol* sym = symbol_table_lookup(analyzer->symbols, name);
            if (!sym) {
                semantic_error_node(analyzer, expr, "Undefined variable: %s", name);
                return NULL;
            }
            if (!sym->is_initialized && sym->kind == SYMBOL_VARIABLE) {
                semantic_error_node(analyzer, expr, "Use of uninitialized variable: %s", name);
            }
            result_type = sym->type;
            break;
        }
        
        case AST_BINARY_OP:
            result_type = check_binary_op(analyzer, expr);
            break;
        
        case AST_UNARY_OP:
            result_type = check_unary_op(analyzer, expr);
            break;
        
        case AST_CAST: {
            Type* expr_type = check_expression(analyzer, expr->data.cast.expression);
            if (!expr_type) {
                return NULL;
            }

            result_type = expr->data.cast.target_type;
            break;
        }
        
        case AST_CALL:
            result_type = check_call(analyzer, expr);
            break;
        
        case AST_INDEX:
            result_type = check_index(analyzer, expr);
            break;
        
        case AST_FIELD:
            result_type = check_field(analyzer, expr);
            break;
        
        case AST_ASSIGNMENT:
            result_type = check_assignment(analyzer, expr);
            break;
        
        case AST_ARRAY_LITERAL: {
            if (expr->data.array_literal.element_count == 0) {
                semantic_error_node(analyzer, expr, "Cannot infer type of empty array literal");
                return NULL;
            }

            Type* elem_type = check_expression(analyzer, expr->data.array_literal.elements[0]);
            for (size_t i = 1; i < expr->data.array_literal.element_count; i++) {
                Type* t = check_expression(analyzer, expr->data.array_literal.elements[i]);
                if (!types_equal(elem_type, t)) {
                    semantic_error_node(analyzer, expr->data.array_literal.elements[i], "Array literal elements must have same type");
                    return NULL;
                }
            }
            
            Type* array_type = type_create(TYPE_ARRAY);
            array_type->data.array.element_type = elem_type;
            array_type->data.array.size = expr->data.array_literal.element_count;
            return array_type;
        }
        
        case AST_STRUCT_LITERAL: {
            const char* struct_name = expr->data.struct_literal.struct_name;
            Symbol* struct_sym = symbol_table_lookup_struct(analyzer->symbols, struct_name);
            if (!struct_sym) {
                semantic_error_node(analyzer, expr, "Undefined struct: %s", struct_name);
                return NULL;
            }

            for (size_t i = 0; i < expr->data.struct_literal.field_count; i++) {
                const char* field_name = expr->data.struct_literal.field_names[i];
                Type* value_type = check_expression(analyzer, expr->data.struct_literal.field_values[i]);

                bool found = false;
                for (size_t j = 0; j < struct_sym->info.struct_def.field_count; j++) {
                    Symbol* field = struct_sym->info.struct_def.fields[j];
                    if (strcmp(field->name, field_name) == 0) {
                        found = true;
                        if (!semantic_check_types_compatible(field->type, value_type)) {
                            semantic_error_node(analyzer, expr, "Type mismatch for field %s in struct literal", field_name);
                        }
                        break;
                    }
                }
                
                if (!found) {
                    semantic_error_node(analyzer, expr, "Unknown field %s in struct %s", field_name, struct_name);
                }
            }
            
            result_type = struct_sym->type;
            break;
        }
        
        default:
            semantic_error_node(analyzer, expr, "Unknown expression type");
            return NULL;
    }

    if (result_type) {
        expr->data_type = result_type;
    }
    
    return result_type;
}

/**
 * Public interface for expression type checking.
 * 
 * @param analyzer The semantic analyzer
 * @param expr The expression AST node
 * @return The type of the expression, or NULL on error
 */
Type* semantic_check_expression(SemanticAnalyzer* analyzer, AstNode* expr) {
    return check_expression(analyzer, expr);
}

/**
 * Performs semantic analysis on let statements (variable declarations).
 * Handles type inference, mutability, and symbol table registration.
 * 
 * @param analyzer The semantic analyzer
 * @param stmt The let statement AST node
 */
static void check_let_statement(SemanticAnalyzer* analyzer, AstNode* stmt) {
    const char* var_name = stmt->data.let_stmt.name;
    Type* declared_type = stmt->data.let_stmt.type;
    Type* init_type = NULL;
    
    if (stmt->data.let_stmt.value) {
        init_type = check_expression(analyzer, stmt->data.let_stmt.value);
    }

    if (!declared_type) {
        semantic_error_node(analyzer, stmt, "Variable %s requires explicit type declaration", var_name);
        return;
    }
    
    Type* var_type = declared_type;

    if (init_type && !semantic_check_types_compatible(declared_type, init_type)) {
        semantic_error_node(analyzer, stmt, "Type mismatch in variable declaration");
        return;
    }

    Symbol* var_sym = symbol_table_define(analyzer->symbols, var_name, SYMBOL_VARIABLE,
                                          var_type, stmt->data.let_stmt.is_mutable);
    if (!var_sym) {
        semantic_error_node(analyzer, stmt, "Variable %s already defined in this scope", var_name);
        return;
    }
    
    if (stmt->data.let_stmt.value) {
        var_sym->is_initialized = true;
    }
    
    analyzer->variables_analyzed++;
}

/**
 * Performs semantic analysis on if statements.
 * Checks condition type and analyzes branches in separate scopes.
 * 
 * @param analyzer The semantic analyzer
 * @param stmt The if statement AST node
 */
static void check_if_statement(SemanticAnalyzer* analyzer, AstNode* stmt) {
    Type* cond_type = check_expression(analyzer, stmt->data.if_stmt.condition);
    if (cond_type && cond_type->kind != TYPE_BOOL) {
        semantic_error_node(analyzer, stmt, "If condition must be boolean");
    }
    
    symbol_table_enter_scope(analyzer->symbols, SCOPE_BLOCK);
    check_statement(analyzer, stmt->data.if_stmt.then_branch);
    symbol_table_exit_scope(analyzer->symbols);
    
    if (stmt->data.if_stmt.else_branch) {
        symbol_table_enter_scope(analyzer->symbols, SCOPE_BLOCK);
        check_statement(analyzer, stmt->data.if_stmt.else_branch);
        symbol_table_exit_scope(analyzer->symbols);
    }
}

/**
 * Performs semantic analysis on while loops.
 * Checks condition type and tracks loop nesting for break/continue.
 * 
 * @param analyzer The semantic analyzer
 * @param stmt The while statement AST node
 */
static void check_while_statement(SemanticAnalyzer* analyzer, AstNode* stmt) {
    Type* cond_type = check_expression(analyzer, stmt->data.while_loop.condition);
    if (cond_type && cond_type->kind != TYPE_BOOL) {
        semantic_error_node(analyzer, stmt, "While condition must be boolean");
    }
    
    analyzer->in_loop_count++;
    symbol_table_enter_scope(analyzer->symbols, SCOPE_LOOP);
    check_statement(analyzer, stmt->data.while_loop.body);
    symbol_table_exit_scope(analyzer->symbols);
    analyzer->in_loop_count--;
}

/**
 * Performs semantic analysis on for loops.
 * Checks range types, defines iterator variable, and tracks loop nesting.
 * 
 * @param analyzer The semantic analyzer
 * @param stmt The for loop AST node
 */
static void check_for_statement(SemanticAnalyzer* analyzer, AstNode* stmt) {
    analyzer->in_loop_count++;
    symbol_table_enter_scope(analyzer->symbols, SCOPE_LOOP);

    Type* start_type = check_expression(analyzer, stmt->data.for_loop.start);
    Type* end_type = check_expression(analyzer, stmt->data.for_loop.end);
    
    if (!type_is_integral(start_type) || !type_is_integral(end_type)) {
        semantic_error_node(analyzer, stmt, "For loop range must be integral");
    }

    Symbol* iter = symbol_table_define(analyzer->symbols, stmt->data.for_loop.iterator,
                                       SYMBOL_VARIABLE, type_create(TYPE_I32), false);
    if (iter) {
        iter->is_initialized = true;
    }
    
    check_statement(analyzer, stmt->data.for_loop.body);
    
    symbol_table_exit_scope(analyzer->symbols);
    analyzer->in_loop_count--;
}

/**
 * Performs semantic analysis on return statements.
 * Checks return type compatibility and presence in function context.
 * 
 * @param analyzer The semantic analyzer
 * @param stmt The return statement AST node
 */
static void check_return_statement(SemanticAnalyzer* analyzer, AstNode* stmt) {
    Type* return_type = symbol_table_get_return_type(analyzer->symbols);
    if (!return_type) {
        semantic_error_node(analyzer, stmt, "Return statement outside function");
        return;
    }
    
    if (stmt->data.return_stmt.value) {
        Type* value_type = check_expression(analyzer, stmt->data.return_stmt.value);
        if (!semantic_check_types_compatible(return_type, value_type)) {
            semantic_error_node(analyzer, stmt, "Return type mismatch");
        }
    } else if (return_type->kind != TYPE_VOID) {
        semantic_error_node(analyzer, stmt, "Function expects return value");
    }
}

/**
 * Performs semantic analysis on break statements.
 * Ensures break occurs within a loop context.
 * 
 * @param analyzer The semantic analyzer
 * @param stmt The break statement AST node
 */
static void check_break_statement(SemanticAnalyzer* analyzer, AstNode* stmt) {
    if (analyzer->in_loop_count == 0) {
        semantic_error_node(analyzer, stmt, "Break statement outside loop");
    }
}

/**
 * Performs semantic analysis on continue statements.
 * Ensures continue occurs within a loop context.
 * 
 * @param analyzer The semantic analyzer
 * @param stmt The continue statement AST node
 */
static void check_continue_statement(SemanticAnalyzer* analyzer, AstNode* stmt) {
    if (analyzer->in_loop_count == 0) {
        semantic_error_node(analyzer, stmt, "Continue statement outside loop");
    }
}

/**
 * Performs semantic analysis on code blocks.
 * Creates a new scope and analyzes all contained statements.
 * 
 * @param analyzer The semantic analyzer
 * @param block The block AST node
 */
static void check_block(SemanticAnalyzer* analyzer, AstNode* block) {
    symbol_table_enter_scope(analyzer->symbols, SCOPE_BLOCK);
    
    for (size_t i = 0; i < block->data.block.statement_count; i++) {
        check_statement(analyzer, block->data.block.statements[i]);
    }
    
    if (block->data.block.final_expr) {
        check_expression(analyzer, block->data.block.final_expr);
    }
    
    symbol_table_exit_scope(analyzer->symbols);
}

/**
 * Main statement semantic analysis dispatcher.
 * Routes different statement types to their specific analyzers.
 * 
 * @param analyzer The semantic analyzer
 * @param stmt The statement AST node
 */
static void check_statement(SemanticAnalyzer* analyzer, AstNode* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_LET:
            check_let_statement(analyzer, stmt);
            break;
        
        case AST_IF:
            check_if_statement(analyzer, stmt);
            break;
        
        case AST_WHILE:
            check_while_statement(analyzer, stmt);
            break;
        
        case AST_FOR:
            check_for_statement(analyzer, stmt);
            break;
        
        case AST_LOOP:
            analyzer->in_loop_count++;
            symbol_table_enter_scope(analyzer->symbols, SCOPE_LOOP);
            check_statement(analyzer, stmt->data.loop_stmt.body);
            symbol_table_exit_scope(analyzer->symbols);
            analyzer->in_loop_count--;
            break;
        
        case AST_RETURN:
            check_return_statement(analyzer, stmt);
            break;
        
        case AST_BREAK:
            check_break_statement(analyzer, stmt);
            break;
        
        case AST_CONTINUE:
            check_continue_statement(analyzer, stmt);
            break;
        
        case AST_BLOCK:
            check_block(analyzer, stmt);
            break;
        
        default:
            // Expression statement
            check_expression(analyzer, stmt);
            break;
    }
}

/**
 * Performs semantic analysis on function declarations.
 * Registers function in symbol table and analyzes function body in separate scope.
 * 
 * @param analyzer The semantic analyzer
 * @param func The function AST node
 */
void semantic_check_function(SemanticAnalyzer* analyzer, AstNode* func) {
    const char* func_name = func->data.function.name;

    size_t param_count = func->data.function.param_count;
    Type** param_types = malloc(sizeof(Type*) * param_count);
    char** param_names = malloc(sizeof(char*) * param_count);
    bool* param_mutability = malloc(sizeof(bool) * param_count);
    
    for (size_t i = 0; i < param_count; i++) {
        param_types[i] = func->data.function.params[i].type;
        param_names[i] = string_duplicate(func->data.function.params[i].name);
        param_mutability[i] = false;
    }

    Symbol* func_sym = symbol_create_function(func_name, func->data.function.return_type,
                                              param_count, param_types, param_names, param_mutability);
    func_sym->is_initialized = true;
    
    Symbol* defined = symbol_table_define(analyzer->symbols, func_name, SYMBOL_FUNCTION, 
                                          func->data.function.return_type, false);
    if (!defined) {
        semantic_error_node(analyzer, func, "Function %s already defined", func_name);
        free(func_sym);
        return;
    }

    defined->info.function = func_sym->info.function;
    free(func_sym->name);
    free(func_sym);

    symbol_table_enter_function_scope(analyzer->symbols, func->data.function.return_type);

    for (size_t i = 0; i < param_count; i++) {
        const char* param_name = func->data.function.params[i].name;
        Type* param_type = func->data.function.params[i].type;
        
        if (strcmp(param_name, "self") == 0) {
            const char* current_struct = symbol_table_get_current_struct(analyzer->symbols);
            if (current_struct && param_type && param_type->kind == TYPE_STRUCT) {
                if (strcmp(param_type->data.struct_type.name, current_struct) != 0) {
                    semantic_error_node(analyzer, func, "self parameter type must match implementing struct");
                }
            }
        }
        
        Symbol* param = symbol_table_define(analyzer->symbols, param_name,
                                           SYMBOL_PARAMETER, param_type, false);
        if (param) {
            param->is_initialized = true;
            param->info.param.index = i;
        }
    }

    check_statement(analyzer, func->data.function.body);
    symbol_table_exit_scope(analyzer->symbols);
    analyzer->functions_analyzed++;
}

/**
 * Performs semantic analysis on struct declarations.
 * Creates struct symbol and registers it in the symbol table.
 * 
 * @param analyzer The semantic analyzer
 * @param struct_def The struct definition AST node
 */
void semantic_check_struct(SemanticAnalyzer* analyzer, AstNode* struct_def) {
    const char* struct_name = struct_def->data.struct_def.name;

    size_t field_count = struct_def->data.struct_def.field_count;
    Symbol** fields = malloc(sizeof(Symbol*) * field_count);
    
    for (size_t i = 0; i < field_count; i++) {
        fields[i] = calloc(1, sizeof(Symbol));
        fields[i]->name = string_duplicate(struct_def->data.struct_def.fields[i].name);
        fields[i]->type = struct_def->data.struct_def.fields[i].type;
        fields[i]->kind = SYMBOL_FIELD;
    }

    Symbol* struct_sym = symbol_create_struct(struct_name, field_count, fields);
    
    if (!symbol_table_register_type(analyzer->symbols, struct_name, struct_sym)) {
        semantic_error_node(analyzer, struct_def, "Struct %s already defined", struct_name);
        free(struct_sym);
        return;
    }
    
    analyzer->structs_analyzed++;
}

/**
 * Performs semantic analysis on impl blocks.
 * Sets struct context and analyzes all methods within the impl block.
 * 
 * @param analyzer The semantic analyzer
 * @param impl The impl block AST node
 */
void semantic_check_impl(SemanticAnalyzer* analyzer, AstNode* impl) {
    const char* struct_name = impl->data.impl_block.struct_name;

    Symbol* struct_sym = symbol_table_lookup_struct(analyzer->symbols, struct_name);
    if (!struct_sym) {
        semantic_error_node(analyzer, impl, "Implementing methods for undefined struct: %s", struct_name);
        return;
    }

    for (size_t i = 0; i < impl->data.impl_block.function_count; i++) {
        AstNode* method = impl->data.impl_block.functions[i];

        char method_full_name[256];
        snprintf(method_full_name, sizeof(method_full_name), "%s::%s",
                struct_name, method->data.function.name);

        char* saved_name = string_duplicate(method_full_name);
        char* orig_name = method->data.function.name;
        method->data.function.name = saved_name;

        size_t param_count = method->data.function.param_count;
        Type** param_types = malloc(sizeof(Type*) * param_count);
        char** param_names = malloc(sizeof(char*) * param_count);
        bool* param_mutability = malloc(sizeof(bool) * param_count);
        
        for (size_t j = 0; j < param_count; j++) {
            param_types[j] = method->data.function.params[j].type;
            param_names[j] = string_duplicate(method->data.function.params[j].name);
            param_mutability[j] = false;
        }

        Symbol* method_sym = symbol_create_function(saved_name, method->data.function.return_type,
                                                    param_count, param_types, param_names, param_mutability);
        method_sym->is_initialized = true;

        Symbol* registered = symbol_table_define(analyzer->symbols, saved_name, SYMBOL_FUNCTION,
                                                method->data.function.return_type, false);
        if (registered) {
            registered->info.function = method_sym->info.function;
            analyzer->functions_analyzed++;
        }
        
        free(method_sym->name);
        free(method_sym);

        method->data.function.name = orig_name;
        free(saved_name);
    }
}

/**
 * Main AST node analysis dispatcher.
 * Handles multi-pass analysis for proper forward reference resolution.
 * 
 * @param analyzer The semantic analyzer
 * @param node The AST node to analyze
 */
static void analyze_node(SemanticAnalyzer* analyzer, AstNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            for (size_t i = 0; i < node->data.program.count; i++) {
                if (node->data.program.items[i]->type == AST_STRUCT) {
                    semantic_check_struct(analyzer, node->data.program.items[i]);
                }
            }

            for (size_t i = 0; i < node->data.program.count; i++) {
                if (node->data.program.items[i]->type == AST_IMPL) {
                    semantic_check_impl(analyzer, node->data.program.items[i]);
                }
            }

            for (size_t i = 0; i < node->data.program.count; i++) {
                AstNode* item = node->data.program.items[i];
                if (item->type != AST_STRUCT && item->type != AST_IMPL && item->type != AST_INCLUDE) {
                    analyze_node(analyzer, item);
                }
            }
            break;
        
        case AST_FUNCTION:
            semantic_check_function(analyzer, node);
            break;
        
        case AST_EXTERN_FUNCTION:
            {
                Symbol* func_sym = symbol_table_define(analyzer->symbols, 
                    node->data.extern_function.name,
                    SYMBOL_FUNCTION,
                    node->data.extern_function.return_type,
                    false);
                
                if (func_sym) {
                    func_sym->is_initialized = true;
                    func_sym->info.function.param_count = node->data.extern_function.param_count;
                    func_sym->info.function.param_types = malloc(sizeof(Type*) * node->data.extern_function.param_count);
                    func_sym->info.function.param_names = malloc(sizeof(char*) * node->data.extern_function.param_count);
                    func_sym->info.function.param_mutability = malloc(sizeof(bool) * node->data.extern_function.param_count);
                    
                    for (size_t i = 0; i < node->data.extern_function.param_count; i++) {
                        func_sym->info.function.param_types[i] = node->data.extern_function.params[i].type;
                        func_sym->info.function.param_names[i] = string_duplicate(node->data.extern_function.params[i].name);
                        func_sym->info.function.param_mutability[i] = false;
                    }
                }
            }
            break;
        
        case AST_INCLUDE:
        case AST_STRUCT:
        case AST_IMPL:
            break;
        default:
            check_statement(analyzer, node);
            break;
    }
}

/**
 * Main entry point for semantic analysis.
 * Performs complete semantic analysis of the AST.
 * 
 * @param analyzer The semantic analyzer
 * @param ast The root AST node to analyze
 * @return true if analysis succeeded without errors, false otherwise
 */
bool semantic_analyze(SemanticAnalyzer* analyzer, AstNode* ast) {
    analyze_node(analyzer, ast);
    return analyzer->success;
}