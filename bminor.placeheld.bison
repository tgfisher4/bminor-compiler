%{
#include <stdio.h>
#include "decl.h"
#include "stmt.h"
#include "expr.h"
//#include "param_list.h"
#include "type.h"
#include <stdlib.h>
#include <string.h>

//#define YYSTYPE struct decl *

extern char *yytext;
extern char *last_string_literal;
extern char last_char_literal;
extern int last_int_literal;
extern int yylex();
extern int yyerror( char *str );
//extern char *clean_string(char *string, char delim);
struct decl *ast;

%}

%token TOKEN_EOF
<keywords_placeholder>
<literal_tokens_placeholder>
%token IDENT
%token STR_LIT
%token INT_LIT
%token CHAR_LIT
%token INTERNAL_ERR
%token SCAN_ERR

%union {
    struct decl *decl;
    struct stmt *stmt;
    struct expr *expr;
    //struct param_list *param_list;
    struct type *type;
    char *ident;
}

%type <decl>        program decl maybe_decls param_comma_list maybe_param_comma_list
%type <stmt>        stmt maybe_stmts non_right_recursive_stmt if_stmt for_stmt non_dangling_stmt non_dangling_if non_dangling_for
%type <expr>        expr maybe_expr expr_comma_list maybe_expr_comma_list expr1 expr2 expr3 expr4 expr5 expr6 expr7 expr8 expr9 expr10 func_call arr_lit atom
/* %type <param_list>  param_comma_list, maybe_param_comma_list, expr_comma_list, maybe_expr_comma_list */
%type <type>        type
%type <ident>       ident

%%

program : maybe_decls TOKEN_EOF
        { ast = $1; return 0; }
        ;

/* END PROGRAM ================================= BEGIN DECLARATIONS */
decl : ident COLON type S_COL
     { $$ = decl_create($1, $3, NULL, NULL); }
     | ident COLON type ASGN expr S_COL
     { $$ = decl_create($1, $3, $5, NULL); }
     | ident COLON type ASGN L_BRC maybe_stmts R_BRC
     { $$ = decl_create($1, $3, NULL, $6); }
     ;

type : INTEGER
     { $$ = type_create(TYPE_INTEGER, NULL, NULL, NULL); }
     | STRING
     { $$ = type_create(TYPE_STRING, NULL, NULL, NULL); }
     | CHAR
     { $$ = type_create(TYPE_CHAR, NULL, NULL, NULL); }
     | BOOLEAN
     { $$ = type_create(TYPE_BOOLEAN, NULL, NULL, NULL); }
     | VOID
     { $$ = type_create(TYPE_VOID, NULL, NULL, NULL); }
     | ARRAY L_BRK R_BRK type        /* infer length from initializer */
     { $$ = type_create(TYPE_ARRAY, $4, NULL, NULL); }
     | ARRAY L_BRK expr R_BRK type   /* we'll catch whether this expression is of a type for which we can generate code later on in the pipeline */
     { $$ = type_create(TYPE_ARRAY, $5, $3, NULL); }
     | FUNCTION type L_PAR maybe_param_comma_list R_PAR
     { $$ = type_create(TYPE_FUNCTION, $2, NULL, $4); }
     ;

maybe_param_comma_list : /* empty */
                       { $$ = NULL; }
                       | param_comma_list
                       { $$ = $1; }
                       ;

param_comma_list : ident COLON type 
                 { $$ = decl_create($1, $3, NULL, NULL); }
                 | ident COLON type COMMA param_comma_list
                 { $$ = decl_create($1, $3, NULL, NULL); $$->next = $5; }
                 ;

maybe_decls : /* empty*/
            { $$ = NULL; }
            | decl maybe_decls
            { $1->next = $2; $$ = $1; }
            ;

/* END DECLARATIONS ============================ BEGIN STATEMENTS */

/*
if and for statements need to be considered separately because they both end with statements
    - if-else is obviously vulnerable to dangling else ambiguities
    - once if-else solved, we need to be careful of for statements too. They end with a statement, meaning if we included them in a 'stmt' nonterminal and did nothing else, then it is possible that a for loop appears as the "then" body of an if-else statement and its 'stmt' might be a dangling if: then, we would have ambiguities again.

    - Note: drew inspriation from in-class dangling-if discussion and Java's solution (https://docs.oracle.com/javase/specs/jls/se9/html/jls-14.html). A statement can be anything we would think it would be. However, the "then" clause of an if-then statement can only contain a statement known not to end with a dangling if: this ensures that the "else" must match this "if" unambiguously. The statements known for sure not to end with a dangling if are those which do not end with another statement and those which do end with another statement, but that statement is known not to end with a dangling else.
*/

stmt : non_right_recursive_stmt
     { $$ = $1; }
     | if_stmt
     { $$ = $1; }
     | for_stmt
     { $$ = $1; }
     | decl
     { $$ = stmt_create(STMT_DECL, $1, NULL, NULL); }
     ;


non_right_recursive_stmt : expr S_COL
                         { $$ = stmt_create(STMT_EXPR, NULL, $1, NULL); }
                         | L_BRC maybe_stmts R_BRC
                         { $$ = stmt_create(STMT_BLOCK, NULL, NULL, $2); }
                         | PRINT maybe_expr_comma_list S_COL
                         { $$ = stmt_create(STMT_PRINT, NULL, $2, NULL); }
                         | RETURN maybe_expr S_COL
                         { $$ = stmt_create(STMT_RETURN, NULL, $2, NULL); }
                         ;


if_stmt : IF L_PAR expr R_PAR non_dangling_stmt ELSE stmt
        { $5->next = $7; $$ = stmt_create(STMT_IF_ELSE, NULL, $3, $5); }
        | IF L_PAR expr R_PAR stmt
        { $$ = stmt_create(STMT_IF_ELSE, NULL, $3, $5); }
        ;

non_dangling_if : IF L_PAR expr R_PAR non_dangling_stmt ELSE non_dangling_stmt
                { $5->next = $7; $$ = stmt_create(STMT_IF_ELSE, NULL, $3, $5); }
                ;


for_stmt : FOR L_PAR maybe_expr S_COL maybe_expr S_COL maybe_expr R_PAR stmt
         { if(!$3) $3 = expr_create_empty();
           if(!$5) $5 = expr_create_boolean_literal(true);
           if(!$7) $7 = expr_create_empty();

           $3->next = $5;
           $5->next = $7;

           $$ = stmt_create(STMT_FOR, NULL, $3, $9);
         }
         ;

non_dangling_for : FOR L_PAR maybe_expr S_COL maybe_expr S_COL maybe_expr R_PAR non_dangling_stmt
                 { if(!$3) $3 = expr_create_empty();
                   if(!$5) $5 = expr_create_boolean_literal(true);
                   if(!$7) $7 = expr_create_empty();

                   $3->next = $5;
                   $5->next = $7;

                   $$ = stmt_create(STMT_FOR, NULL, $3, $9);
                 }
                 ;


/* statement not ending with a dangling if. The only place we use this in a rule besides to define other non_dangling structures is the if-else "then" body */
/* note that a non_dangling_stmt cannot reduce to a stmt directly, so there is no ambiguity here */
non_dangling_stmt : non_right_recursive_stmt
                  { $$ = $1; }
                  | non_dangling_for
                  { $$ = $1; }
                  | non_dangling_if
                  { $$ = $1; }
                  ;


maybe_stmts : /* empty */
            { $$ = NULL; }
            | stmt maybe_stmts
            { $1->next = $2; $$ = $1; }
            ;

/* END STATEMENTS ============================ BEGIN EXPRESSIONS */

/* expression: can be treated as a single value */
expr : expr10
     { $$ = $1; }
     ;

/* assignment: = */
expr10: expr9 ASGN expr10
      { $$ = expr_create_oper(EXPR_ASGN, $1, $3); }
      | expr9
      { $$ = $1; }
      ;

/* logical or: || (lower precedence bc add in bool alg */
expr9 : expr9 OR expr8
      { $$ = expr_create_oper(EXPR_OR, $1, $3); }
      | expr8
      { $$ = $1; }
      ;
/* logical and: && (higher precedence bc mult in bool alg */
expr8 : expr8 AND expr7
      { $$ = expr_create_oper(EXPR_AND, $1, $3); }
      | expr7
      { $$ = $1; }
      ;

/* comparisons: < <= > >= == != */
expr7 : expr7 LT expr6
      { $$ = expr_create_oper(EXPR_LT, $1, $3); }
      | expr7 LT_EQ expr6
      { $$ = expr_create_oper(EXPR_LT_EQ, $1, $3); }
      | expr7 GT expr6
      { $$ = expr_create_oper(EXPR_GT, $1, $3); }
      | expr7 GT_EQ expr6
      { $$ = expr_create_oper(EXPR_GT_EQ, $1, $3); }
      | expr7 EQ expr6
      { $$ = expr_create_oper(EXPR_EQ, $1, $3); }
      | expr7 NOT_EQ expr6
      { $$ = expr_create_oper(EXPR_NOT_EQ, $1, $3); }
      | expr6
      { $$ = $1; }
      ;

/* binary add/sub: + - */
expr6 : expr6 PLUS expr5
      { $$ = expr_create_oper(EXPR_ADD, $1, $3); }
      | expr6 MINUS expr5
      { $$ = expr_create_oper(EXPR_SUB, $1, $3); }
      | expr5
      { $$ = $1; }
      ;

/* mult/div/mod: * / % */
expr5 : expr5 STAR expr4
      { $$ = expr_create_oper(EXPR_MUL, $1, $3); }
      | expr5 SLASH expr4
      { $$ = expr_create_oper(EXPR_DIV, $1, $3); }
      | expr5 PRCT expr4
      { $$ = expr_create_oper(EXPR_MOD, $1, $3); }
      | expr4
      { $$ = $1; }
      ;

/* exponentiaion: ^ */
expr4 : expr4 CARET expr3
      { $$ = expr_create_oper(EXPR_EXP, $1, $3); }
      | expr3
      { $$ = $1; }
      ;

/* unary operators: + - ! */
expr3 : PLUS expr3
      { $$ = expr_create_oper(EXPR_ADD_ID, NULL, $2); }
      | MINUS expr3
      { $$ = expr_create_oper(EXPR_ADD_INV, NULL, $2); }
      | NOT expr3
      { $$ = expr_create_oper(EXPR_NOT, NULL, $2); }
      | expr2
      { $$ = $1; }
      ;

/* unary postfix decrement/increment */
expr2 : expr2 DEC
      { $$ = expr_create_oper(EXPR_POST_DEC, $1, NULL); }
      | expr2 INC
      { $$ = expr_create_oper(EXPR_POST_INC, $1, NULL); }
      | expr1
      { $$ = $1; }
      ;

/* grouping: () [] f() */
expr1 : L_PAR expr R_PAR
      { $$ = $2; }
      | expr1 L_BRK expr R_BRK
      { $$ = expr_create_array_access($1, $3); }
      | func_call
      { $$ = $1; }
      | atom
      { $$ = $1; }
      ;

/* atom: lowest form of expression */
atom : ident
     { $$ = expr_create_identifier($1); }
     | STR_LIT
     { $$ = expr_create_string_literal(last_string_literal); }
     | INT_LIT
     { $$ = expr_create_integer_literal(last_int_literal); }
     | CHAR_LIT
     { $$ = expr_create_char_literal(last_char_literal); }
     | TRUE
     { $$ = expr_create_boolean_literal(true); }
     | FALSE
     { $$ = expr_create_boolean_literal(false); }
     | arr_lit /* should this be here or in decl? technically you can only have an array literal like this in a declaration, but I like the general idea of having an array literal that, when used, creates a temporary array for you to use in your expression. Either way, we can catch this during typechecking. */
     { $$ = $1; }
     ;

ident: IDENT
     { char *s = strdup(yytext);
       if (!s){
           fprintf(stdout, "[ERROR|internal] Failed to allocate space for duping identifier.\n");
           exit(EXIT_FAILURE);
       }
       $$ = s;
     }
     ;


/* function call: <function name>(<arg list>) */
/* use expr1 instead of ident at the head of this call so that I can have a function return a function, and then access that function */
func_call: expr1 L_PAR maybe_expr_comma_list R_PAR
         { $$ = expr_create_function_call($1, $3); }
         ;

/* array literal: {<value list>} - used to initialize arrays at declaration */
arr_lit : L_BRC expr_comma_list R_BRC
        { $$ = expr_create_array_literal($2); }
        ;

/* maybe comma separated list : comma list of empty */
maybe_expr_comma_list: /* empty */
                     { $$ = NULL; }
                     | expr_comma_list
                     { $$ = $1; }
                     ;

/* comma list: arbirary number of values separated by commas */
expr_comma_list: expr
               { $$ = $1; }
               | expr COMMA expr_comma_list
               { $1->next = $3; $$ = $1; }
               ;

maybe_expr : /* empty */
           { $$ = NULL; }
           | expr
           { $$ = $1; }
           ;

%%

int yyerror( char *str )
{
    printf("parse error: %s\n",str);
    return 0;
}

