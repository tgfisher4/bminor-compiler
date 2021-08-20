#include "decl.h"
#include "scope.h"
#include <stdio.h>
#include <stdlib.h>
//#include <stdbool.h>

extern void indent(int indents);
extern int typecheck_errors;

struct decl * decl_create(char *ident, struct type *type, struct expr *init_value, struct stmt *func_body){
    struct decl *d = malloc(sizeof(*d));
    if(!d) {
        puts("[ERROR|FATAL] Could not allocate decl memory, exiting...");
        exit(EXIT_FAILURE);
    }

    d->ident         = ident;
    d->type          = type;
    d->init_value    = init_value;
    d->func_body     = func_body;
    d->next          = NULL;
    d->symbol        = NULL;
    d->num_locals    = 0;

    return d;
}

void decl_print(struct decl *d, int indents, char* term){
    if (!d) return;

    indent(indents);
    printf("%s: ", d->ident);
    type_print(d->type);

    if (d->init_value){
        fputs(" = ", stdout);
        expr_print(d->init_value);
    }

    if (d->func_body){
        fputs(" = {\n", stdout);
        stmt_print_list(d->func_body, indents + 1, "\n");
        indent(indents);
        fputs("\n}", stdout);
    } else fputs(term, stdout);
}

void decl_print_list(struct decl *d, int indents, char* term, char *delim){
    if (!d) return;
    decl_print(d, indents, term);
    if (d->next) fputs(delim, stdout);
    decl_print_list(d->next, indents, term, delim);
}

void decl_print_no_asgn(struct decl *d){
    if (!d) return;
    printf("%s: ", d->ident);
    type_print(d->type);
}

int decl_resolve(struct decl *d, struct scope *sc, bool am_param, bool verbose){
    if( !d ) return 0;

    int err_count = 0;
    // be sure to resolve RHS before LHS: prevent `x: integer = x;`
    err_count += expr_resolve(d->init_value, sc, verbose);
    // possibly resolve array size
    err_count += expr_resolve(d->type->arr_sz, sc, verbose);

    struct symbol *ident_sym = symbol_create(   scope_is_global(sc)
                                                ? SYMBOL_GLOBAL
                                                : (am_param
                                                   ? SYMBOL_PARAM
                                                   : SYMBOL_LOCAL),
                                                d->type, d->ident,
                                                // if transition to all function bodies are stmt blocks, check d->func_body->body instead
                                                d->type->kind == TYPE_FUNCTION && d->func_body);
    // scope_bind takes care of deleting ident_sym if necessary. this is useful because it would require extra effort on our part to know when to delete ident_sym, because the symbol could be ident_sym or a symbol already in the hash table: it is not as simple as the bind succeeding or failing
    d->symbol = scope_bind(sc, d->ident, ident_sym);
    if( !d->symbol ){
        // possible future idea:
        //  - return error symbols from from scope_bind
        //      - global symbol with non-zero which
        //  - could also emit error message IN scope_bind
        //      - not crazy about this
        printf("[ERROR|resolve] Non-prototype variable %s has either a redeclaration or function body redefinition\n", d->ident);
        err_count++;
    }
    else if(verbose){
        printf("Variable %s declared as ", d->ident);
        symbol_print(d->symbol);
        puts("");
    };

    // create new scope for declaring a function
    // resolve function name before body to allow recursion
    struct scope *inner_sc   = scope_enter(sc);
    // resolve (bind) function parameters
    // assuming every decl has a type which should be true from AST construction
    err_count += decl_resolve(d->type->params, inner_sc, true, verbose);
    // resolve function body
    err_count += stmt_resolve(d->func_body, inner_sc, verbose);
    d->num_locals = inner_sc->locals + inner_sc->nested_locals;
    scope_exit(inner_sc);

    err_count += decl_resolve(d->next, sc, am_param, verbose);
    return err_count;
}

void decl_typecheck( struct decl *d, bool is_global ){
    if( !d ) return;

    // TODO: ensure init_values for globals are const
    // make sure we have valid type
    type_typecheck(d->type, d);

    if( !type_equals(d->symbol->type, d->type) ){
        // must be result of redeclaring mismatched prototype
        printf("[ERROR|typecheck] You attempted to re-declare function %s as type ", d->ident);
        type_print(d->type);
        printf(", but earlier you declared it as type ");
        type_print(d->symbol->type);
        printf(".\n");
        typecheck_errors++;
    }

    if( d->type->kind == TYPE_VOID ){
        printf("[ERROR|typecheck] You attempted to declare a variable of type void (`");
                decl_print_no_asgn(d);
                printf("`). The void type may only be used as the return type for a function which returns nothing. It is meaningless otherwise: a void variable makes no sense.\n");
    }

    // array size is given or inferable
    if( d->type->kind == TYPE_ARRAY ){ 
        // check size given
        // allow inference of size from initializer
        if( !d->type->arr_sz && !d->init_value ){
            printf("[ERROR|typecheck] You attempted to declare an array without a size or initializer (`");
            decl_print(d, 0, "");
            printf("`). You may only do this for an array as a function parameter. In an array declaration, you must specifiy a fixed (does not use variables), positive, integer size, or give an array initializer.\n");
            typecheck_errors++;
        }
    }

    // check init_value type matches variable type
    struct type *init_value_type = expr_typecheck(d->init_value);
    if( d->init_value && !type_equals(d->type, init_value_type) ){
        // emit type mismatch error
        printf("[ERROR|typecheck] You attempted to assign a(n) ");
        type_print(d->type);
        printf(" variable (`%s`) to a(n) ", d->ident);
        expr_print_type_and_expr(init_value_type, d->init_value);
        printf(" value. A variable may only store values of its type. Note that an array size is part of its type, and types must match exactly.\n");
        typecheck_errors++;
    }

    // Check that a global's initializer is const
    if( is_global && !init_value_type->is_const ){
        printf("[ERROR|typecheck] You attempted to initialize a global variable (");
        decl_print_no_asgn(d, 0, "");
        printf(") with a non-constant expression (");
        expr_print(d->init_value);
        printf("). A global variable may only by initialized with a constant expression. However, you are free to assign non-constant values to global variables using the assignment operator in a function body.\n");
        typecheck_errors++;
    }
    type_delete(init_value_type);

    // typecheck function body
    stmt_list_typecheck(d->func_body, d);
}

void decl_list_typecheck(struct decl *d, bool is_global){
    if (!d) return;
    decl_typecheck(d, is_global);
    decl_list_typecheck(d->next, is_global);
}

int decl_list_length( struct decl *d ){
    if (!d) return 0;
    return 1 + decl_list_length(d->next);
}

void decl_local_code_gen( struct decl *d, FILE *output, bool is_global ){
    if (!d) return;
    // A declaration without an initializer is used by earlier stages to
    // determine local storage space needed, but requires no effort here.
    if( !d->init_value ) return;

    // Otherwise, this is just an assignment expression
    struct expr *assignment = expr_create_oper(
        EXPR_ASGN,
        expr_create_identifier(d->ident),
        expr_copy(d->init_value)
    );
    expr_code_gen(output, assignment);

    // Clean-up: copying d->init_value allows us to expr_delete the whole assignment
    scratch_free(assignment->reg);
    expr_delete(assignment);
}

void decl_list_code_gen(FILE *output, struct decl *d, bool is_global){
    if( !d ) return;
    decl_code_gen(output, d, is_global);
    decl_list_code_gen(output, d->next, is_global);
}

void decl_global_list_code_gen(FILE *output, struct decl *d){
    fprintf(output, "    .globl %s\n", d->ident);

    if( e->type->kind != TYPE_FUNCTION ){ // Non-function
        struct expr *evald_init_value = expr_eval_const(d->init_value);
        declare_global(output, evald_init_value, d->ident);
        expr_delete(evald_init_value);
    } else { // Function
        fprintf(output, "%s:\n", d->ident);
        /* preamble */
        // Create new stack frame
        fprintf(output, "    PUSHQ    %rbp\n"
                        "    MOVQ     %rsp, %rbp\n");
        // Push argument registers onto stack
        for( int i = 0; func_arg_reg_name(i); i++)
            fprintf(output, "    PUSHQ  %r%s\n",
                    func_arg_reg_name(i));
        // Allocate locals
        fprintf(output, "    SUBQ     $%d",
                8 * d->func_num_locals);
        // Save callee-saved registers
        for( int i = 0; i < num_callee_saved_regs; i++){
            fprintf(output, "    PUSHQ    %r%s\n", callee_saved_regs[i]);
        }

        /* func body */
        stmt_code_gen(d->func_body, d->ident);

        /* postamble */
        // Return statement will jump to this label, but needs function name
        fprintf(output, "%s_postamble:\n", d->ident);
        // Restore callee-saved registers (pop in opposite order bc LIFO)
        for( int i = num_callee_caved_regs - 1; i >= 0; i--){
            fprintf(output, "    POPQ    %r%s", calee_saved_regs[i]);
        }

        // Destroy stack frame
        fprintf(output, "    MOVQ     %rbp, %rsp\n"
                        "    POPQ     %rbp\n"
                        "    RET\n");
    }
    decl_global_list_codegen(d->next);
}

void declare_global(FILE *output, struct expr *const_val, char *label){
    // Know that const_val is const: only literals
    int data;
    switch( e->kind ){
        case EXPR_BOOL_LIT:
            data = (int) const_val->bool_data;
        case EXPR_CHAR_LIT:
            data = (int) const_val->char_data;
        case EXPR_INT_LIT:
            data = (int) const_val->int_data;
            fprintf(output, "%s:\n"
                            "    .quad %d\n",
                    label,
                    data);
            break;
        case EXPR_STR_LIT:
            if( !const_val ){
                // If no initial value, set as null pointer.
                fprintf(output, "%s:\n"
                                "    .quad  0\n",
                        label);
                break;
            }
            char *string_label = label_create();
            fprintf(output, "    .section .rodata\n" // Place literal in rodata sxn
                            "%s:\n"
                            "    .string  \"%s\"\n"
                            "    .data\n" // Place actual variable in data sxn
                            "%s:\n"
                            "    .quad  %s\n"
                    string_label,
                    const_value->data->str_data,
                    label,
                    string_label);
            free(string_label);
            break;
        case EXPR_ARR_LIT:
            // Emit array definition, then, if needed, define elements
            struct expr *head = const_val->data->arr_elements;
            // Base Case: immediate types (int/char/bool)
            //   - data stored is just the element value
            if( head->kind == EXPR_INT_LIT
                || head->kind == EXPR_CHAR_LIT
                || head->kind == EXPR_BOOL_LIT ){
                fprintf(output, "%s:\n", label);
                for( struct expr *ele = head; ele; ele = ele->next ){
                    fprintf(output, "    .quad  %d\n",
                            // Wasn't sure if pasting char/bool data into a union field
                            // and then reading via a integer union field was the same
                            // as casting, so went with the cast to be sure.
                            (int)
                            ele->kind == EXPR_INT_LIT
                            ? ele->data->int_data
                            : ele->kind == EXPR_CHAR_LIT
                              ? ele->data->char_data
                              : ele->data->bool_data
                            );
                }
                break;
            }

            // Recursive Case: indirect types (arr/string)
            //   - each element's data will be another label (a pointer)
            // Determine array length
            int arr_len = 0;
            for(struct expr *curr = head; curr; curr = curr->next)
                arr_len += 1;
            char **ele_labels = malloc(sizeof(char *) * arr_sz);

            // Declare array
            for( int i = 0; i < arr_len; i++ ){
                ele_labels[i] = label_create();
                fprintf(output, "    .quad  %s\n", ele_labels[i]);
            }

            // Declare array elements
            struct expr *curr = head;
            for( int i = 0; i < arr_sz; i++, curr = curr->next ){
                declare_global(output, curr, ele_labels[i]);
            }

            // Clean up
            for( int i = 0; i < arr_sz; i++ ){
                free(ele_labels[i]);
            }
            free(ele_labels);
            break;
        default:
            fprintf("Unexpected expr kind in declare_global: %d. Aborting...\n", d->type->kind);
            abort();
            break;
    }
}

