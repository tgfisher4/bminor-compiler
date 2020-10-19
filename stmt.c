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

void stmt_print(struct stmt *s, int indents, bool indent_first){
    if (!s) return;

    // somewhat sloppy solution but gets the job done for if-else and bracket on same line
    if (indent_first) indent(indents);

    //bool indent_next = true;

    switch(s->kind){
        case STMT_DECL:
            decl_print(s->decl, 0, ";");
            break;
        case STMT_EXPR:
            expr_print(s->expr_list);
            fputs(";", stdout);
            break;
        case STMT_IF_ELSE:
            fputs("if ( ", stdout);
            expr_print(s->expr_list);
            fputs(" )", stdout);
            bool body_is_not_block = !(s->body->kind == STMT_BLOCK);
            fputs(body_is_not_block ? "\n" : " " , stdout);
            stmt_print(s->body, indents + body_is_not_block, body_is_not_block);
            if (s->body->next) {
                fputs("\n", stdout);
                indent(indents);
                fputs("else", stdout);
                bool body_is_not_block_or_if = !(s->body->next->kind == STMT_BLOCK || s->body->next->kind == STMT_IF_ELSE);
                fputs(body_is_not_block_or_if ? "\n" : " " , stdout);
                stmt_print(s->body->next, indents + body_is_not_block_or_if, body_is_not_block_or_if);
            }
            break;
        case STMT_FOR:
            fputs("for ( ", stdout);
            expr_print_list(s->expr_list, " ; ");
            fputs(" )", stdout);
            body_is_not_block = !(s->body->kind == STMT_BLOCK);
            fputs(body_is_not_block ? "\n" : " " , stdout);
            stmt_print(s->body, indents + body_is_not_block, body_is_not_block);
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

    stmt_print(s, indents, true);
    if (s->next) fputs(delim, stdout);
    stmt_print_list(s->next, indents, delim);
}
