#include "stmt.h"
#include "scope.h"
#include <stdlib.h>
#include <stdio.h>

extern void indent(int indents);

struct stmt * stmt_create( stmt_t kind, struct decl *decl, struct expr *expr_list, struct stmt *body){
    struct stmt *s = malloc(sizeof(*s));
    if (!s){
        puts("Failed to allocate space for stmt, exiting...");
        exit(EXIT_FAILURE);
    }

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
            fputs("if( ", stdout);
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
            fputs("for( ", stdout);
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

int stmt_resolve(struct stmt *s, struct scope *sc, bool verbose){
    if( !s ) return 0;
    int err_count = 0;
    //printf("stmt: received scope with %d params\n", sc->params);
    err_count += decl_resolve(s->decl, sc, false, verbose);
    // resolve if cond, print params, for params, general expression
    err_count += expr_resolve(s->expr_list, sc, verbose);
    // resolve inner scope if block
    //static int i = 0;
    //i++;
    //struct scope *p = sc;
    //if( i == 3 ) printf("scope being nested contains argv? %d\n", (bool) scope_lookup(sc, "argv", true));
    sc = s->kind == STMT_BLOCK ? scope_enter(sc) : sc;
    //if( i == 3 ) printf("nested scope contains argv? %d\n", (bool) scope_lookup(sc, "argv", false));
    //printf("inner scope next is og scope? %d\n", p == sc->next);
    //if( i == 3 ) printf("nested scope next contains argv? %d\n", (bool) scope_lookup(sc->next, "argv", true));
    //struct scope *inner_sc = scope_enter(sc);
    //printf("stmt: resolving body %d\n", i);
    //err_count += stmt_resolve(s->body, inner_sc, verbose);
    err_count += stmt_resolve(s->body, sc, verbose);
    sc = s->kind == STMT_BLOCK ? scope_exit(sc) : sc;
    //printf("stmt: body resolved %d\n", i);
    //printf("stmt %d has %d errors\n", s->kind, err_count);

    err_count += stmt_resolve(s->next, sc, verbose);
    return err_count;
}
