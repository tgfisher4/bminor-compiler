#include "stmt.h"
#include <stdlib.h>
#include <stdio.h>

extern void indent(int indents);

struct stmt * stmt_create( stmt_t kind, struct decl *decl, struct expr *expr_list, struct stmt *body){
    struct stmt *s = malloc(sizeof(*s));
    if (!s) return NULL;

    s->kind = kind;
    s->decl = decl;
    s->expr_list = expr_list;
    s->body = body;
    s->next = NULL;

    return s;
}

void stmt_print(struct stmt *s, int indents){
    if (!s) return;

    indent(indents);

    switch(s->kind){
        case STMT_DECL:
            decl_print(s->decl, 0, ';');
            break;
        case STMT_EXPR:
            expr_print(s->expr_list);
            fputs(";", stdout);
            break;
        case STMT_IF_ELSE:
            fputs("if (", stdout);
            expr_print(s->expr_list);
            fputs(")", stdout);
            //if (s->body->kind != STMT_BLOCK)
                fputs("\n", stdout);
            stmt_print(s->body, indents + (s->body->kind != STMT_BLOCK));
            if (s->body->next) {
                fputs("\n", stdout);
                indent(indents);
                fputs("else ", stdout);
                //if (s->body->next->kind != STMT_BLOCK)
                    fputs("\n", stdout);
                stmt_print(s->body->next, indents + (s->body->next->kind != STMT_BLOCK));
            }
            break;
        case STMT_FOR:
            fputs("for (", stdout);
            expr_print_list(s->expr_list, "; ");
            /*
            fputs("; ", stdout);
            expr_print(s->expr_list->next, false);
            fputs("; ", stdout);
            expr_print(s->expr_list->next->next, false);
            */
            fputs(")", stdout);
            //if (s->body->kind != STMT_BLOCK)
                fputs("\n", stdout);
            stmt_print(s->body, indents + s->body->kind != STMT_BLOCK);
            break;
        case STMT_PRINT:
            fputs("print ", stdout);
            expr_print_list(s->expr_list, ", ");
            fputs(";", stdout);
            break;
        case STMT_RETURN:
            fputs("return ", stdout);
            expr_print(s->expr_list);
            fputs(";", stdout);
            break;
        case STMT_BLOCK:
            fputs("{\n", stdout);
            stmt_print_list(s->body, indents + 1, "\n");
            fputs("\n", stdout);
            indent(indents);
            fputs("}", stdout);
            break;
        default:
            break;
    }
}

void stmt_print_list(struct stmt *s, int indents, char *delim){
    if (!s) return;

    stmt_print(s, indents);
    if (s->next) fputs(delim, stdout);
    stmt_print_list(s->next, indents, delim);
}
