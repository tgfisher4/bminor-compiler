#ifndef SYMBOL_H
#define SYMBOL_H

#include "type.h"
#include <stdbool.h>

typedef enum {
	SYMBOL_LOCAL,
	SYMBOL_PARAM,
	SYMBOL_GLOBAL
} symbol_t;

struct symbol {
	symbol_t     kind;
	struct type *type;
	char        *name;
	int          stack_idx;
    bool         func_defined;
    int          reg; // why is this here again?
};

struct symbol * symbol_create( symbol_t kind, struct type *type, char *name, bool func_defined );

void symbol_print(struct symbol *sym);

void symbol_delete(struct symbol *sym);

#endif
