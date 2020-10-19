#ifndef EXPR_H
#define EXPR_H

//#include "symbol.h"
#include <stdbool.h>

typedef enum {
    EXPR_EMPTY,
    EXPR_ASGN,
    EXPR_OR,
    EXPR_AND,
    EXPR_LT,
    EXPR_LT_EQ,
    EXPR_GT,
    EXPR_GT_EQ,
	EXPR_EQ,
	EXPR_NOT_EQ,
    EXPR_ADD,
    EXPR_SUB,
	EXPR_MUL,
	EXPR_DIV,
    EXPR_MOD,
    EXPR_EXP,
    EXPR_NOT,
    EXPR_ADD_ID,
    EXPR_ADD_INV,
    EXPR_POST_INC,
    EXPR_POST_DEC,
    EXPR_ARR_ACC,
    EXPR_ARR_LIT,
    EXPR_FUNC_CALL,
    EXPR_IDENT,
    EXPR_INT_LIT,
    EXPR_STR_LIT,
    EXPR_CHAR_LIT,
    EXPR_BOOL_LIT
} expr_t;

union expr_data {
    /* mutually exclusive data fields involved in an expression
     * allows the access of different fields by intuitive names
     *  - ident_name: identifier name
     *  - str_literal: string literal data
     *  - int_literal: integer literal data
     *  - char_literal: char literal data
     *  - bool_literal: boolean literal data
     *  - arr_literal: array literal elements as a linked list
     *  - func_and_args: function arguments as a linked list. Head of list is the expression resulting in the function being called
    */
    const char *ident_name;
    const char *str_data;
    int   int_data;
    char  char_data;
    bool  bool_data;
    struct expr *arr_elements;
    struct expr *func_and_args;
    struct expr *operator_args;
};

struct expr {
	/* used by all kinds of exprs */
	expr_t kind;
    // allow expression/operator to have an arbitrary number of arguments
    //struct expr *args;
	//struct expr *left_arg;
	//struct expr *right_arg;

	/* used by various leaf exprs */
	//const char *name;
	//int literal_value;
	//const char * string_literal;
    union expr_data *data;
	struct symbol *symbol;
    struct expr *next;
};

struct expr * expr_create( expr_t kind, union expr_data *data);

struct expr * expr_create_oper( expr_t kind, struct expr *left, struct expr *right );
struct expr * expr_create_identifier( const char *ident );
struct expr * expr_create_integer_literal( int c );
struct expr * expr_create_boolean_literal( bool b );
struct expr * expr_create_char_literal( char c );
struct expr * expr_create_string_literal( const char *str );
struct expr * expr_create_array_literal( struct expr *expr_list );
struct expr * expr_create_array_access( struct expr *array, struct expr *index );
struct expr * expr_create_function_call( struct expr *function, struct expr *arg_list );
struct expr * expr_create_empty();

void expr_print( struct expr *e );
void expr_print_list( struct expr *e, char *delim);

#endif