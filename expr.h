#ifndef EXPR_H
#define EXPR_H

//#include "symbol.h"
#include <stdbool.h>

typedef enum {
    /* EXPR_NAME       @ precedence @   commutative? @  associativity @   string  @ */
    EXPR_EMPTY,     // @    11      @                @                @           @
    EXPR_ASGN,      // @    1       @   no           @   right        @   " = "   @
    EXPR_OR,        // @    2       @   yes          @   left         @   " || "  @
    EXPR_AND,       // @    3       @   yes          @   left         @   " && "  @
    EXPR_LT,        // @    4       @   yes          @   left         @   " < "   @
    EXPR_LT_EQ,     // @    4       @   yes          @   left         @   " <= "  @
    EXPR_GT,        // @    4       @   yes          @   left         @   " > "   @
    EXPR_GT_EQ,     // @    4       @   yes          @   left         @   " >= "  @
	EXPR_EQ,        // @    4       @   yes          @   left         @   " == "  @
	EXPR_NOT_EQ,    // @    4       @   yes          @   left         @   " != "  @
    EXPR_ADD,       // @    5       @   yes          @   left         @   " + "   @
    EXPR_SUB,       // @    5       @   no           @   left         @   " - "   @  
	EXPR_MUL,       // @    6       @   yes          @   left         @   " * "   @
	EXPR_DIV,       // @    6       @   no           @   left         @   " / "   @ 
    EXPR_MOD,       // @    6       @   no           @   left         @   " % "   @ 
    EXPR_EXP,       // @    7       @   no           @   left         @   " ^ "   @ 
    EXPR_NOT,       // @    8       @                @                @   "!"     @  
    EXPR_ADD_ID,    // @    8       @                @                @   "+"     @
    EXPR_ADD_INV,   // @    8       @                @                @   "-"     @
    EXPR_POST_INC,  // @    9       @                @                @   "++"    @
    EXPR_POST_DEC,  // @    9       @                @                @   "--"    @
    EXPR_ARR_ACC,   // @    10      @                @                @           @ 
    EXPR_ARR_LIT,   // @    10      @                @                @           @
    EXPR_FUNC_CALL, // @    10      @                @                @           @
    EXPR_IDENT,     // @    10      @                @                @           @
    EXPR_INT_LIT,   // @    10      @                @                @           @
    EXPR_STR_LIT,   // @    10      @                @                @           @
    EXPR_CHAR_LIT,  // @    10      @                @                @           @
    EXPR_BOOL_LIT   // @    10      @                @                @           @
} expr_t;

union expr_data {
    /* mutually exclusive data fields involved in an expression
     * allows the access of different fields by intuitive names
     *  - ident_name: identifier name
     *  - str_data: string literal data
     *  - int_data: integer literal data
     *  - char_data: char literal data
     *  - bool_data: boolean literal data
     *  - arr_elements: array literal elements as a linked list
     *  - func_and_args: function arguments as a linked list. Head of list is the expression resulting in the function being called
     *  - operator_args: operator arguments as a linked list. Each has exactly two nodes. One of the two unary operator's nodes is empty (depending on which side of the operator the argument appears (! vs ++)
     *  Once I decided to use a union I might as well define fields of the same type that have more intuitive names: I never store anything extra in my union as a reuslt (which was my qualm with the struct with many NULL fields: felt like it could be simplified)
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
