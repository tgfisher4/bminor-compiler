#include <stdio.h>
#include <stdlib.h>
#include "type.h"

bool type_params_equal(struct decl *t_params, struct decl *s_params);

extern int typecheck_errors;

struct type *type_create_full(type_t kind, struct type *subtype, struct expr *arr_sz, struct decl *params, bool is_lvalue, bool is_const_value){
    struct type *t = malloc(sizeof(*t));
    if( !t ) {
        puts("Failed to allocate space for type, exiting...");
        exit(EXIT_FAILURE);
    }

    t->kind     = kind;
    t->subtype  = subtype;
    t->arr_sz   = arr_sz;
    t->params   = params;
    t->is_lvalue   = is_lvalue;
    t->is_const_value = is_const_value;

    return t;
}

struct type * type_copy(struct type *t){
    if (!t) return NULL;
    return type_create_full(t->kind, type_copy(t->subtype), t->arr_sz, t->params, t->is_lvalue, t->is_const_value);
}

struct type *type_create(type_t kind, struct type *subtype, struct expr *arr_sz, struct decl *params){
    return type_create_full(kind, subtype, arr_sz, params, false, false);
}

struct type * type_create_atomic(type_t kind, bool is_lvalue, bool is_const_value){
    return type_create_full(kind, NULL, NULL, NULL, is_lvalue, is_const_value);
}

/*
struct type *type_create_ident(type_t kind, bool is_lvalue, bool is_const_value){
    return type_create_full(TYPE_IDENT, NULL, NULL
}
*/

char *type_t_to_str(type_t kind){ 
    char *type_t_to_str[] = <type_t_to_str_arr_placeholder>;
    return type_t_to_str[kind - <first_type_placeholder>];
}

void type_print(struct type *t){
    if (!t) return;

    //char *kind_to_str[] = {"void", "boolean", "char", "integer", "string", "array", "function"};
    //printf("%s", kind_to_str[t->kind - TYPE_VOID]);
    printf("%s", type_t_to_str(t->kind));
    //type_t_print(t->kind);
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

void type_typecheck(struct type *t, struct decl *context){
    // checks whether a type t is valid in bminor. if not, prints an error message using the context in which this type appears
    if (!t) return;

    if( t->kind == TYPE_FUNCTION ){
        for(struct decl *curr_param = t->params; curr_param; curr_param = curr_param->next){
            type_typecheck(curr_param->type, context);
            if( curr_param->type->kind == TYPE_FUNCTION ){
                printf("[ERROR|typecheck] You attempted to declare a function which takes a function parameter (`");
                decl_print_no_asgn(context);
                printf("`). Sorry, but BMinor does not currently support functions taking function parameters.\n");
                typecheck_errors++;
            }

            if( curr_param->type->kind == TYPE_ARRAY && curr_param->type->arr_sz ){
                printf("[WARNING|typecheck] You attempted to declare a size for array parameter %s to function %s (`", curr_param->ident, context->ident);
                decl_print_no_asgn(context);
                printf("`). This has no effect, but may give the reader of the code the false impression that a typecheck error will result if an array of different size is passed as an argument here. In fact, an array of any size matching the subtype specified will be accepted without error.\n");
            }
        }

        if( t->subtype->kind == TYPE_FUNCTION || t->subtype->kind == TYPE_ARRAY ){
            printf("[ERROR|typecheck] You attempted to declare a function returning a(n) %s (`", type_t_to_str(t->subtype->kind));
            decl_print_no_asgn(context);
            printf("`). Sorry, but BMinor does not currently support functions returning functions or arrays.\n");
            typecheck_errors++;
        }

    }

    // check array conditions
    if( t->kind == TYPE_ARRAY ){
        // check array of functions
        if( t->subtype->kind == TYPE_FUNCTION ){
            printf("[ERROR|typecheck] You attempted to declare an array of functions (`");
            decl_print_no_asgn(context);
            printf("`). Sorry, but BMinor does not currently support arrays of functions.\n");
            typecheck_errors++;
        }
        
        // check that size, if present, is constant and integer
        struct type *arr_sz_type = expr_typecheck(t->arr_sz);
        if( arr_sz_type && arr_sz_type->kind != TYPE_INTEGER ){
            // array size must be integer
            printf("[ERROR|typecheck] You attempted to declare an array with a non-integer size (`");
            decl_print_no_asgn(context);
            printf("`). You must specify a fixed (does not use variables), positive, integer size in an array declaration.\n");
            typecheck_errors++;
        }
        else if( arr_sz_type && !arr_sz_type->is_const_value ){
            // array size must be constant/fixed
            printf("[ERROR|typecheck] You attempted to declare an array with a variable size (`");
            decl_print_no_asgn(context);
            printf("`). Sorry, but BMinor does not currently support variable size arrays. You must specify a fixed (does not use variables), positive, integer size in an array declaration.\n");
            typecheck_errors++;
        }
        else if( arr_sz_type && expr_eval_const_int(t->arr_sz) <= 0 ){
            printf("[ERROR|typecheck] You attempted to declare an array with size %d (`",
                    expr_eval_const_int(t->arr_sz));
            decl_print_no_asgn(context);
            printf("`). You must specify a fixed (does not use variables), positive, integer size in an array declaration.\n");
            typecheck_errors++;
            
        }
        // we'll make sure it's the CORRECT size in the init_value check
        type_delete(arr_sz_type);

        if( t->subtype->kind == TYPE_VOID ){
            printf("[ERROR|typecheck] You attempted to declare an array of void objects (`");
            decl_print_no_asgn(context);
            printf("`). The void type may only be used as the return type for a function which returns nothing. It is meaningless otherwise: an array of void objects makes no sense.\n");
        }
    }

    type_typecheck(t->subtype, context);
}

// I forget where I was going with this...
bool type_t_equals(type_t t, type_t s){ return t == s; }

bool type_params_equal(struct decl *t_params, struct decl *s_params){
    return  (!t_params || !s_params)
            ? t_params == s_params
            : type_equals(t_params->type, s_params->type) && type_params_equal(t_params->next, s_params->next);
}

bool type_equals(struct type *t, struct type *s){
    if( !t || !s )    return  t == s;
    if( t->kind != s->kind ) return false;

    if( t->kind == TYPE_FUNCTION && !type_params_equal(t->params, s->params) )  return false;

    if( t->kind == TYPE_ARRAY && t->arr_sz && s->arr_sz ){
        // don't complain if either size is not declared
        struct type *t_sz_type = expr_typecheck(t->arr_sz);
        struct type *s_sz_type = expr_typecheck(s->arr_sz);
        int t_sz = 0;
        int s_sz = 0;
        if( t_sz_type->is_const_value && t_sz_type->kind == TYPE_INTEGER
         && s_sz_type->is_const_value && s_sz_type->kind == TYPE_INTEGER ){
            t_sz = expr_eval_const_int(t->arr_sz);
            s_sz = expr_eval_const_int(s->arr_sz);
        }
        type_delete(t_sz_type);
        type_delete(s_sz_type);
        // don't complain if either size is variable size
        if( t_sz != s_sz ) return false;
    }
    return type_equals(t->subtype, s->subtype);    
}

void type_delete(struct type *t){
    if (!t) return;
    type_delete(t->subtype);
    free(t);
}
