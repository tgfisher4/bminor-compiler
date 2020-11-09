#include <stdio.h>
#include <stdlib.h>
#include "type.h"

struct type *type_create_full(type_t kind, struct type *subtype, struct expr *arr_sz, struct decl *params, bool lvalue, bool const_value){
    struct type *t = malloc(sizeof(*t));
    if( !t ) {
        puts("Failed to allocate space for type, exiting...");
        exit(EXIT_FAILURE);
    }

    t->kind     = kind;
    t->subtype  = subtype;
    t->arr_sz   = arr_sz;
    t->params   = params;
    t->lvalue   = lvalue;
    t->const_value = const_value;

    return t;
}

struct type * type_copy(struct type *t){
    return type_create_full(t->kind, t->subtype, t->arr_sz, t->params, t->lvalue, t->const_value);
}

struct type *type_create(type_t kind, struct type *subtype, struct expr *arr_sz, struct decl *params){
    return type_create_full(kind, subtype, arr_sz, params, false, false);
}

struct type * type_create_atomic(type_t kind, bool lvalue, bool const_value){
    return type_create_full(kind, NULL, NULL, NULL, lvalue, const_value);
}

/*
struct type *type_create_ident(type_t kind, bool lvalue, bool const_value){
    return type_create_full(TYPE_IDENT, NULL, NULL
}
*/

char *type_t_to_str(type_t kind){ 
    char *type_t_to_str[] = <type_t_to_str_arr_placeholder>;
    return type_t_to_str[t->kind - <first_type_placeholder>];
}

void type_print(struct type *t){
    if (!t) return;

    //char *kind_to_str[] = {"void", "boolean", "char", "integer", "string", "array", "function"};
    //printf("%s", kind_to_str[t->kind - TYPE_VOID]);
    printf("%s", type_t_to_str(t_);
    type_t_print(t->kind);
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

bool type_equals(struct type *t, struct type *s){
    //if( !t && !s )  return true;
    //if( t || s )    return false;
    // base case: if one is NULL, they better both be
    if( !t || !s )    return  t == s;
    if( t->kind != s->kind ) return false;

    if( t->kind == TYPE_FUNCTION)   // check params
    return type_equals(t->subtype, s->subtype);
    
    /*
    switch( t->kind ){
        case TYPE_FUNCTION:
            // check params
        case TYPE_ARRAY:
            return type_equals(t->subtype, s->subtype);
    }
    */
}
