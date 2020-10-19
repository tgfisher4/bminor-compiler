
#ifndef STMT_H
#define STMT_H

#include "decl.h"

typedef enum {
	STMT_DECL,
	STMT_EXPR,
	STMT_IF_ELSE,
	STMT_FOR,
	STMT_PRINT,
	STMT_RETURN,
	STMT_BLOCK
} stmt_t;

struct stmt {
	stmt_t kind;
	struct decl *decl;
    // used by for (3) [take care no middle is null], print (?), if (1)
    struct expr *expr_list;
	//struct expr *init_expr;
	//struct expr *expr;
	//struct expr *next_expr;
    // used by block (duh), and IF (if body; next ptr is the else body, if exists)
    struct stmt *body;
	//struct stmt *else_body;
	struct stmt *next;
};

struct stmt * stmt_create( stmt_t kind, struct decl *decl, struct expr *expr_list, struct stmt *body);
void stmt_print( struct stmt *s, int indents );
void stmt_print_list( struct stmt *s, int indents, char *delim );


#endif
