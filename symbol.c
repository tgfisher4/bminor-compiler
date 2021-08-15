#include "symbol.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

struct symbol *symbol_create( symbol_t kind, struct type *type, char *name, bool func_defined){
    struct symbol *s = malloc(sizeof(*s));
    if( !s ){
        puts("Could not allocate memory for symbol. Exiting...\n");
        exit(EXIT_FAILURE);
    }

    s->kind  = kind;
    s->type  = type;
    s->name  = name;
    s->func_defined = func_defined;

    return s;
}

void symbol_print(struct symbol *sym){
    if( !sym ) return;
    switch(sym->kind){
        case SYMBOL_GLOBAL:
            printf("global %s", sym->name);
            break;
        case SYMBOL_LOCAL:
            printf("local %d", sym->which);
            break;
        case SYMBOL_PARAM:
            printf("param %d", sym->which);
            break;
        default:
            puts(":/\n");
            break;
    }
}

void symbol_delete(struct symbol *sym){
    if(!sym)    return;
    // don't free/delete anything but the symbol struct itself. the name and type pointed to should remain bc they're still used in AST
    free(sym);
}

char *symbol_to_location(struct symbol *sym, int num_args){
    if(!sym)    return NULL;

    switch(sym->kind){
        case SYMBOL_GLOBAL:
            return strdup(sym->name);
            break;
        case SYMBOL_PARAM:
        case SYMBOL_LOCAL:
            // allows offsets up to 999
            char *to_return = malloc(11);
            // TODO: instead of allocating num_args spots on the stack, allocate min(6, num_args) spots for args and access any beyond 6 from caller's stack frame
            // whichs start at 0, but our args/locals will start from the second item in stack
            sprintf(to_return, "%d(%rbp)",
                    -8 * (1 + which + (sym->kind == SYMBOL_LOCAL ? num_args : 0)));
            return to_return;
            break;
        default:
            break;
    }
}
