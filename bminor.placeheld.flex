%{
    /* Preamble */
    #include "token.h"
    #include <stdbool.h>

    char *clean_string(char *string, char skip);
    
    int   last_int_literal;
    char  last_char_literal;
    char *last_string_literal;
%}

%option nounput
%option noinput

 /* DEFINITIONS */
 /* interesting regexs placed here along with custom classes since the rules section is necessarily cluttered */
DIGIT       [0-9]
LOWER       [a-z]
UPPER       [A-Z]
LETTER      {LOWER}|{UPPER}
WHITESPACE  [ \r\n\t]
 /* some inspiration was taken from the flex manual to come up with this regex */
C_COMMENT   "/*"([^*]|"*"[^/])*"*"+"/"
C_COMMENT_UNTERM "/*"
CPP_COMMENT "//".*
IDENT       ({LETTER}|_)({LETTER}|{DIGIT}|_)*
STRING_LIT  \"([^\n"\\]|\\(.|\n))*\"
INT_LIT     {DIGIT}+
 /* INT_LIT_SIGN     ("+"|"-")?{DIGIT}+ */
CHAR_LIT    '([^'\n\\]|\\.)'

%%
 /* RULES */
{WHITESPACE}                /* consume whitespace */
{C_COMMENT}|{CPP_COMMENT}   /* consume comment */
 /* the following pattern will only be matched if we could not match C_COMMENT */
{C_COMMENT_UNTERM}          { return SCAN_ERR; }
<<EOF>>                     { return TOKEN_EOF; }

 /* keywords */
 /* Shell script generates all keyword regexs from a file (keywords.txt) - placeholder substituted in makefile via sed */
<keyword_rules_placeholder>

{IDENT}                     { return yyleng <= 256 ? IDENT : SCAN_ERR; }
{STRING_LIT}                {
                            last_string_literal = clean_string(yytext, '"');
                            if (!last_string_literal)   return INTERNAL_ERR;
                            if( strlen(last_string_literal) < 256 )  return STR_LIT;
                            free(last_string_literal);
                            return SCAN_ERR;
                            }
{INT_LIT}                   {
                            last_int_literal = atoi(yytext);
                            return INT_LIT;
                            }
{CHAR_LIT}                  {
                            char *char_string = clean_string(yytext, '\'');
                            if (!char_string)   return INTERNAL_ERR;
                            last_char_literal = *char_string; // ch literal is first char of char_string
                            free(char_string); // no longer need char_string
                            return CHAR_LIT;
                            }

 /* literal tokens (i.e. uninteresting regex's containing only concatenation) */
 /* A shell script generates all literal token regexs from a file (literal_tokens.txt) - placeholder substituted in makefile via sed */
<literal_token_rules_placeholder>

.                           { return SCAN_ERR; }


%%

/* additional code */

int yywrap(){ return 1; }

char *clean_string(char *string, char delim){
    /*  Returns a malloc'd string that copies 'string', with the following exceptions:
            - escape sequences '\n' and '\0' made from consecutive characters are replaced with their escape sequence characters
            - the character 'delim' is not present without a preceding backslash
        Does not modify 'string'. Returns NULL is malloc fails. */ 

    // need at most strlen(yytext) + 1 (\0) - 2 (skip delimeters) bytes
    char *to_return = malloc(strlen(yytext)-1); 
    if (!to_return){
        puts("[ERROR|internal] Failed to allocate space for new string in clean_string. Exiting...");
        exit(EXIT_FAILURE);
    }

    // writer holds the next char in to_return to write to
    char *writer = to_return;
    for (char *reader = string; *reader; ++reader, ++writer){
        if (*reader == '\\'){
            switch(*(++reader)){
                case 'n':           *writer = '\n';    break;
                case '0':           *writer = '\0';    break;
                default:            *writer = *reader; break;
            }
        }
        else if (*reader == delim)   writer--;   // keeps write in place, skipping 'delim'
        else                        *writer = *reader;
    }
    *writer = '\0'; 

    return to_return;
}

