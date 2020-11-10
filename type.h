#ifndef TYPE_H
#define TYPE_H

#include "decl.h"
#include "expr.h"

typedef enum {
	TYPE_VOID,
	TYPE_BOOLEAN,
	TYPE_CHAR,
	TYPE_INTEGER,
	TYPE_STRING,
	TYPE_ARRAY,
	TYPE_FUNCTION,
} type_t;

struct type {
	type_t      kind;
	struct decl *params;
	struct type *subtype;
    struct expr *arr_sz;
    // used to check whether we can be assigned to
    //  - could also search down AST to find if value being assigned is lvalue
    //      - won't have to search far in bminor since only 2 lvalues
    //      - in a more complex language, this would seem to add significant complexity; having this lvalue bool seems the more generalizable approach
    bool        is_lvalue;
    // used to check whether an expression can be used as an array size
    //  - in bminor, array sizes must be computable at compile time (cannot use variables)
    //  - like above, but even in bminor this might require an extensive search, so we'll use a bool for efficiency
    bool        is_const_value;
};

struct type * type_create_full( type_t kind, struct type *subtype, struct expr *arr_sz, struct decl *params, bool is_lvalue, bool is_const_value);
struct type * type_create( type_t kind, struct type *subtype, struct expr *arr_sz, struct decl *params );
struct type * type_create_atomic( type_t kind, bool is_lvalue, bool is_const_value);
struct type * type_copy( struct type *t );
void          type_print( struct type *t );
char         *type_t_to_str( type_t kind );

bool type_t_equals(type_t t, type_t s);
bool type_equals(struct type *t, struct type *s);
void type_typecheck(struct type *t, struct decl *context);
void type_delete(struct type *t);

#endif
