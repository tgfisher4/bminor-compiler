#include "decl.h"
#include "scope.h"
#include <stdio.h>
#include <stdlib.h>
//#include <stdbool.h>

extern void indent(int indents);

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
    scope_exit(inner_sc);

    err_count += decl_resolve(d->next, sc, am_param, verbose);
    return err_count;
}
