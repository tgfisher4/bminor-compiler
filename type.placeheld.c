#include <stdio.h>
#include <stdlib.h>
#include "type.h"

struct type *type_create(type_t kind, struct type *subtype, struct expr *arr_sz, struct decl *params){
    struct type *t = malloc(sizeof(*t));
    if( !t ) {
        puts("Failed to allocate space for type, exiting...");
        exit(EXIT_FAILURE);
    }

    t->kind     = kind;
    t->subtype  = subtype;
    t->arr_sz   = arr_sz;
    t->params   = params;

    return t;
}

void type_print(struct type *t){
    if (!t) return;

    //char *kind_to_str[] = {"void", "boolean", "char", "integer", "string", "array", "function"};
    //printf("%s", kind_to_str[t->kind - TYPE_VOID]);
    char *type_t_to_str[] = <type_t_to_str_arr_placeholder>;
    printf("%s", type_t_to_str[t->kind - <first_type_placeholder>]);
    switch(t->kind){
        case TYPE_ARRAY:
            fputs(" [", stdout);
            expr_print(t->arr_sz);
            fputs("] ", stdout);
            type_print(t->subtype);
            break;
        case TYPE_FUNCTION:
            fputs(" ", stdout);
            type_print(t->subtype);
            fputs(" (", stdout);
            decl_print_list(t->params, 0, "", ", ");
            fputs(")", stdout);
            break;
        default:
            break;
    }
}
