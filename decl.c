#include "decl.h"
#include <stdio.h>
#include <stdlib.h>

extern void indent(int indents);

struct decl * decl_create(char *ident, struct type *type, struct expr *init_value, struct stmt *func_body){
    struct decl *d = malloc(sizeof(*d));
    if(!d) { return NULL; }

    d->ident         = ident;
    d->type          = type;
    d->init_value    = init_value;
    d->func_body     = func_body;
    d->next          = NULL;
    d->symbol        = NULL;

    return d;
}

void decl_print(struct decl *d, int indents, char term){
    if (!d) return;

    indent(indents);
    printf("%s: ", d->ident);
    type_print(d->type);

    if (d->init_value){
        fputs(" = ", stdout);
        expr_print(d->init_value);
    }

    if (d->func_body){
        fputs(" = {\n", stdout);
        stmt_print_list(d->func_body, indents + 1, "\n");
        indent(indents);
        fputs("\n}", stdout);
    } else printf("%c", term);
}

void decl_print_list(struct decl *d, int indents, char term, char *delim){
    if (!d) return;
    decl_print(d, indents, term);
    if (d->next) fputs(delim, stdout);
    decl_print_list(d->next, indents, term, delim);
}
