
#ifndef DECL_H
#define DECL_H

#include "type.h"
#include "stmt.h"
#include "expr.h"
#include "symbol.h"

struct decl {
	char          *ident;
	struct type   *type;
	struct expr   *init_value;
	struct stmt   *func_body;
	struct symbol *symbol;
	struct decl   *next;
};

struct decl * decl_create( char *name, struct type *type, struct expr *init_value, struct stmt *func_body);
void decl_print( struct decl *d, int indents, char* term );
void decl_print_list( struct decl *d, int indents, char* term, char *delim );

struct scope;
int  decl_resolve( struct decl *d, struct scope *sc, bool am_param, bool verbose);

#endif


