#ifndef SCOPE_H
#define SCOPE_H

#include "symbol.h"
#include "hash_table.h"
#include <stdbool.h>

struct scope {
    struct  hash_table *table;
    struct  scope *next;
    int     locals;
    int     params;
    int     nested_locals;
};

struct symbol *scope_bind(struct scope *sc, const char *name, struct symbol *sym);
struct symbol *scope_lookup(struct scope *sc, const char *name, bool only_curr);

struct scope *scope_enter(struct scope *sc);
struct scope *scope_exit(struct scope *sc);
bool          scope_is_global(struct scope *sc);

#endif
