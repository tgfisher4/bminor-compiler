#include "expr.h"
#include "scope.h"
#include "code_gen_utils.h"
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
    e->reg = -1; // no sane default here

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

struct expr *expr_copy(struct expr *e){
    union expr_data *d = malloc(sizeof(*d));
    memcpy(d, e->data, sizeof(*d));
    return expr_create(e->kind, d);
}

void expr_delete(struct expr *e){
    if( !e )  return;
    bool is_atomic = e->kind == EXPR_INT_LIT
                    || e->kind == EXPR_CHAR_LIT
                    || e->kind == EXPR_BOOL_LIT
                    || e->kind == EXPR_STR_LIT
                    || e->kind == EXPR_IDENT;
    if( !is_atomic ){
        // Just interpreting the data as expr *:
        // the operator_args is insignificant.
        expr_delete(e->data->operator_args);
    }
    free(e->data);
    expr_delete(e->next);
    free(e);
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
        // An array is const if all of its elements are const.
        bool is_const = true;
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
            is_const = is_const && curr_type->is_const_value;
            type_delete(curr_type);
        }
        struct type* to_return = type_create(
                TYPE_ARRAY,
                expected_type,
                expr_create_integer_literal(sz),
                NULL
        );
        to_return->is_const_value = is_const;
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
            // why wouldn't an array have a subtype?
            if( !to_return ) to_return = type_create_atomic(TYPE_INTEGER, false, is_const);
            to_return->is_lvalue = true;
            to_return->is_const_value = is_const;
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

void expr_code_gen(FILE *output, struct expr *e){
    if (!e) return;

    // In general, we will collapse registers left.
    // There are a few cases like SUB where this is really the only way to go about it.
    // But in general it makes more sense to collapse left:
    // for an operator, the left arg is always (for both unary and binary) defined,
    // but right arg is only defined for binary.
    // So collapsing into the left arg creates a consistent collapsing scheme among all operators.
    switch(e->kind){
        case EXPR_EMPTY:
            // This should never be called:
            // unary operators should just not code_gen for their empty operand.
            break;
        case EXPR_ASGN: {
            // TODO: look into combining /extracting common functionality from EXPR_ASGN and EXPR_POST_INC/DEC
            //   - memory address computation is the same
            // We have two lvalues in B-minor: variables and array elements (x or a[i]).
            // Either way, we must have an underlying symbol here.

            // 1. Compute the memory address to which we are assigning.
            struct expr *lvalue = e->data->operator_args;
            char *mem_addr;
            if( lvalue->kind == EXPR_ARR_ACC ){
                struct symbol *sym = lvalue->data->operator_args->symbol;  // the array symbol
                expr_code_gen(output, lvalue->data->operator_args->next); // resolve index
                int offset_reg = lvalue->data->operator_args->next->reg;
                mem_addr = symbol_to_location(sym, offset_reg);
                scratch_free(offset_reg);
            } else if( lvalue->kind  == EXPR_IDENT ){
                struct symbol *sym = lvalue->symbol; // the ident symbol
                mem_addr = symbol_to_location(sym, -1);
            } else {
                printf("Unexpected lvalue kind: %d. Aborting...\n", lvalue->kind);
                abort();
            }

            // 2. Move assigned value into memory address.
            expr_code_gen(output, e->data->operator_args);
            fprintf(output, "MOVQ %%r%s, %s\n",
                    scratch_name(e->data->operator_args->next->reg),
                    mem_addr);

            // 3. Return value assigned
            e->reg = e->data->operator_args->next->reg;

            free(mem_addr); // clean up symbol_to_location's allocation
            break;
        }
        // TODO: combine OR, AND, ADD, SUB operators using an expr_t_to_inst function or giant ternary?
        case EXPR_OR:
        case EXPR_AND: {
            expr_code_gen(output, e->data->operator_args);
            expr_code_gen(output, e->data->operator_args->next);
            e->reg = e->data->operator_args->reg; // fold left
            fprintf(output, "%sQ %%r%s, %%r%s\n",
                            e->kind == EXPR_OR
                            ? "OR"
                            : "AND",
                            scratch_name(e->data->operator_args->next->reg),
                            scratch_name(e->data->operator_args->reg));
            scratch_free(e->data->operator_args->next->reg);
            break;
        }
        case EXPR_LT:
        case EXPR_LT_EQ:
        case EXPR_GT:
        case EXPR_GT_EQ: 
        case EXPR_EQ:     
        case EXPR_NOT_EQ: {
            expr_code_gen(output, e->data->operator_args);
            expr_code_gen(output, e->data->operator_args->next);
            e->reg = e->data->operator_args->reg; // fold left
            char *tgt_reg  = scratch_name(e->reg);

            // compare operands
            fprintf(output, "CMP %%r%s, %%r%s\n",
                            scratch_name(e->data->operator_args->reg),
                            scratch_name(e->data->operator_args->next->reg));

            /* Premise: using labels to evaluate a comparison feels a little weird, and this was fun to think about.
             * This is probably not the optimal setup (jumps gotta be way faster), but was a fun thought experiment.
             * So, we grab the sign and zero flags from the comparison by copying the AH reg and setting the bits beside the flag to zero.
             * To do this, we use the logical shift as demonstrated below:
             * ----X--- ==> X---0000 ==> 0000000X
             * Thus, we store the sign and bit flags into individual bytes,
             * and perform byte-scope boolean operatings between them.
             * We leave the result floating in the bottom byte of the target reg,
             * then AND the reg with $1 to clear all but the LSB,
             * which holds the result of the operations on the sign and flag bits.
             * Thus, the resulting quad reg will hold a non-zero value iff the
             * result of the operations on the sign and flag bits was non-zero.
             */

            // Move sign and zero flags into 8-bit regs
            // Construct sign and zero reg names (see x86 reg naming conventions)

            // Design flaw: want to encapsulate knowledge of scratch registers in codegen_utils, but this bit here requires intimiate knowledge of inner workings
            // 2 = max(len(B), len(10), ..., len(15))
            // 1 = max(len(L), len(H))
            // 1 = \0
            //char *sign_reg = malloc(2 + 1 + 1);
            //char *zero_reg = malloc(2 + 1 + 1);
            char sign_reg[2 + 1 + 1];
            char zero_reg[2 + 1 + 1];
            
            // Split the tgt reg as our sign and zero regs
            if( !strcmp(tgt_reg, "bx") ){
                sprintf(sign_reg, "BL");
                sprintf(zero_reg, "BH");
            }
            else { // others are 10,...,15
                sprintf(sign_reg, "%sL", tgt_reg);
                sprintf(zero_reg, "%sH", tgt_reg);
            }
            fprintf(output, "LAHF\n" // load status flags into AH reg
                            "MOVB AH, %%r%s\n" // copy to one scratch
                            "MOVB AH, %%r%s\n" // copy to other scratch
                            // grab sign flag (idx 7: just shift right)
                            "SHRB $7, %%r%s\n" // logical shift: fills in 0s
                            // grab zero flag (idx 6: shift left 1, then right 7)
                            "SHLB $1, %%r%s\n"
                            "SHRB $7, %%r%s\n",
                            sign_reg, zero_reg,
                            sign_reg,
                            zero_reg, zero_reg);

            switch(e->kind){
                // tgt reg = zero:sign, so move result into sign and clear zero
                case EXPR_GT: // sign 0 AND zero 0
                    fprintf(output, "NOTB %%r%s\n" // !zero
                                    "NOTB %%r%s\n" // !sign
                                    "ANDB %%r%s, %%r%s\n" // tgt <- !zero && !sign
                                    "ANDQ $1, %%r%s\n", // clear collateral
                                    zero_reg, sign_reg,
                                    zero_reg, sign_reg,
                                    tgt_reg);
                    break;
                case EXPR_GT_EQ: // sign 0
                    fprintf(output, "NOTB %%r%s\n" // tgt <- !sign
                                    "ANDQ $1, %%r%s\n", // clear collateral
                                    sign_reg,
                                    tgt_reg);
                    break;
                case EXPR_LT: // sign 1
                    fprintf(output, "ANDQ $1, %%r%s", tgt_reg); // clear collateral
                    break;
                case EXPR_LT_EQ: //sign 1 OR zero 1
                    fprintf(output, "ORB %%r%s, %%r%s\n" // tgt <- zero || sign
                                    "ANDQ $1, %%r%s\n", // clear collateral
                                    zero_reg, sign_reg,
                                    tgt_reg);
                    break;
                case EXPR_EQ: // zero 1
                    fprintf(output, "MOVB %%r%s, %%r%s\n" // tgt <- zero
                                    "ANDQ $1, %%r%s\n",  // clear collateral
                                    zero_reg, sign_reg,
                                    tgt_reg);
                    break;
                case EXPR_NOT_EQ: // zero 0
                    fprintf(output, "NOTB %%r%s\n" // !zero
                                    "MOVB %%r%s, %%r%s\n" // tgt <- !zero
                                    "ANDQ $1, %%r%s\n", // clear collateral
                                    zero_reg,
                                    zero_reg, sign_reg,
                                    tgt_reg);
                    break;
                default:
                    printf("Unexpected expr kind in bool code gen: %d. Aborting...\n", e->kind);
                    abort();
            }
            scratch_free(e->data->operator_args->next->reg);
            //free(sign_reg);
            //free(zero_reg);
        }
        case EXPR_ADD: 
        case EXPR_SUB:
            expr_code_gen(output, e->data->operator_args);
            expr_code_gen(output, e->data->operator_args->next);
            // 4 - 3 ~ SUBQ 3 4, so always collapse into left reg
            fprintf(output, "%sQ %%r%s, %%r%s\n",
                    e->kind == EXPR_ADD ? "ADD" : "SUB",
                    scratch_name(e->data->operator_args->next->reg),
                    scratch_name(e->data->operator_args->reg));
            e->reg = e->data->operator_args->reg;
            scratch_free(e->data->operator_args->next->reg);
            break;
        case EXPR_MUL:
        case EXPR_DIV:
        case EXPR_MOD:
            // MUL and DIV have the weird implicit rax operand: 
            // this makes it convenient to combine these cases.
            expr_code_gen(output, e->data->operator_args);
            expr_code_gen(output, e->data->operator_args->next);
            e->reg = e->data->operator_args->reg; //  fold left

            // 1. Place left operand into %%rax
            //    This is either the dividend (order matters: must be left) or a factor (order doesn't matter), so just always put left operand.
            fprintf(output, "MOVQ %%r%s, %%rax\n",
                            scratch_name(e->data->operator_args->reg));

            // 2. Perform the operation between %%rax (implicit) and other operand
            fprintf(output, "I%sQ %%r%s\n",
                            e->kind == EXPR_MUL ? "MUL" : "DIV",
                            scratch_name(e->data->operator_args->next->reg));

            // 3. Retrieve interesting result
            fprintf(output, "MOVQ %%r%s, %%r%s\n",
                            e->kind == EXPR_MOD ? "dx" : "ax",
                            scratch_name(e->reg));

            scratch_free(e->data->operator_args->reg);
            break;
        case EXPR_EXP: {
            // Call a special runtime pow (need to convert ints to floats?)
 
            // Create copies that can be deleted in expr_delete
            struct expr *base = expr_copy(e->data->operator_args);
            struct expr *xp = expr_copy(e->data->operator_args->next);
            base->next = xp;
            struct expr *runtime_pow_call = expr_create_function_call(
                expr_create_identifier("bminor_runtime_pow"),
                base
            );
            expr_code_gen(output, runtime_pow_call);
            e->reg = runtime_pow_call->reg;
            expr_delete(runtime_pow_call); // also deletes base and xp
            break;
        }
        case EXPR_NOT:
            expr_code_gen(output, e->data->operator_args);
            fprintf(output, "NOTQ %%r%s\n",
                    scratch_name(e->data->operator_args->reg));
            e->reg = e->data->operator_args->reg;
            break;
        case EXPR_ADD_ID:
            // no-op
            e->reg = e->data->operator_args->reg;
            break;
        case EXPR_ADD_INV: 
            expr_code_gen(output, e->data->operator_args->next);
            fprintf(output, "NEGQ %%r%s\n",
                    scratch_name(e->data->operator_args->next->reg));
            e->reg = e->data->operator_args->next->reg;
            break;
        case EXPR_POST_INC: 
        case EXPR_POST_DEC: {
            // We have two lvalues in B-minor: variables and array elements (x or a[i]).
            // Either way, we must have an underlying symbol here.
            // (We also know that the underlying symbols have int types since only those can be inc/dec'd)

            // 1. Compute the memory address being post-operated.
            struct expr *lvalue = e->data->operator_args;
            char *mem_addr;
            if( lvalue->kind == EXPR_ARR_ACC ){
                struct symbol *sym = lvalue->data->operator_args->symbol; // the array symbol
                expr_code_gen(output, lvalue->data->operator_args->next); // resolve index
                int offset_reg = lvalue->data->operator_args->next->reg;
                mem_addr = symbol_to_location(sym, offset_reg);
                scratch_free(offset_reg);
            } else if( lvalue->kind  == EXPR_IDENT ){
                struct symbol *sym = lvalue->symbol; // the ident symbol
                mem_addr = symbol_to_location(sym, -1);
            } else {
                printf("Unexpected lvalue kind: %d. Aborting...\n", lvalue->kind);
                abort();
            }

            // 2. Load the memory address's current value to return.
            e->reg = scratch_alloc();
            fprintf(output, "MOVQ %%r%s, %s\n",
                            mem_addr,
                            scratch_name(e->reg));

            // 3. Increment the memory address's value (without disturbing the results of (2)).
            fprintf(output, "%sQ $1, r%s\n",
                            e->kind == EXPR_POST_INC ? "ADD" : "SUB",
                            mem_addr); // seems legit, adding an immediate to an m32 like the manual says

            free(mem_addr); // clean up symbol_to_location's allocation
            break;
        }
        case EXPR_ARR_ACC:
            expr_code_gen(output, e->data->operator_args);       // the array
            expr_code_gen(output, e->data->operator_args->next); // the index
            e->reg = e->data->operator_args->reg;        // fold left as always
            // Our array literal work below allows us to grab an array element super easily using the complex address mode.
            fprintf(output, "MOVQ (%%r%s, %%r%s, 8), %%r%s",
                    scratch_name(e->data->operator_args->reg),
                    scratch_name(e->data->operator_args->next->reg),
                    scratch_name(e->reg));
            scratch_free(e->data->operator_args->next->reg);
            break;
        case EXPR_ARR_LIT: {
            // Fundamental idea: allocate elements of an array literal on the stack,
            // and return its starting address as the "contents" of an array variable
            // Specifically, we resolve each array element and it push it onto stack in *reverse* order.
            // This makes it more natural to index an array later: a[5] becomes <a_stack_idx>(%%rbp, <index>, 8).
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
            for( struct expr *ele = e->data->arr_elements; ele; ele = ele->next ){
                arr_len += 1;
            }

            // 2. Allocate space on the stack for the array elements.
            //    (treating everthing as 64-bit/quad word)
            fprintf(output, "SUBQ $%d, %%rsp",
                    8 * arr_len);

            // 3. Load the array "value": the address of its first element (which, post-allocation, is now the stack pointer).
            e->reg = scratch_alloc();
            fprintf(output, "MOVQ %%rsp, %%r%s",
                    scratch_name(e->reg));

            // 4. "Reverse push" each element onto the stack.
            int arr_idx = 0;
            for( struct expr *ele = e->data->arr_elements; ele; arr_idx++, ele = ele->next ){
                expr_code_gen(output, ele);
                fprintf(output, "MOVQ %%r%s, -%d(%%rsp)\n",
                        scratch_name(ele->reg),
                        8 * arr_idx);
                scratch_free(ele->reg);
            }
            break;
        }
        case EXPR_FUNC_CALL:
            // 1. Save caller-saved registers (%%r10, %%r11).
            //    (Again, not concerned with optimizing for whether these are actually in use by the caller)
            for( int i = 0; i < num_caller_saved_regs; i++ ){
                fprintf(output, "PUSHQ %%r%s\n", caller_saved_regs[i]);
            }
            
            // 2. Load up first few arguments into the designated registers.
            struct expr *curr_arg = e->data->func_and_args->next;
            for( int i = 0; i < num_func_arg_regs && curr_arg; i++, curr_arg = curr_arg->next ){
                expr_code_gen(output, curr_arg);
                fprintf(output, "MOVQ %%r%s, %%r%s\n",
                        scratch_name(curr_arg->reg),
                        func_arg_regs[i]);
                scratch_free(curr_arg->reg);
            }

            // 3. Place the remaining arguments onto the caller stack frame, in reverse order.
            //    a. Count the number of remaining arguments.
            int rem_args = 0;
            for( struct expr *x = curr_arg; x; x = x->next ){
                rem_args += 1;
            }
            //    b. Allocate stack space for the remaining arguments.
            //       (May be a no-op if there are no remaining arguments)
            fprintf(output, "SUBQ $%d, %%rsp\n",
                    8 * rem_args);
            //    c. "Reverse push" the remaining arguments onto stack frame.
            int stack_bottom_offset = 0;
            for( ; curr_arg; curr_arg = curr_arg->next, stack_bottom_offset++ ){
                expr_code_gen(output, curr_arg);
                fprintf(output, "MOVQ %%r%s, -%d(%%rsp)\n",
                        scratch_name(curr_arg->reg),
                        8 * stack_bottom_offset);
                scratch_free(curr_arg->reg);
            }

            // 4. Call the function.
            //    Functions may only be declared and defined as global variables.
            //    Thus, there is no expression resolvable to a function other than the function's identifier.
            //    So, we can assume that the expression resolving to the function being called is an identifier expression.
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
            fprintf(output, "MOVQ %%rax, %%r%s",
                    scratch_name(e->reg));
            
            // 6. Restore caller-saved registers.
            for( int i = num_caller_saved_regs - 1; i >= 0; i-- ){
                fprintf(output, "POPQ %%r%s\n", caller_saved_regs[i]);
            }
            break;
        case EXPR_IDENT: {
            char *var_loc = symbol_to_location(e->symbol, -1);
            e->reg = scratch_alloc();
            fprintf(output, "MOVQ %s, %%r%s\n", var_loc, scratch_name(e->reg));
            free(var_loc);
            break;
        }
        case EXPR_INT_LIT:
            e->reg = scratch_alloc();
            fprintf(output, "MOVQ $%d, %%r%s\n",
                    e->data->int_data,
                    scratch_name(e->reg));
            break;
        case EXPR_STR_LIT:
            // Switch sxn to rodata, insert string literal,
            // then switch back to text and load the string literal.
            e->reg = scratch_alloc();
            char *str_label = label_create();
            fprintf(output, ".section  .rodata\n" // switch to rodata sxn
                            "%s:    .string \"%s\"\n"
                            ".text\n" // switch back to text (code) sxn
                            "MOVQ $%s, %%r%s\n", // load data from new label
                            str_label, e->data->str_data,
                            str_label, scratch_name(e->reg));
            free(str_label);
            break;
        case EXPR_CHAR_LIT:
            e->reg = scratch_alloc();
            // cast bool_data to int and move it into the reg
            fprintf(output, "MOVQ $%d, %%r%s\n",
                    e->data->char_data,
                    scratch_name(e->reg));
            break;
        case EXPR_BOOL_LIT:
            e->reg = scratch_alloc();
            // cast bool_data to int and move it into the reg
            fprintf(output, "MOVQ $%d, %%r%s\n",
                    e->data->bool_data,
                    scratch_name(e->reg));
            break;
        default:
            printf("Unexpected expr kind: %d. Aborting...\n", e->kind);
            abort();
            break;
    }
}

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

#define EVAL_BIN_OP(op, intype, outtype) { \
    struct expr *to_return = expr_create_ ## outtype ## _literal( \
        l->data->intype ## _data \
        op \
        r->data->intype ## _data \
    ); \
    expr_delete(l); \
    expr_delete(r); \
    return to_return; \
}

struct expr *expr_eval_const(struct expr *e){
    if( !e || e->kind == EXPR_EMPTY ) return NULL; // ??
    // Literals
    switch( e->kind ){
        case EXPR_INT_LIT:
        case EXPR_CHAR_LIT:
        case EXPR_BOOL_LIT:
        case EXPR_STR_LIT:
            // Copy to allow unconditional freeing later
            return expr_copy(e);
        case EXPR_ARR_LIT: {
            struct expr *to_return = expr_create_array_literal(NULL);
            struct expr *tail;
            for( struct expr *ele = e->data->arr_elements; ele; ele = ele->next ){
                struct expr *evald = expr_eval_const(ele);
                // If first ele, set up to_return
                if( ele == e->data->arr_elements ){
                    to_return->data->arr_elements = evald;
                // Otherwise, string evald along to tail
                } else {
                    tail->next = evald;
                }
                tail = evald;
            }
            return to_return;
        }
        default: break; // Don't expect to catch every case here
    }

    // Operators
    struct expr *l = expr_eval_const(e->data->operator_args);
    struct expr *r = expr_eval_const(e->data->operator_args->next);
    switch( e->kind ){
        // ez cases
        case EXPR_ADD:
            EVAL_BIN_OP(+, int, integer);
        case EXPR_SUB:
            EVAL_BIN_OP(-, int, integer);
        case EXPR_MUL:
            EVAL_BIN_OP(*, int, integer);
        case EXPR_DIV:
            EVAL_BIN_OP(/, int, integer);
        case EXPR_MOD:
            EVAL_BIN_OP(%, int, integer);
        case EXPR_OR:
            EVAL_BIN_OP(||, bool, boolean);
        case EXPR_AND:
            EVAL_BIN_OP(&&, bool, boolean);
        case EXPR_LT:
            EVAL_BIN_OP(<, int, boolean);
        case EXPR_LT_EQ:
            EVAL_BIN_OP(<=, int, boolean);
        case EXPR_GT:
            EVAL_BIN_OP(>, int, boolean);
        case EXPR_GT_EQ:
            EVAL_BIN_OP(>=, int, boolean);
        // interesting cases
        case EXPR_ASGN:
            expr_delete(l);
            return r;
        case EXPR_EQ:
            // Compare literal operands, interpreted as integers.
            // Note that this means arrays and strings with same contents
            // will NOT evaluate to equal.
            // This is how string/array comparison works at runtime, too.
            EVAL_BIN_OP(==, int, boolean);
        case EXPR_NOT_EQ:
            // See comment above
            EVAL_BIN_OP(!=, int, boolean);
        case EXPR_ADD_ID:
            return r;
        case EXPR_ADD_INV: {
            struct expr *l = expr_create_integer_literal(0);
            EVAL_BIN_OP(-, int, integer);
        }
        case EXPR_EXP: {
            struct expr *to_return = expr_create_integer_literal(
                // implicitly casting from int to float to int
                pow(l->data->int_data, r->data->int_data)
            );
            expr_delete(l);
            expr_delete(r);
            return to_return;
        }
        case EXPR_ARR_ACC: {
            int idx = r->data->int_data;
            struct expr *ele = l->data->arr_elements;
            // If idx is out of range, behavior is already undefined,
            // so just truncating to last element is easiest here.
            for( int i = 0; i < idx && ele; i++ ){
                ele = ele->next;
            }
            struct expr *to_return = expr_copy(ele);
            expr_delete(l);
            expr_delete(r);
            return to_return;
        }
        default: {
            printf("Unhandled expr kind in expr_eval_const: %d. Aborting...\n", e->kind);
            abort();
        }
    }
}

void expr_print_type_and_expr(struct type *t, struct expr *e){
    type_print(t);
    printf(" (`");
    expr_print(e);
    printf("`)");
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


