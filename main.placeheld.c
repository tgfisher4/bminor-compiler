#include <stdio.h>
#include "token.h"
#include "decl.h"
#include "scope.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

typedef enum yytokentype token_t;

extern FILE *yyin;
extern int   yylex();
extern char *yytext;
extern int   last_int_literal;
extern char  last_char_literal;
extern char *last_string_literal;
extern struct decl *ast;


extern int yyparse();

char *indent_space(int indents);
int scan_file(char *filename, bool verbose);
int parse_file(char *filename);
void print_ast(struct decl *ast);
int resolve_ast(struct decl *ast, bool verbose);
void typecheck_ast(struct decl *ast);
void process_cl_args(int argc, char** argv, bool* stages, char** to_compile);

/* stages */
int SCAN      = 0,
    PARSE     = 1,
    PPRINT    = 2,
    RESOLVE   = 3,
    TYPECHECK = 4;
    CODEGEN   = 5;

int   typecheck_errors;

void usage(int return_code, char *called_as){
    printf(
"usage: %s [options]\n"
"\n"
"Options:\n"
"   -scan <file>    Scans <file> and outputs tokens encountered\n"
"   -parse <file>   Scans <file> quietly and reports whether parse was successful\n"
"   -print <file>   Scans and parses <file> quietly and outputs a nicely formatted version of the bminor program <file>\n"
"   -resolve <file> Scans, parses, and builds AST for program <file> quietly, then resolves all variable references\n"
"   -typecheck <file> Scans, parses, and builds AST for program <file> queitly, then resolves and typechecks\n"
"   -codegen <file> <output> Scans, parses, built AST for program <file>, resolves and typechecks, and generates assembly program <output>\n"
            , called_as);
    exit(return_code);
}

int main(int argc, char **argv){
    // default values
    bool stages[] = {false, false, false, false, false};
    char *to_compile  = "";
    char *output_file = "";

    bool run_all = true;

    /* process CL args */
    process_cl_args(argc, argv, stages, &to_compile);

    for(int i = 0; i < 4; i++)      run_all = run_all && !stages[i];

    /* scan */
    if (scan_file(to_compile, stages[SCAN])){
        puts("Scan unsuccessful");
        return EXIT_FAILURE;
    }
    else if (stages[SCAN])
        puts("Scan successful");

    /* parse */
    if (stages[PARSE] || stages[PPRINT] || stages[RESOLVE] || stages[TYPECHECK]) {
        if (parse_file(to_compile)) {
            puts("Parse unsuccessful");
            return EXIT_FAILURE;
        }
        else if (stages[PARSE])
            puts("Parse successful");
    }
    
    /* print */
    if (stages[PPRINT]) { print_ast(ast); puts(""); }

    /* resolve */
    // if resolve or typecheck or...
    if( stages[RESOLVE] || stages[TYPECHECK] ){
        int err_count = resolve_ast(ast, stages[RESOLVE]);
        if( stages[RESOLVE] ) puts("");
        if(err_count){
            printf("Encountered %d name resolution error%s\n", err_count, err_count == 1 ? "" : "s");
            puts("Name resolution unsuccessful");
            return EXIT_FAILURE;
        }
        else if(stages[RESOLVE]){
            puts("Name resolution successful");
        }
    }

    /* typecheck */ 
    if( stages[TYPECHECK] || stages[CODEGEN] ){
        typecheck_ast(ast);
        if(typecheck_errors){
            printf("\nEncountered %d typechecking error%s\n",
                    typecheck_errors,
                    typecheck_errors == 1 ? "" : "s");
            puts("Typecheck unsuccessful");
            return EXIT_FAILURE;
        }
        else if(stages[TYPECHECK]){
            puts("Typecheck successful");
        }
    }

    /* codegen */
    if( stages[CODEGEN] ){
        gen_ast_code(ast);
        puts("Code generation successful."); 
    }

    return EXIT_SUCCESS;
}

void process_cl_args(int argc, char **argv, bool *stages, char **to_compile, char **output_file){ 

    for (int i = 1; i < argc; i++){
        if (!strcmp("-scan", argv[i])){
             stages[SCAN] = true;
        }
        else if (!strcmp("-parse", argv[i])){
            stages[PARSE] = true;
        }
        else if (!strcmp("-print", argv[i])){
            stages[PPRINT] = true;
        }
        else if (!strcmp("-resolve", argv[i])){
            stages[RESOLVE] = true;
        }
        else if (!strcmp("-typecheck", argv[i])){
            stages[TYPECHECK] = true;
        }
        else if (!strcmp("-codegen", argv[i])){
            stages[CODEGEN] = true;
        }
        else if ( !strcmp("-help", argv[i]) || !strcmp("-h", argv[i]) ){
            usage(EXIT_SUCCESS, argv[0]);
        }
        else {
            // if we've already assigned the file to compile and the file to which to output
            if (**to_compile && **output_file)  usage(EXIT_FAILURE, argv[0]);
            // shouldn't need to worry about data pointed to going out of scope here since this isn't being assigned to something on the stack
            // first arg is the file to compile, second is file to which to output
            else if (*to_compile)               *to_compile  = argv[i];
            else                                *output_file = argv[i];
        }
    }
}

void print_ast(struct decl *ast){ decl_print_list(ast, 0, ";", "\n"); }

int resolve_ast(struct decl *ast, bool verbose){
    struct scope *sc = scope_enter(NULL);
    int err_count = decl_resolve(ast, sc, false, verbose);
    scope_exit(sc);
    return err_count;
}

void typecheck_ast(struct decl *ast){ decl_list_typecheck(ast); }

void gen_ast_code(struct decl *ast){ decl_codegen(ast, output_file); }

int parse_file(char *filename){
    yyin = fopen(filename, "r");
    if(!yyin) {
        printf("[ERROR|file] Could not open %s! %s\n", filename, strerror(errno));
        return 1;
    }
    // 0 for success, 1 for failure
    int to_return = yyparse();
    fclose(yyin);
    return to_return;
}

int scan_file(char *filename, bool verbose){
    /* Runs the flex-generated scanner on the file with name given by 'filename'
        - returns 1 on failure, 0 on success */

    /* An array of strings, where token_strs[<token>] = "<token name as str>", where <token> is a value of the enum token_t and <token name as str> is the symbolic name given to <token> in the enum token_t. Substituted by the Makefile via sed, ensuring the array is up to date with token.h */
    char* token_strs[] = <token_str_arr_placeholder>;

    yyin = fopen(filename, "r");
    token_t t = TOKEN_EOF;
    if(!yyin) {
        printf("[ERROR|file] Could not open %s! %s\n", filename, strerror(errno));
        return 1;
    }
    do {
        t = yylex();
        int t_str_idx = t - TOKEN_EOF;
        if (verbose) {
            switch(t){
                case SCAN_ERR:
                    fprintf(stdout, "[ERROR|scan] Invalid token: %s\n", yytext);
                    break;
                case INTERNAL_ERR:
                    fprintf(stdout, "[ERROR|internal] Internal error: %s (sorry!)\n", strerror(errno));
                    break;
                case IDENT:
                    printf("%s %s\n", token_strs[t_str_idx], yytext);
                    break;
                case STR_LIT:
                    printf("%s %s\n", token_strs[t_str_idx], last_string_literal);
                    break;
                case INT_LIT:
                    printf("%s %d\n", token_strs[t_str_idx], last_int_literal);
                    break;
                case CHAR_LIT:
                    printf("%s %c\n", token_strs[t_str_idx], last_char_literal);
                    break;
                default:
                    printf("%s\n", token_strs[t_str_idx]);
                    break;
            }
        } 
        if( t == STR_LIT ) free(last_string_literal);
    } while( !(t == TOKEN_EOF || t == SCAN_ERR || t == INTERNAL_ERR) );
    fclose(yyin);
    return t != TOKEN_EOF;
}

void indent(int indents){
    for(int i = 0; i < indents; i++) fputs("\t", stdout);
}
