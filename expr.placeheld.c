#include "expr.h"
#include "scope.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>

extern int typecheck_errors;

/* internal helpers */
char *oper_to_str(expr_t t);
int oper_precedence(expr_t t);
void expr_print_subexpr(struct expr *e, expr_t parent_oper, bool right_oper);
void descape_and_print_str_lit(const char *s);
void descape_and_print_char_lit(char c);
char *descape_char(char c, char delim);

type_t expr_t_literal_to_type_t( expr_t e_t );
bool expr_is_literal_atom( expr_t e_t );
bool is_vowel(char c);
void expr_typecheck_func_args( struct expr *args, struct decl *params, char *func_name );
void print_symm_binary_type_error(expr_t oper, char *expected_type, struct type *left_type, struct expr *left_expr, struct type *right_type, struct expr *right_expr);
void print_asymm_binary_type_error(expr_t oper, char *expected_left_type, struct type *left_type, struct expr *left_expr, char *expected_right_type, struct type *right_type, struct expr *right_expr);
void print_unary_type_error(expr_t oper, bool is_right_unary, char *expected_type, struct type *arg_type, struct expr *arg_expr);
void print_binary_type_error(expr_t oper, char *preamble, struct type *left_type, struct expr *left_expr, struct type *right_type, struct expr *right_expr);


struct expr * expr_create(expr_t expr_type, union expr_data *data){
    struct expr *e = malloc(sizeof(*e));
    if (!e){
        puts("[ERROR|internal] Could not allocate expr memory, exiting...");
        exit(EXIT_FAILURE);
    }

    e->kind = expr_type;
    // this seemed to me a good union application since the data values are mutually exclusive
    e->data = data;
    e->next = NULL;
    e->symbol = NULL;

    return e;
}

struct expr * expr_create_oper( expr_t expr_type, struct expr *left_arg, struct expr* right_arg ){
    /* */
    union expr_data *d = malloc(sizeof(*d));
    if (!d) return NULL;
    // uniform interface for unary operators: pass as NULL whichever operand is not used
    // fill in with empty placeholders on either side so that we have two operands no matter the operator
    if (!left_arg)  left_arg  = expr_create_empty();
    if (!right_arg) right_arg = expr_create_empty();
    left_arg->next = right_arg;
    d->operator_args = left_arg;
    return expr_create(expr_type, d);
}

struct expr * expr_create_identifier( const char *ident ){
    union expr_data *d = malloc(sizeof(*d));
    d->ident_name = ident;
    return expr_create(EXPR_IDENT, d);
}

struct expr * expr_create_integer_literal( int i ){ 
    union expr_data *d = malloc(sizeof(*d));
    d->int_data = i;
    return expr_create(EXPR_INT_LIT, d);
}

struct expr * expr_create_boolean_literal( bool b ){
    union expr_data *d = malloc(sizeof(*d));
    d->bool_data = b;
    return expr_create(EXPR_BOOL_LIT, d);
}

struct expr * expr_create_char_literal( char c ){
    union expr_data *d = malloc(sizeof(*d));
    d->char_data = c;
    return expr_create(EXPR_CHAR_LIT, d);
}

struct expr *expr_create_string_literal( const char *str ){
    union expr_data *d = malloc(sizeof(*d));
    d->str_data = str;
    return expr_create(EXPR_STR_LIT, d);
}

struct expr *expr_create_array_literal(struct expr *expr_list){
    union expr_data *d = malloc(sizeof(*d));
    d->arr_elements = expr_list;
    return expr_create(EXPR_ARR_LIT, d);
}

struct expr *expr_create_function_call(struct expr *function, struct expr *arg_list){ 
    union expr_data *d = malloc(sizeof(*d));
    function->next = arg_list;
    d->func_and_args = function;
    return expr_create(EXPR_FUNC_CALL, d);
}

struct expr *expr_create_array_access(struct expr *array, struct expr *index){
    union expr_data *d = malloc(sizeof(*d));
    array->next = index;
    d->operator_args = array;
    return expr_create(EXPR_ARR_ACC, d);
}

struct expr *expr_create_empty(){
    return expr_create(EXPR_EMPTY, NULL);
}

int expr_resolve(struct expr *e, struct scope *sc, bool verbose){
    if( !e ) return 0;

    int err_count = 0;
    if( e->kind == EXPR_IDENT ){
        e->symbol = scope_lookup(sc, e->data->ident_name, false);
        if ( !e->symbol ){
            printf("[ERROR|resolve] Variable %s used before declaration\n", e->data->ident_name);
            err_count++;
        }
        else if(verbose){
            printf("Variable %s resolved to ", e->data->ident_name);
            symbol_print(e->symbol);
            puts("");
        }
    }
    // if we have arguments, resolve them: operator_args is arbitrary choice here
    //  - just need to interpret data as expr pointer
    // my choice to use a union for e->data makes this clunky

    if( !(e->kind == EXPR_EMPTY    || e->kind == EXPR_IDENT
       || expr_is_literal_atom(e->kind)) )
        err_count += expr_resolve(e->data->operator_args, sc, verbose);

    err_count += expr_resolve(e->next, sc, verbose);
    return err_count;
}

bool expr_is_literal_atom(expr_t e_t){
    return (e_t == EXPR_INT_LIT  || e_t == EXPR_STR_LIT ||
             e_t == EXPR_CHAR_LIT || e_t == EXPR_BOOL_LIT);
}

type_t expr_t_literal_to_type_t(expr_t e_t){
    //if( !is_literal(e_t) ) return TYPE_VOID;
    // can use placeholder later
    type_t literal_types[] = {TYPE_INTEGER, TYPE_STRING, TYPE_CHAR, TYPE_BOOLEAN};
    return literal_types[e_t - EXPR_INT_LIT];
}

struct type *expr_typecheck(struct expr *e){
    if( !e || e->kind == EXPR_EMPTY ) return NULL;

    if( e->kind == EXPR_IDENT ){
        // copy type found in relevant symbol
        //  - this allows me to unconditionally delete types after grabbing them from subexpressions
        struct type *to_return = type_copy(e->symbol->type);
        // double check that these are set as we expect
        to_return->is_lvalue = !(to_return->kind == TYPE_ARRAY || to_return ->kind == TYPE_FUNCTION);
        to_return->is_const_value = false;
        return to_return;
    }

    if( e->kind == EXPR_FUNC_CALL){
        // TODO: check args/params
        if( e->data->func_and_args->symbol->type->kind != TYPE_FUNCTION ){
            // emit error: cannot call nonfunction ident
            printf("[ERROR|typecheck] You attempted to call a non-function variable %s as if it were a function (`", e->data->func_and_args->symbol->name);
            expr_print(e);
            printf("`).\n");
            typecheck_errors++;
        }
        expr_typecheck_func_args(e->data->func_and_args->next, e->data->func_and_args->symbol->type->params, e->data->func_and_args->symbol->name);
        // return return type
        struct type *to_return = type_copy(e->data->func_and_args->symbol->type->subtype);
        if( !to_return ) to_return = type_create_atomic(TYPE_VOID, false, false);
        to_return->is_lvalue       = false;
        to_return->is_const_value  = false;
        return to_return;
    }

    if( expr_is_literal_atom(e->kind) ){
        // type being created from a literal: no type to copy
        return type_create_atomic(expr_t_literal_to_type_t(e->kind), false, true);
    }

    if( e->kind == EXPR_ARR_LIT ){
        struct type *expected_type = expr_typecheck(e->data->arr_elements);
        int sz = 1;
        for( struct expr *curr = e->data->arr_elements->next; curr; curr = curr->next, sz++ ){
            struct type *curr_type = expr_typecheck(curr);
            if( !type_equals(curr_type, expected_type) ){
                // emit arr multiple types error
                printf("[ERROR|typecheck] You attempted to create an array literal with elements of multiple types, including ");
                expr_print_type_and_expr(expected_type, e->data->arr_elements);
                printf(" and ");
                expr_print_type_and_expr(curr_type, curr);
                printf(". You may only have elements of a single type in your array.\n");
                typecheck_errors++;
            }
            type_delete(curr_type);
        }
        struct type* to_return = type_create(TYPE_ARRAY, expected_type,
                                    expr_create_integer_literal(sz), NULL);
        //type_delete(expected_type);
        return to_return;
    }

    // now, we've checked all other cases, we are left with an operator
    //  - we can interpret data field as expr *
    struct type *left_arg_type = expr_typecheck(e->data->operator_args);
    struct type *right_arg_type = expr_typecheck(e->data->operator_args->next);

    if( (e->kind == EXPR_ASGN || e->kind == EXPR_POST_INC || e->kind == EXPR_POST_DEC)
     && !left_arg_type->is_lvalue){
        // TODO emit is_lvalue error message
        printf("[ERROR|typecheck] You attempted to assign to unassignable object ");
        expr_print(e->data->operator_args);
        printf(" (`");
        expr_print(e);
        printf("`). You may only assign to variables and array elements.\n");
        typecheck_errors++;
    }

    // check operand types
    switch(e->kind){
        case EXPR_EQ:
        case EXPR_NOT_EQ:
            // check not compound type or void and fall through
            if( left_arg_type->kind == TYPE_VOID || !type_equals(left_arg_type, right_arg_type) || left_arg_type->subtype ){
                // TODO emit type error
                print_symm_binary_type_error(e->kind, "same, non-void, atomic (non-array, non-function)", left_arg_type, e->data->operator_args, right_arg_type, e->data->operator_args->next);
                typecheck_errors++;
            }
        case EXPR_ASGN:
            if( !type_equals(left_arg_type, right_arg_type) ){
                // emit type error
                print_symm_binary_type_error(e->kind, "same", left_arg_type, e->data->operator_args, right_arg_type, e->data->operator_args->next);
                typecheck_errors++;
            }
            break;
        case EXPR_OR:
        case EXPR_AND:
            if( !(type_t_equals(left_arg_type->kind,  TYPE_BOOLEAN)
               && type_t_equals(right_arg_type->kind, TYPE_BOOLEAN)) ){
                // TODO emit type error
                print_symm_binary_type_error(e->kind, "boolean", left_arg_type, e->data->operator_args, right_arg_type, e->data->operator_args->next);
                typecheck_errors++;
            }
            break;
        case EXPR_NOT:
            if( !type_t_equals(right_arg_type->kind, TYPE_BOOLEAN) ){
                // TODO emit type error
                print_unary_type_error(e->kind, true, "boolean", right_arg_type, e->data->operator_args->next);
                typecheck_errors++;
            }
            break;
        case EXPR_POST_INC:
        case EXPR_POST_DEC:
            if( !type_t_equals(left_arg_type->kind, TYPE_INTEGER) ){
                // TODO emit type error
                print_unary_type_error(e->kind, false, "integer", left_arg_type, e->data->operator_args);
                typecheck_errors++;
            }
            break;
        case EXPR_LT:
        case EXPR_LT_EQ:
        case EXPR_GT:
        case EXPR_GT_EQ:
        case EXPR_ADD:
        case EXPR_SUB:
        case EXPR_MUL:
        case EXPR_DIV:
        case EXPR_MOD:
        case EXPR_EXP:
            if( !(type_t_equals(left_arg_type->kind,  TYPE_INTEGER)
               && type_t_equals(right_arg_type->kind, TYPE_INTEGER)) ){
                // TODO emit type error
                print_symm_binary_type_error(e->kind, "integer", left_arg_type, e->data->operator_args, right_arg_type, e->data->operator_args->next);
                typecheck_errors++;
            }
            break;
        case EXPR_ADD_ID:
        case EXPR_ADD_INV:
            if( !type_t_equals(right_arg_type->kind, TYPE_INTEGER) ){
                // TODO emit type error
                print_unary_type_error(e->kind, true, "integer", right_arg_type, e->data->operator_args->next);
                typecheck_errors++;
            }
            break;
        case EXPR_ARR_ACC:
            if( !(type_t_equals(left_arg_type->kind,  TYPE_ARRAY)
               && type_t_equals(right_arg_type->kind, TYPE_INTEGER)) ){
                // emit type error
                print_asymm_binary_type_error(e->kind, "array", left_arg_type, e->data->operator_args, "integer", right_arg_type, e->data->operator_args->next);
                typecheck_errors++;
            }
            break;
        default:
            printf("Wah wah wah\n");
            break;
    }

    // construct operator return type
    struct type *to_return;
    bool is_const = (!left_arg_type  || left_arg_type->is_const_value)
                 && (!right_arg_type || right_arg_type->is_const_value);
    switch(e->kind){
        case EXPR_ASGN:
            to_return = type_copy(right_arg_type);
            to_return->is_const_value = right_arg_type->is_const_value;
            to_return->is_lvalue = false;
            break;
        case EXPR_OR:
        case EXPR_AND:
        case EXPR_LT:
        case EXPR_LT_EQ:
        case EXPR_GT:
        case EXPR_GT_EQ:
        case EXPR_EQ:
        case EXPR_NOT_EQ:
        case EXPR_NOT:
            to_return = type_create_atomic(TYPE_BOOLEAN, false, is_const);
            break;
        case EXPR_ADD:
        case EXPR_SUB:
        case EXPR_MUL:
        case EXPR_DIV:
        case EXPR_MOD:
        case EXPR_EXP:
        case EXPR_ADD_ID:
        case EXPR_ADD_INV:
        case EXPR_POST_INC:
        case EXPR_POST_DEC:
            to_return = type_create_atomic(TYPE_INTEGER, false, is_const);
            break;
        case EXPR_ARR_ACC:
            to_return = type_copy(left_arg_type->subtype);
            if( !to_return ) to_return = type_create_atomic(TYPE_INTEGER, false, is_const);
            to_return->is_lvalue = true;
            break;
        default:
            printf("Yikes: %d\n", e->kind);
            abort();
            break;
    }
    type_delete(left_arg_type);
    type_delete(right_arg_type);
    return to_return;
}

void expr_typecheck_func_args(struct expr *args, struct decl *params, char *func_name){
    // base case NULL checks
    if( !args && !params ) return;
    if( args && !params ){
        // too many args
        printf("[ERROR|typecheck] You passed too many arguments to function %s.\n", func_name);
        typecheck_errors++;
        return;
    }
    if( params && !args ){
        // too few args
        printf("[ERROR|typecheck] You passed too few arguments to function %s.\n", func_name);
        typecheck_errors++;
        return;
    }

    struct type *arg_type   = expr_typecheck(args);
    struct type *param_type = params->type;

    if( !type_equals(arg_type, param_type) ){
        // emit arg type error: maybe pass func name in too so we can cite that
        printf("[ERROR|typecheck] You passed a(n) ");
        expr_print_type_and_expr(arg_type, args);
        printf(" as an argument to %s where a(n) ", func_name);
        type_print(param_type);
        printf(" was expected.\n");
        typecheck_errors++;
    }
    type_delete(arg_type);

    expr_typecheck_func_args(args->next, params->next, func_name);
}

void expr_code_gen(struct expr *e, FILE *output){
    if (!e) return;

    // In general, we will collapse registers left.
    // There are a few cases like SUB where this is really the only way to go about it.
    // But in general it makes more sense to collapse left:
    // for an operator, the left arg is always (for both unary and binary) defined,
    // but right arg is only defined for binary.
    // So collapsing into the left arg creates a consistent collapsing scheme among all operators.
    switch(e->kind){
        case EXPR_EMPTY:
            //TODO ??
            break;
        case EXPR_ASGN:
            // TODO: look into combining /extracting common functionality from EXPR_ASGN and EXPR_POST_INC/DEC
            //   - memory address computation is the same
            // We have two lvalues in B-minor: variables and array elements (x or a[i]).
            // Either way, we must have an underlying symbol here.

            // 1. Compute the memory address to which we are assigning.
            struct expr *lvalue = e->data->operator_args;
            char *mem_addr;
            if( lvalue->kind == EXPR_ARR_ACC ){
                struct symbol *sym = lvalue->data->operator_args->symbol  // the array symbol
                expr_code_gen(lvalue->data->operator_args->next); // resolve index
                offset_reg = lvalue->data->operator_args->next->reg;
                mem_addr = symbol_to_location(sym, offset_reg);
                scratch_free(offset_reg);
            } else if( lvalue->kind  == EXPR_IDENT ){
                struct symbol *sym = lvalue->symbol; // the ident symbol
                mem_addr = symbol_to_location(sym, 0); // second arg ignored since symbol type is int (not array)
            } else {
                printf("Unexpected lvalue kind: %d. Aborting...\n", lvalue->kind);
                abort();
            }

            // 2. Move assigned value into memory address.
            expr_code_gen(e->data->operator_args); // this won't cause any complications (unintended side effects), will it?
            fprintf(output, "MOVQ %r%s, %s\n",
                    scratch_name(e->data->operator_args->next->reg),
                    mem_addr);

            // 3. Return value assigned
            e->reg = e->data->operator_args->next->reg;

            free(mem_addr); // clean up symbol_to_location's allocation
            break;
        // TODO: combine OR, AND, ADD, SUB operators using an expr_t_to_inst function or giant ternary?
        //  - or even just combine OR and AND? at the least, groups like operators.
        //  - needs some general clean up as well.
        case EXPR_OR:
            expr_code_gen(e->data->operator_args);
            e->reg = e->data->operator_args->reg; // fold left
            fprintf(output, "ORQ %s, %s\n",
                            scratch_name(e->data->operator_args->next->reg,
                            e->data->operator_args->reg));
            scratch_free(e->data->operator_args->next->reg);
            break;
        case EXPR_AND:
            expr_code_gen(e->data->operator_args);
            e->reg = e->data->operator_args->reg; // fold left
            fprintf(output, "ANDQ %s, %s\n",
                            scratch_name(e->data->operator_args->next->reg,
                            e->data->operator_args->reg));
            scratch_free(e->data->operator_args->next->reg);
            break;
        case EXPR_LT:
        case EXPR_LT_EQ:
        case EXPR_GT:
        case EXPR_GT_EQ: 
        case EXPR_EQ:     
        case EXPR_NOT_EQ: {
            expr_code_gen(e->data->operator_args);

            // compare operands
            char *left_reg  = reg_to_name(e->data->operator_args->reg);
            char *right_reg = reg_to_name(e->data->operator_args->next->reg);
            fprintf(output, "CMP %s, %s\n",
                            left_reg, right_reg);

            /* Premise: using labels to evaluate a comparison feels a little weird, and this was fun to think about.
             * This is probably not the optimal setup (jumps are probably way faster), but was a fun thought experiment.
             * So, we grab the sign and zero flags from the comparison by copying the AH reg and setting the bits beside the flag to zero.
             * To do this, we use the logical shift as demonstrated below:
             * ----X--- ==> X---0000 ==> 0000000X
             * Then, since we interpret any non-zero value as a true boolean, we can simply perform byte-scope boolean operatings on the
             * sign and zero flags and leave the result floating in the register. */
            // Move sign and zero flags into 8-bit regs
            // Construct sign and zero reg names (see x86 reg naming conventions)

            // TODO: allocate space for zero_reg and sign_reg here, outside, unconditionally, and unconditionally free later
            char *sign_reg, zero_reg;
            char *tgt_name = scratch_name(left_reg);
            if( tgt_name == "bx" ){ // others are 10,...,15
                sign_reg = "BL";
                zero_reg = "BH";
            }
            else{
                sprintf(sign_reg, "%sL", tgt_name);
                sprintf(zero_reg, "%sH", tgt_name);
            }
            fprintf(output, "LAHF\n" // load status flags into AH reg
                            "MOVB AH, \%%s\n" // copy to scratch
                            "MOVB AH, \%%s\n" // copy to other scratch
                            // grab sign flag (idx 7: just shift right)
                            "SHRB $7, \%%s%s\n" // logical shift fills in 0s
                            // grab zero flag (idx 6: shift left 1, then right 7)
                            "SHLB $1, \%%s\n"
                            "SHRB $7, \%%s\n",
                            left_reg, right_reg
                            sign_reg, zero_reg,
                            sign_reg,
                            zero_reg, zero_reg);

            switch(e->kind){
                // zero:sign, so move result into sign and clear zero
                case EXPR_GT: // sign 0 AND zero 0
                    fprintf(output, "NOTB \%%s\n"
                                    "NOTB \%%s\n"
                                    "ANDB \%%s, \%%s\n"
                                    "ANDB $0, \%%s\n",
                                    zero_reg, sign_reg,
                                    zero_reg, sign_reg,
                                    zero_reg);
                    break;
                case EXPR_GT_EQ: // sign 0
                    fprintf(output, "NOTB \%%s\n"
                                    "ANDB $0, \%%s\n",
                                    sign_reg,
                                    zero_reg);
                    break;
                case EXPR_LT: // sign 1
                    fprintf(output, "ANDB $0, \%%s", zero_reg);
                    break;
                case EXPR_LT_EQ: //sign 1 OR zero 1
                    fprintf(output, "ORB \%%s, \%%s\n"
                                    "ANDB $0, \%%s\n",
                                    zero_reg, sign_reg,
                                    zero_reg);
                    break;
                case EXPR_EQ: // zero 1
                    fprintf(output, "MOVB \%%s, \%%s\n"
                                    "ANDB $0, \%%s\n",
                                    zero_reg, sign_reg,
                                    zero_reg);
                    break;
                case EXPR_NOT_EQ: // zero 0
                    fprintf(output, "NOTB \%%s\n"
                                    "MOVB \%%s, \%%s\n"
                                    "ANDB $0, \%%s\n",
                                    zero_reg,
                                    zero_reg, sign_reg,
                                    zero_reg);
                    break;
                default: break;
            }
            e->reg = e->data->operator_args->reg; // fold left
            scratch_free(e->data->operator_args->next->reg);
        }
        case EXPR_ADD: 
        case EXPR_SUB:
            // TODO: does this evaluate all operator_args?
            expr_code_gen(e->data->operator_args);
            // 4 - 3 ~ SUBQ 3 4, so always collapse into left reg
            fprintf(output, "%sQ %r%s, %r%s\n",
                    e->kind == EXPR_ADD ? "ADD" : "SUB",
                    reg_to_name(e->data->operator_args->next->reg),
                    reg_to_name(e->data->operator_args->reg));
            e->reg = e->data->operator_args->reg;
            scratch_free(e->data->operator_args->next->reg);
            break;
        case EXPR_MUL:
        case EXPR_DIV:
        case EXPR_MOD:
            // MUL and DIV have the weird implicit rax operand: 
            // this makes it convenient to combine these cases.
            expr_code_gen(e->data->operator_args);
            expr_code_gen(e->data->operator_args->next);
            e->reg = e->data->operator_args->reg; // follow left-folding convention

            // 1. Place left operand into %rax
            //    This is either the dividend (order matters) or a factor (order doesn't matter).
            fprintf(output, "MOVQ %r%s, %rax\n",
                            scratch_name(e->data->operator_args->reg));

            // 2. Perform the operation between %rax (implicit) and other operand
            fprintf(output, "I%sQ %r%s\n",
                            e->kind == EXPR_MUL ? "MUL" : "DIV",
                            scratch_name(e->data->operator_args->next->reg));

            // 3. Retrieve interesting result
            fprintf(output, "MOVQ %r%s, %r%s\n",
                            e->kind == EXPR_MOD ? "dx" : "ax",
                            scratch_name(e->reg));

            scratch_free(e->data->operator_args->reg);
            break;
        case EXPR_EXP: 
            // Call pow (need to convert ints to floats?)
            break;
        case EXPR_NOT:
            expr_code_gen(e->data->operator_args);
            fprintf("NOTQ %r%s\n",
                    scratch_name(e->data->operator_args->reg));
            e->reg = e->data->operator_args->reg;
            break;
        case EXPR_ADD_ID:
            // no-op
            e->reg = e->data->operator_args->reg;
            break;
        case EXPR_ADD_INV: 
            expr_code_gen(e->data->operator_args);
            fprintf(output, "NEGQ %r%s\n",
                    scratch_name(e->data->operator_args->reg));
            e->reg = e->data->operator_args->reg;
            break;
        case EXPR_POST_INC: 
        case EXPR_POST_DEC:
            // We have two lvalues in B-minor: variables and array elements (x or a[i]).
            // Either way, we must have an underlying symbol here.
            // (We also know that the underlying symbols have int types since only those can be inc/dec'd)

            // 1. Compute the memory address being post-operated.
            struct expr *lvalue = e->data->operator_args;
            char *mem_addr;
            if( lvalue->kind == EXPR_ARR_ACC ){
                struct symbol *sym = lvalue->data->operator_args->symbol  // the array symbol
                expr_code_gen(lvalue->data->operator_args->next); // resolve index
                offset_reg = lvalue->data->operator_args->next->reg;
                mem_addr = symbol_to_location(sym, offset_reg);
                scratch_free(offset_reg);
            } else if( lvalue->kind  == EXPR_IDENT ){
                struct symbol *sym = lvalue->symbol; // the ident symbol
                mem_addr = symbol_to_location(sym, 0); // second arg ignored since symbol type is int (not array)
            } else {
                printf("Unexpected lvalue kind: %d. Aborting...\n", lvalue->kind);
                abort();
            }

            // 2. Load the memory address's current value to return.
            e->reg = scratch_alloc();
            fprintf(output, "MOVQ %r%s, %s\n",
                            mem_addr,
                            scratch_name(e->reg));

            // 3. Increment the memory address's value (without disturbing the results of (2)).
            fprintf(output, "%sQ $1, r%s\n",
                            e->kind == EXPR_POST_INC ? "ADD" : "SUB",
                            mem_addr); // seems legit, adding an immediate to an m32 like the manual says

            free(mem_addr); // clean up symbol_to_location's allocation
            break;
        case EXPR_ARR_ACC:
            expr_code_gen(e->data->operator_args);       // the array
            expr_code_gen(e->data->operator_args->next); // the index
            e->reg = e->data->operator_args->reg;        // fold left as always
            // Our array literal work below allows us to grab an array element super easily using the complex address mode.
            fprintf(output, "MOVQ (%r%s, %r%s, 8), %r%s",
                    scratch_name(e->data->operator_args->reg),
                    scratch_name(e->data->operator_args->next->reg),
                    scratch_name(e->reg));
            scratch_free(e->data->operator_args->next->reg);
            break;
        case EXPR_ARR_LIT: 
            // Fundamental idea: allocate elements of an array literal on the stack,
            // and return its starting address as the "contents" of an array variable
            // Specifically, we resolve each array element and it push it onto stack in *reverse* order.
            // This makes it more natural to index an array later: a[5] becomes <a_stack_idx>(%rbp, <index>, 8).
            // That is, to traverse the array we move in the positive direction (up the stack),
            // which works nicely with the complex address mode, which cannot be negative.
            // To do this requires a bit more work here, but a bit less work later.
            // I dig that, in general: do the hard work early, at a single source,
            // and make all code referencing array literals simpler later: concentrate the complexity.
            // This will also hopefully make all array accesses (local and global) uniform.
            // TODO: what to do about 0 length arrays? have typechecker reject them?

            // 1. Determine the array literal's length.
            //    This will enable us to calculate the addresses to which to "reverse push".
            int arr_len = 0;
            for( struct *expr ele = e->data->arr_elements; ele; ele = ele->next ){
                arr_len += 1;
            }

            // 2. Allocate space on the stack for the array elements.
            //    (treating everthing as 64-bit/quad word)
            fprintf(output, "SUBQ $%d, %rsp",
                    8 * arr_len);

            // 3. Load the array "value": the address of its first element (which, post-allocation, is now the stack pointer).
            int e->reg = scratch_alloc();
            fprintf(output, "MOVQ %rsp, %r%s",
                    scratch_name(e->reg));

            // 4. "Reverse push" each element onto the stack.
            int arr_idx = 0;
            for( struct *expr ele = e->data->arr_elements; ele; ele = ele->next, arr_idx++ ){
                expr_code_gen(ele);
                fprintf(output, "MOVQ %r%s, -%d(%rsp)\n",
                        scratch_name(ele->reg),
                        8 * arr_idx);
                scratch_free(ele->reg);
            }
            break;
        case EXPR_FUNC_CALL:
            // 1. Save caller-saved registers (%r10, %r11).
            //    (Again, not concerned with optimizing for whether these are actually in use by the caller)
            fprintf(output, "PUSHQ %r10\n");
            fprintf(output, "PUSHQ %r11\n");
            
            // 2. Load up to 6 arguments into the designated registers.
            char *arg_idx_to_reg[] = {"di", "si", "dx", "cx", "8", "9"};
            struct expr *curr_arg = e->data->func_and_args->next;
            for( int i = 0; i < 6 && curr_arg; i++, curr_arg = curr_arg->next ){
                expr_code_gen(curr_arg);
                fprintf(output, "MOVQ %r%s, %r%s\n,
                        scratch_name(curr_arg->reg),
                        arg_idx_to_reg[i]);
                scratch_free(curr_arg->reg);
            }

            // 3. Place the remaining arguments onto the caller stack frame, in reverse order.
            //    a. Count the number of remaining arguments.
            int rem_args = 0;
            for( struct expr *rem_arg = curr_arg; rem_arg; rem_arg = rem_arg->next ){
                rem_args += 1;
            }
            //    b. Allocate stack space for the remaining arguments.
            //       (May be a no-op if there are no remaining arguments)
            fprintf(output, "SUBQ $%d, %rsp\n",
                    8 * rem_args);
            //    c. "Reverse push" the remaining arguments onto stack frame.
            int stack_bottom_offset = 0;
            for( ; curr_arg; curr_arg = curr_arg->next, stack_bottom_offset++ ){
                expr_code_gen(curr_arg);
                fprintf(output, "MOVQ %r%s, -%d(%rsp)\n",
                        scratch_name(curr_arg->reg),
                        8 * stack_bottom_offset);
                scratch_free(curr_arg->reg);
            }

            // 4. Call the function.
            //    Currently, functions may only be declared and defined as global variables.
            //    That is, there is no expression resolvable to a function other than the function's identifier.
            //    Thus, we can assume that the expression resolving to the function being called is an identifier expression.
            //    This allows us to simply extract the ident name from the function call,
            //    rather than worrying about code-generating the expression and using the resolved value from a register.
            //    (Indeed, how would we use the resolution in the register?
            //     We have no representation for a function other than the label to which to jump to execute it).
            if( e->data->func_and_args->kind != EXPR_IDENT ){
                printf("Code generator is not set up to handle function expressions other than identifiers. Aborting...\n");
                abort();
            }
            fprintf(output, "CALL %s", e->data->func_and_args->data->ident_name);
            
            // 5. Retrieve return value.
            e->reg = scratch_alloc();
            fprintf(output, "MOVQ %rax, %r%s",
                    scratch_name(e->reg));
            
            // 6. Restore caller-saved registers.
            fprintf(output, "POPQ %r11\n");
            fprintf(output, "POPQ %r10\n");
            break;
        case EXPR_IDENT:
            // symbol to addr
            // move addr into reg
            char *var_loc = symbol_to_location(e->data->operator_args->ident_name, -1);
            int reg = scratch_alloc();
            fprintf(output, "MOVQ %s, %r%s\n", var_loc, reg_name(tmp_reg));
            free(var_loc);
            e->reg = reg;
            break;
        case EXPR_INT_LIT:
            e->reg = scratch_alloc();
            fprintf(output, "MOVQ %d, %r%s\n",
                    e->data->int_data,
                    scratch_name(e->reg));
            break;
        case EXPR_STR_LIT:
            // It seems we'll want some mechanism to gather the string literals found and place them in the data section.
            // Likewise, it seems we'll need a way to map which label we used for which string literal to reference them later.
            printf("no implementation for string literals rn, sorry\n");
            abort();
            break;
        case EXPR_CHAR_LIT:
            e->reg = scratch_alloc();
            fprintf(output, "MOVQ %d, %r%s\n",
                    e->data->char_data,
                    scratch_name(e->reg));
            break;
        case EXPR_BOOL_LIT:
            e->reg = scratch_alloc();
            // cast bool to int and move it into the reg
            fprintf(output, "MOVQ %d, %r%s\n",
                    e->data->bool_data,
                    scratch_name(e->reg));
            break;
        default:
            printf("Unexpected expr kind: %d. Aborting...\n", e->kind);
            abort();
            break;
    }

    expr_code_gen(e->next);
}


// move this array to top eventually
bool  reg_in_use[7]     = { 0 };
char *reg_name[7]       = { "bx", "10", "11", "12", "13", "14", "15" }
int scratch_alloc(){
    for(int i = 0; i < (sizeof reg_in_use)/(sizeof reg_in_use[0]); i++){
        if( !reg_in_use[i] ) return i;
    }
    // all registers in use
    return -1;
}

void scratch_free(int r){ reg_in_use[r] = 0; }

char *scratch_name(int r){ return reg_name[r]; }

void print_symm_binary_type_error(expr_t oper, char *expected_type, struct type *left_type, struct expr *left_expr, struct type *right_type, struct expr *right_expr){
    char postamble[BUFSIZ];
    sprintf(postamble, "The '%s' operator accepts two operands of the %s type",
            oper_to_str(oper),
            expected_type);
    print_binary_type_error(oper, postamble, left_type, left_expr, right_type, right_expr);
}

void print_asymm_binary_type_error(expr_t oper, char *expected_left_type, struct type *left_type, struct expr *left_expr, char *expected_right_type, struct type *right_type, struct expr *right_expr){
    char postamble[BUFSIZ];
    sprintf(postamble, "The '%s' operator accepts a left operand of %s type and a right operand of %s type",
            oper_to_str(oper),
            expected_left_type,
            expected_right_type);
    print_binary_type_error(oper, postamble, left_type, left_expr, right_type, right_expr);
}

void print_unary_type_error(expr_t oper, bool is_right_unary, char *expected_type, struct type *arg_type, struct expr *arg_expr){
    printf("[ERROR|typecheck] You passed to the '%s' operator %s ",
            oper_to_str(oper), is_vowel(*type_t_to_str(arg_type->kind)) ? "an" : "a");
    expr_print_type_and_expr(arg_type, arg_expr);
    printf(". The '%s' operator accepts a single operand of %s type, placed to the %s.\n",
            oper_to_str(oper), expected_type, is_right_unary ? "right" : "left");
}

void print_binary_type_error(expr_t oper, char *postamble, struct type *left_type, struct expr *left_expr, struct type *right_type, struct expr *right_expr){
    // NULL type indicates unary operator with empty left/right operand
    bool left_an  = is_vowel(*type_t_to_str(left_type->kind));
    bool right_an = is_vowel(*type_t_to_str(right_type->kind));
    printf("[ERROR|typecheck] You passed the '%s' operator %s ", oper_to_str(oper), left_an ? "an" : "a");
    type_print(left_type);
    printf(" left operand (`");
    expr_print(left_expr);
    printf("`) and %s ", right_an ? "an" : "a");
    type_print(right_type);
    printf(" right operand (`");
    expr_print(right_expr);
    printf("`). %s.\n", postamble);
}

void expr_print_type_and_expr(struct type *t, struct expr *e){
    type_print(t);
    printf(" (`");
    expr_print(e);
    printf("`)");
}

int expr_eval_const_int(struct expr *e){
    // return dummy value that will be ignored
    if( !e || e->kind == EXPR_EMPTY )   return 0;

    if( e->kind == EXPR_INT_LIT )       return e->data->int_data;
    
    // won't necessarily use these (i.e. unary operators), but I'll set em up here
    int l = expr_eval_const_int(e->data->operator_args);
    int r = expr_eval_const_int(e->data->operator_args->next);

    switch( e->kind ){
        case EXPR_ASGN:
            return r;
            break;
        case EXPR_ADD:
            return l + r;
            break;
        case EXPR_SUB:
            return l - r;
            break;
        case EXPR_MUL:
            return l * r;
            break;
        case EXPR_DIV:
            return l / r;
            break;
        case EXPR_MOD:
            return l % r;
            break;
        case EXPR_EXP:
            return pow(l, r);
            break;
        case EXPR_ADD_ID:
            return r;
            break;
        case EXPR_ADD_INV:
            return -r;
            break;
        default:
            printf("Expr eval failed for expr: ");
            expr_print(e);
            printf("\n");
            break;
    }
    // shouldn't get here, but just in case
    return 0;
}

bool is_vowel(char c){
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

char *oper_to_str(expr_t t){
    char *strs[] = <oper_str_arr_placeholder>;
    return (t < <first_oper_placeholder> || t > <last_oper_placeholder>) ? "" : strs[t - <first_oper_placeholder>];
}

void expr_print(struct expr *e){
    if (!e) return;

    char *s;
    switch(e->kind){
        case EXPR_EMPTY:
            break;
        case EXPR_ARR_ACC:
            // expecting exactly two arguments: expression resolving to array and indexing expression
            expr_print_list(e->data->operator_args, "[");
            fputs("]", stdout);
            break;
        case EXPR_ARR_LIT:
            fputs("{", stdout);
            expr_print_list(e->data->arr_elements, ", ");
            fputs("}", stdout);
            break;
        case EXPR_FUNC_CALL:
            expr_print(e->data->func_and_args);
            fputs("(", stdout);
            expr_print_list(e->data->func_and_args->next, ", ");
            fputs(")", stdout);
            break;
        case EXPR_IDENT:
            fputs(e->data->ident_name, stdout);
            break;
        case EXPR_INT_LIT:
            printf("%d", e->data->int_data);
            break;
        case EXPR_STR_LIT:
            // revrese clean string function from scanner
            descape_and_print_str_lit(e->data->str_data);
            break;
        case EXPR_CHAR_LIT:
            descape_and_print_char_lit(e->data->char_data);
            break;
        case EXPR_BOOL_LIT:
            fputs(e->data->bool_data ? "true" : "false", stdout);
            break;
        default:
            //operators
            /* this printing code is made elegant by allowing the AST to have empty nodes */
            s = e->data->operator_args->kind == EXPR_EMPTY || e->data->operator_args->next->kind == EXPR_EMPTY ? "" : " ";
            expr_print_subexpr(e->data->operator_args, e->kind, false);
            printf("%s%s%s", s, oper_to_str(e->kind), s);
            expr_print_subexpr(e->data->operator_args->next, e->kind, true);
            break;
    }
}

void expr_print_subexpr(struct expr *e, expr_t parent_oper, bool right_oper){ 
    if (!e || e->kind == EXPR_EMPTY) return;

    // if parent operator is non-commutative and we are the operand opposite the associativy of the operator, wrap in parens ( a - (b - c) | (a = b) = c or (a = &b) = c )
    bool commutativities[] = <commutativities_arr_placeholder>;
    // false = 0, hence left (0 to left of 1 on number line)
    bool associativities[] = <associativities_arr_placeholder>;

    // to see relevance, consider the cases | (3+4)*5 | a - -b | - -(a - - b) |
    bool wrap_in_parens = oper_precedence(parent_oper) > oper_precedence(e->kind)
        || (parent_oper == e->kind && (parent_oper == EXPR_ADD_INV || parent_oper == EXPR_ADD_ID))
        // now that I include spaces in my expressions, parentheses are not strictly necessary in this case
        //|| (parent_oper == EXPR_ADD && e->kind == EXPR_ADD_ID)
        //|| (parent_oper == EXPR_SUB && e->kind == EXPR_ADD_INV)
        || (parent_oper == e->kind
                && parent_oper - <first_oper_placeholder> < sizeof(commutativities)/sizeof(*commutativities)
                && !commutativities[parent_oper - <first_oper_placeholder>]
                && associativities[parent_oper - <first_oper_placeholder>] != right_oper);
    if (wrap_in_parens) fputs("(", stdout);
    expr_print(e);
    if (wrap_in_parens) fputs(")", stdout);
}

void expr_print_list(struct expr *e, char *delim){
    if(!e) return;
    
    expr_print(e);
    if (e->next) fputs(delim, stdout);
    expr_print_list(e->next, delim);
}

char *descape_char(char c, char delim){
    char *clean = malloc(3);
    if(!clean){
        puts("[ERROR|internal] Failed to allocate clean char memory, exiting...");
        exit(EXIT_FAILURE);
    }
    char *writer = clean;
    switch(c){
        case '\n':
            *(writer)    = '\\';
            *(++writer)  = 'n';
            break;
        case '\\':
            *(writer)    = '\\';
            *(++writer)  = '\\';
            break;
        default:
            if(c == delim){
                *(writer)    = '\\';
                *(++writer)  = delim;
            }
            else  *(writer)    = c;
            break;
    }
    *(++writer) = '\0';
    return clean;
}

void descape_and_print_char_lit(char c){
    char clean[5] = "'";
    char *clean_c = descape_char(c, '\'');
    strcat(clean, clean_c);
    strcat(clean, "'");
    fputs(clean, stdout);
    free(clean_c);
}

void descape_and_print_str_lit(const char *s){
    // we'll need at most twice the characters, plus delimeters, plus nul
    char *clean = malloc(2 * strlen(s) + 3);
    if(!clean){
        puts("Failed to allocate clean char memory, exiting...");
        exit(EXIT_FAILURE);
    }
    clean[0] = '"';
    clean[1] = '\0';
    for (const char *reader = s; *reader; reader++){
        char *next = descape_char(*reader, '"');
        strcat(clean, next);
        free(next);
    }
    strcat(clean, "\"");
    fputs(clean, stdout);
    free(clean);
}

int oper_precedence(expr_t t){
    // handle against unlikely empty expr here
    if( t == EXPR_EMPTY )   return INT_MAX;
    int oper_precs[] = <oper_precedences_arr_placeholder>;
    return oper_precs[t - <first_oper_placeholder>];
}


