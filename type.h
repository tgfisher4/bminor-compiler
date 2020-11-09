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
    bool        lvalue;
    // used to check whether an expression can be used as an array size
    //  - in bminor, array sizes must be computable at compile time (cannot use variables)
    //  - like above, but even in bminor this might require an extensive search, so we'll use a bool for efficiency
    bool        const_value;
};

struct type * type_create( type_t kind, struct type *subtype, struct expr *arr_sz, struct decl *params );
struct type * type_create_atomic( type_t kind, bool lvalue, bool const_value);
struct type * type_copy(struct type *t);
void          type_print( struct type *t );

#endif
