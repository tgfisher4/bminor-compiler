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

void decl_typecheck( struct decl *d){
    if( !d ) return;

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
    type_delete(init_value_type);

    // typecheck function body
    stmt_list_typecheck(d->func_body, d);
}

void decl_list_typecheck(struct decl *d){
    if (!d) return;
    decl_typecheck(d);
    decl_list_typecheck(d->next);
}

int decl_list_length( struct decl *d ){
    if (!d) return 0;
    return 1 + decl_list_length(d->next);
}

void decl_code_gen( struct decl *d, FILE *output, bool is_global ){
    if (!d) return;
    if( is_global ){
        switch( d->type->kind ){
            case TYPE_INTEGER:
                fprintf(output, "%s:\n.quad %s\n", d->ident);
                break;
            case TYPE_STRING:
                fprintf(output, "%s:\n.string %s\n", d->init_value->expr_data->str_data);
                break;
            case TYPE_FUNCTION:
                fprintf(output, ".globl %s\n%s:\n", d->ident, d->ident);
                /* preamble */
                // new stack frame
                fprintf(output, "\tPUSHQ    %rbp\n"
                                "\tMOVQ     %rsp, %rbp\n");
                // push argument registers
                char *arg_regs[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
                for( int i = 0; i < 6; i++) fprintf(output, "\tPUSHQ    \%%s\n", arg_regs[i]);
                // allocate locals
                fprintf(output, "\tSUBQ     $%d", d->func_num_locals);
                // push callee saved registers
                char *callee_saved[] = { "rbx", "r12", "r13" ,"r14", "r15" };
                for( int i = 0; i < 6; i++) fprintf(output, "\tPUSHQ    \%%s\n", calee_saved[i]);

                /* func body */

                /* postamble */
                // pop callee saved registers
                for( int i = 5; i >= 0; i--) fprintf(output, "\tPOPQ    \%%s", calee_saved[i]);
                // destroy stack frame
                fprintf(output, "\tMOVQ     %rbp, %rsp\n"
                                "\tPOPQ     %rbp\n"
                                "\tRET\n");
                break;
            default:
                break;
        }
}
