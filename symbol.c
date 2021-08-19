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

char *symbol_to_location(struct symbol *sym, int reg_offset){
    if(!sym)    return NULL;

    switch(sym->kind){
        case SYMBOL_GLOBAL:
            return strdup(sym->name);
            break;
        case SYMBOL_PARAM:
        case SYMBOL_LOCAL:
            // Use the sym->stack_idx we set up in scope_bind.
            if( sym->type != TYPE_ARRAY ){
                // 11 bytes allow offsets from -999 to 9999:
                // len(X999(%rbp)\0) = 11.
                char *location = malloc(11);
                // TODO: err-check malloc
                sprintf(location, "%d(%rbp)",
                        -8 * sym->stack_idx);
                return location;
            } else {
                // 20 bytes allow offsets from -999 to 9999:
                // len(X999(%rbp, %rXX, 8)\0) = 20.
                char * location = malloc(20);
                // TODO: err-check malloc
                sprintf(location, "%d(%rbp, %r%s, 8)",
                        -8 * sym->stack_idx, scratch_name(reg_offset));
                return location;
            }
            break;
        default:
            printf("Unexpected symbol kind: %d. Aborting...\n", sym->kind);
            abort();
            break;
    }
}
