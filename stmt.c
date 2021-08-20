#include "stmt.h"
#include "decl.h"
#include "scope.h"
#include <stdlib.h>
#include <stdio.h>

extern void indent(int indents);
extern int typecheck_errors;

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
    err_count += decl_resolve(s->decl, sc, false, verbose);
    // resolve if cond, print params, for params, general expression
    err_count += expr_resolve(s->expr_list, sc, verbose);
    // resolve inner scope if block
    sc = s->kind == STMT_BLOCK ? scope_enter(sc) : sc;
    err_count += stmt_resolve(s->body, sc, verbose);
    int nested_locals = s->kind == STMT_BLOCK ? sc->locals + sc->nested_locals: 0;
    sc = s->kind == STMT_BLOCK ? scope_exit(sc) : sc;
    sc->nested_locals += nested_locals;

    err_count += stmt_resolve(s->next, sc, verbose);
    return err_count;
}

void stmt_typecheck(struct stmt *s, struct decl *enc_func){
    if (!s) return;

    struct type *type;
    struct expr *expr;
    switch(s->kind){
        case STMT_DECL:
            decl_typecheck(s->decl, false);
            if( s->decl->type->kind == TYPE_FUNCTION ){
                // cannot declare function inside function
                printf("[ERROR|typecheck] You attempted to declare a function (`");
                decl_print_no_asgn(s->decl);
                printf("`) within a function (`");
                decl_print_no_asgn(enc_func);
                printf("`). Sorry, but BMinor does not currently support nested function definitions. That is, you cannot declare a function within another function.\n");
                typecheck_errors++;
            }
            break;
        case STMT_EXPR:
            type = expr_typecheck(s->expr_list);
            type_delete(type);
            break;
        case STMT_IF_ELSE:
            type = expr_typecheck(s->expr_list);
            if( type->kind != TYPE_BOOLEAN ){
                // can only check boolean condition
                printf("[ERROR|typecheck] You attempted to pass an expression of type ");
                expr_print_type_and_expr(type, s->expr_list);
                printf(" as the condition to an if statement. You must pass an expression resolving to a boolean as the condition, indiciating whether to execute the body of the if statement.\n");
                typecheck_errors++;
            }
            type_delete(type);
            break;
        case STMT_FOR:
            // check that our init and incremental expr's are cool
            type = expr_typecheck(s->expr_list);
            type_delete(type);
            type = expr_typecheck(s->expr_list->next->next);
            type_delete(type);

            type = expr_typecheck(s->expr_list->next);
            if( type->kind != TYPE_BOOLEAN ){
                // can only check boolean condition
                printf("[ERROR|typecheck] You attempted to pass an expression of type ");
                expr_print_type_and_expr(type, s->expr_list->next);
                printf(" as the continuation condition to a for statement. You must pass an expression resolving to a boolean as the continuation condition (or no expression at all, which is interpreted as the expression 'true'), indiciating whether to execute the body of the loop.\n");
                typecheck_errors++;
            }
            break;
        case STMT_PRINT:
            for( expr = s->expr_list; expr; expr = expr->next ){
                type = expr_typecheck(expr);
                if( type->kind == TYPE_ARRAY
                 || type->kind == TYPE_FUNCTION
                 || type->kind == TYPE_VOID ){
                    // cannot print array or function
                    printf("[ERROR|typecheck] You attempted to print an expression of type ");
                    expr_print_type_and_expr(type, expr);
                    printf(". You may only print expressions of atomic, non-void types (integer, char, string, boolean).\n");
                    typecheck_errors++;
                }
                type_delete(type);
            }
            break;
        case STMT_RETURN:
            type = expr_typecheck(s->expr_list);
            if( type && enc_func->type->subtype->kind == TYPE_VOID ){
                // void function cannot return value
                printf("[ERROR|typecheck] You attempted to return an expression of type ");
                expr_print_type_and_expr(type, s->expr_list);
                printf(" from a function which returns type ");
                type_print(enc_func->type->subtype);
                printf(" (");
                decl_print_no_asgn(enc_func);
                printf("). You may not return a value from a function of return type void, but the return statement may be used without an expression to force an early exit.\n");
                typecheck_errors++;
            }
            // null will not type_equal a struct type with subtype void: void functions have already passed their test
            else if( enc_func->type->subtype->kind != TYPE_VOID 
                     && !type_equals(type, enc_func->type->subtype) ){
                // type of expression returned does not match function return type
                printf("[ERROR|typecheck] You attempted to return an expression of type ");
                expr_print_type_and_expr(type, s->expr_list);
                printf(" from a function which returns type ");
                type_print(enc_func->type->subtype);
                printf(" (");
                decl_print_no_asgn(enc_func);
                printf("). You may only return an expression of the return type of the enclosing function.\n");
                typecheck_errors++;
            }
            break;
        case STMT_BLOCK:
            break;
        default:
            break;
    }

    // check body(ies) of if statements, for loops, blocks, etc.
    stmt_list_typecheck(s->body, enc_func);
}

void stmt_list_typecheck(struct stmt *s, struct decl *enc_func){
    if (!s) return;
    stmt_typecheck(s, enc_func);
    stmt_list_typecheck(s->next, enc_func);
}

void stmt_code_gen(FILE *output, struct stmt *s, char *enc_func){
    if( !s ) return;
    switch( s->kind ){
        case STMT_DECL:
            decl_code_gen(s->decl, false);
            break;
        case STMT_EXPR:
            expr_code_gen(s->expr_list);
            scratch_free(s->expr_list->reg);
            break;
        case STMT_IF_ELSE:
            expr_code_gen(e->expr_list);
            fprintf(output, "CMP %r%s, $0\n",
                    scratch_name(e->expr_list));
            scratch_free(e->expr_list->reg);
            char *else_label = label_create();
            fprintf(output, "JE %s\n", else_label);
            stmt_code_gen(s->body);
            fprintf(output, "%s:\n", else_label);
            if( s->next ){ // Generate else block if present
                stmt_code_gen(s->next);
            }
            break;
        case STMT_FOR:
            char *test_label = label_create();
            char *exit_label = label_create();
            // Initializer
            expr_code_gen(s->expr_list);
            scratch_free(s->expr_list->reg); // discard result: only side effects relevant
            // Test condition
            fprintf(output, "%s:\n", test_label);
            expr_code_gen(s->expr_list->next);
            fprintf(output, "CMP %r%s, $0\n"
                            "JE %s\n,
                    scratch_name(s->expr_list->next->reg),
                    exit_label);
            scratch_free(s->expr_list->next->reg);
            // Loop body
            stmt_code_gen(s->body);
            // Step
            expr_code_gen(s->expr_list->next->next);
            scratch_free(s->expr_list->next->next->reg); // discard result: only side effects relevant
            // Exit label
            fprintf(output, "%s:\n", exit_label);
            break;
        case STMT_PRINT:
            for( struct expr *e = s->expr_list; e; e = e->next ){
                // TODO: figure out runtime stuff
                // Transform each element into a call to the runtime print funcs
                struct type *t = expr_typecheck(e);
                // 6 = len(print_),
                // 7 = max(len(integer), len(char), len(string), len(boolean)
                // 1 = \0
                char *func_name[6 + 7 + 1];
                sprintf(func_name, "print_%s", type_t_to_str(type_str));
                struct expr *arg = expr_copy(e);
                arg->next = NULL; // o/w, the print_call will seem to include several args
                struct expr *print_call = expr_create_function_call(
                    expr_create_identifier(func_name),
                    arg
                );
                expr_code_gen(print_call);
                scratch_free(print_call);
                expr_delete(print_call); // also deletes arg
                type_delete(t);
            }
            break;
        case STMT_RETURN:
            expr_code_gen(s->expr_list);
            fprintf(output, "MOVQ %r%s, %rax\n"
                            "JMP %s_postamble,
                    scratch_name(s->expr_list->reg),
                    enc_func);
            scratch_free(s->expr_list->reg);
            break;
        case STMT_BLOCK:
            stmt_list_code_gen(s->body);
            break;
        default:
            printf("Unexpected stmt kind: %d. Aborting...\n", s->kind);
            abort();
    }
}

void stmt_list_code_gen(FILE *output, struct stmt *s, char *enc_func){
    if( !s ) return;
    stmt_code_gen(output, s, enc_func);
    stmt_list_code_gen(output, s->next, enc_func);
}
