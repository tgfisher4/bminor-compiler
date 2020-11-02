#include "scope.h"
#include <stdio.h>
#include <stdlib.h>

struct scope *scope_create(struct scope *next){
    struct scope *sc = malloc(sizeof(*sc));
    if( !sc ){
        puts("[ERROR] Failed to allocate memory for struct scope. Exiting...");
        exit(EXIT_FAILURE);
    }
    // allocated memory that needs to be freed later
    sc->table = hash_table_create(0,0);
    sc->next = next;
    sc->locals = 0;
    sc->params = 0;
    return sc;
}

void scope_delete(struct scope *sc){
    // hash table is only member needing to be freed
    hash_table_delete(sc->table);
    free(sc);
}

struct symbol *scope_bind(struct scope *sc, const char *name, struct symbol *sym){
    // on success, returns symbol to bind to AST: on failure, returns NULL
    if( !hash_table_insert(sc->table, name, sym) ){
        struct symbol *existing_sym = scope_lookup(sc, name, true);
        // must both be functions with at least one undefined to support multiple declarations (unlimited prototypes, at most one definition, cannot declare both as prototype and non-function type)
        // this allows the peculiar function protype behavior in which a function may be declared but not defined multiple times (and may even be declared again after being defined)
        if( existing_sym->type->kind != TYPE_FUNCTION
            || sym->type->kind != TYPE_FUNCTION
            || (existing_sym->func_defined && sym->func_defined))
            return NULL;

        // crucially, do not overwrite existing_sym's type info. this will allow the type checker to catch whether the type matches the declaration (i.e. whether function was redeclared with a conflicting type)
        existing_sym->func_defined = existing_sym->func_defined || sym->func_defined;
        // free sym if not returning it. don't want to free its members, which are still in AST
        symbol_delete(sym);
        return existing_sym;
    }
    // else, set which
    switch(sym->kind){
        case SYMBOL_GLOBAL:
            sym->which = 0;
            break;
        case SYMBOL_LOCAL:
            sym->which = sc->locals++;
            break;
        case SYMBOL_PARAM:
            sym->which = sc->params++;
            break;
        default:
            puts("Uh-oh. Here come a flock o' Wah-Wahs");
            break;
    }
    return sym;
}

struct symbol *scope_lookup(struct scope *sc, const char *name, bool only_curr){
    if( !sc ) return NULL;

    struct symbol *result =  hash_table_lookup(sc->table, name);
    return  result
            ? result
            :   only_curr
                ? NULL
                : scope_lookup(sc->next, name, false);
}

struct scope *scope_enter(struct scope *sc){
    struct scope *head = scope_create(sc);
    return head;
}

struct scope *scope_exit(struct scope *sc){
    struct scope *to_return = sc->next;
    scope_delete(sc);
    return to_return;
}

bool scope_is_global(struct scope *sc){
    return sc->next == NULL;
}

