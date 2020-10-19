#include "expr.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* internal helpers */
int oper_precedence(expr_t);
void expr_print_subexpr(struct expr *e, expr_t parent_oper);
void print_clean_str_lit(const char *s);
void print_clean_char_lit(char c);
char *clean_char(char c, char delim);

//struct expr * expr_create(expr_t expr_type, struct expr *args, struct expr* right_arg, union expr_data *data){
struct expr * expr_create(expr_t expr_type, union expr_data *data){
    struct expr *e = malloc(sizeof(*e));
    if (!e) return NULL;

    e->kind = expr_type;
    //e->args = args;
    //e->left_arg  = left_arg;
    //e->right_arg = right_arg;
    // is this a good union application? have a data union holding int or str?
    e->data = data;
    /*
    if ( str_data || int_data ) e->data = malloc(sizeof(e->data));
    if (!e->data) return null;
    if (str_data) e->data->str_data = str_data;
    if (int_data) e->data->int_data = int_data;
    */
    e->next = NULL;
    e->symbol = NULL;

    return e;
}

struct expr * expr_create_oper( expr_t expr_type, struct expr *left_arg, struct expr* right_arg ){
    union expr_data *d = malloc(sizeof(*d));
    if (!d) return NULL;
    // if unary with empy right (!, +, -), create an empty placeholder
    if (!left_arg) left_arg = expr_create_empty();
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

char *oper_to_str(expr_t t){
    char *strs[] = {"=",
                    "||", "&&", "<", "<=", ">", ">=", "==", "!=",
                    "+", "-", "*", "/", "%", "^", "!",
                    "+", "-", "++", "--"};
    return (t < EXPR_ASGN || t > EXPR_POST_DEC) ? "" : strs[t - EXPR_ASGN];
}

void expr_print(struct expr *e){
    if (!e) return;

    switch(e->kind){
        case EXPR_EMPTY:
            //fputs("", stdout);
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
            //TODO: clean string for output (newline -> \n, " -> \", \ -> \\)
            // revrese clean string function from scanner
            print_clean_str_lit(e->data->str_data);
            //printf("\"%s\"", e->data->str_data);
            break;
        case EXPR_CHAR_LIT:
            //TODO: clean char for output
            print_clean_char_lit(e->data->char_data);
            //printf("%s", print_clean_literal(e->data->char_data));
            break;
        case EXPR_BOOL_LIT:
            fputs(e->data->bool_data ? "true" : "false", stdout);
            break;
        default:
            //operators
            /* this printing code can be made more elegant if you allow the AST to have empty nodes
             * However, I considered having an AST which represents exactly the program to run to be more desirable than more elegant printing code
            */
            expr_print_subexpr(e->data->operator_args, e->kind);
            fputs(oper_to_str(e->kind), stdout);
            expr_print_subexpr(e->data->operator_args->next, e->kind);
            /*
            // binary
            if (e->data->operator_args->next) {
                expr_print_subexpr(e->data->operator_args, e->kind);
                fputs(oper_to_str(e->kind), stdout);
                expr_print_subexpr(e->data->operator_args->next, e->kind);
            }
            // unary
            else{
                if (e->kind == EXPR_POST_DEC || e->kind == EXPR_POST_INC){
                    expr_print_subexpr(e->data->operator_args, e->kind);
                    fputs(oper_to_str(e->kind), stdout);
                } else {
                    fputs(oper_to_str(e->kind), stdout);
                    expr_print_subexpr(e->data->operator_args, e->kind);
                }
            }
            */
            break;
    }
}

void expr_print_subexpr(struct expr *e, expr_t parent_oper){ 
    // may be null: unary operators with empty right (represented by null instead of empty)
    if (!e || e->kind == EXPR_EMPTY) return;
    bool wrap_in_parens = parent_oper && oper_precedence(parent_oper) > oper_precedence(e->kind);
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

char *clean_char(char c, char delim){
    char *clean = malloc(3);
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

void print_clean_char_lit(char c){
    char clean[5] = "'";
    char *clean_c = clean_char(c, '\'');
    strcat(clean, clean_c);
    strcat(clean, "'");
    fputs(clean, stdout);
    fflush(stdout);
    free(clean_c);
}

void print_clean_str_lit(const char *s){
    // we'll need at most twice the characters, plus delimeters, plus nul
    char *clean = malloc(2 * strlen(s) + 3);
    if (!clean) return;
    clean[0] = '"';
    clean[1] = '\0';
    for (const char *reader = s; *reader; reader++){
        char *next = clean_char(*reader, '"');
        strcat(clean, next);
        free(next);
    }
    strcat(clean, "\"");
    fputs(clean, stdout);
    free(clean);
}

int oper_precedence(expr_t t){
    //{1, 2, 3, 4, 4, 4, 4, 4, 4, 5, 5, 
    switch(t){
        case EXPR_ASGN:
            return 1;
        case EXPR_OR:
            return 2;
        case EXPR_AND:
            return 3;
        case EXPR_LT:
        case EXPR_LT_EQ:
        case EXPR_GT:
        case EXPR_GT_EQ:
        case EXPR_EQ:
        case EXPR_NOT_EQ:
            return 4;
        case EXPR_ADD:
        case EXPR_SUB:
            return 5;
        case EXPR_MUL:
        case EXPR_DIV:
        case EXPR_MOD:
            return 6;
        case EXPR_EXP:
            return 7;
        case EXPR_NOT:
        case EXPR_ADD_ID:
        case EXPR_ADD_INV:
            return 8;
        case EXPR_POST_INC:
        case EXPR_POST_DEC:
            return 9;
        case EXPR_ARR_ACC:
        case EXPR_ARR_LIT:
        case EXPR_FUNC_CALL:
        case EXPR_IDENT:
        case EXPR_INT_LIT:
        case EXPR_STR_LIT:
        case EXPR_CHAR_LIT:
        case EXPR_BOOL_LIT:
            return 10;
        default:
            return 11;
    }
}
