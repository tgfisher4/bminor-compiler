CC 		= gcc
CFLAGS 	= -g -Wall -std=gnu99
LD 		= gcc
LDFLAGS = 
LEX	= flex
LEXFLAGS = 
YACC  = bison
YACCFLAGS = --verbose
SED = $(shell gsed --version &>/dev/null 2>&1 && echo "gsed" || echo "sed")

AST_COMP = expr.o decl.o stmt.o type.o
NAME_RES = scope.o symbol.o hash_table.o
CODE_GEN = code_gen_utils.o

TARGETS = bminor

all:		$(TARGETS)

test: bminor
	./run_all_tests.sh

clean:
	@echo Cleaning...
	@rm -f $(TARGETS)
	@rm -f token.h
	@rm -f *.o
	@rm -f bminor.c bminor.$(LEX) bminor.$(YACC) bminor_parse.c bminor_scan.c
	@rm -f main.c expr.c type.c
	@rm -f bminor_parse.output
	@rm -f *_tests/*_tests/*.out
	@rm -f valgrind-out.txt

bminor: 		    main.o bminor_scan.o bminor_parse.o $(AST_COMP) $(NAME_RES) $(CODE_GEN) token.h
	@echo "Linking bminor..."
	$(LD) $(LDFLAGS) -o $@ $^ -lm

#token.h:		    token.h.placeheld
#	@echo "Substituting placeholders for token.h..."
#	@cp $< $@
#	$(SED) -i 's|<keywords_placeholder>|$(shell ./reformat_space_list.sh  -u -f keywords.txt)|' $@
#	$(SED) -i 's|<literal_tokens_placeholder>|$(shell ./reformat_space_list.sh -f literal_tokens.txt)|' $@

%.o:		        %.c %.h
	@echo "Compiling $@..."
	$(CC) $(CFLAGS) -c -o $@ $<

#resolve_sed:
#	$(eval $SED := $(shell gsed --version &> /dev/null && echo "gsed" || echo "sed"))

bminor.$(LEX):	bminor.placeheld.$(LEX)
	@echo "Substituting placeholders for bminor.$(LEX)..."
	@cp $< $@
	$(SED) -i 's|<keyword_rules_placeholder>|$(shell ./scripts/keyword_list_to_regex_list.sh keywords.txt)|' $@
	$(SED) -i 's|<literal_token_rules_placeholder>|$(shell ./scripts/literal_token_list_to_regex_list.sh literal_tokens.txt)|' $@

bminor.$(YACC):	bminor.placeheld.$(YACC) $(AST_COMP)
	@echo "Substituting placeholders for bminor.$(YACC)..."
	@cp $< $@
	$(SED) -i 's|<keywords_placeholder>|$(shell ./scripts/reformat_space_list.sh -t -u -f keywords.txt)|' $@
	$(SED) -i 's|<literal_tokens_placeholder>|$(shell ./scripts/reformat_space_list.sh -t -l -f literal_tokens.txt)|' $@

expr.c: expr.placeheld.c
	@echo "Substituting placeholders for expr.c..."
	@cp $< $@
	$(SED) -i 's|<oper_precedences_arr_placeholder>|$(shell ./scripts/parse_expr_t_table.sh -c 1)|' $@
	$(SED) -i 's|<commutativities_arr_placeholder>|$(shell ./scripts/parse_expr_t_table.sh -c 2)|' $@
	$(SED) -i 's|<associativities_arr_placeholder>|$(shell ./scripts/parse_expr_t_table.sh -c 3)|' $@
	$(SED) -i 's|<oper_str_arr_placeholder>|$(shell ./scripts/parse_expr_t_table.sh -c 4)|' $@
	$(SED) -i 's|<first_oper_placeholder>|$(shell ./scripts/get_end_expr_enum.sh -f)|g' $@
	$(SED) -i 's|<last_oper_placeholder>|$(shell ./scripts/get_end_expr_enum.sh -l)|g' $@

type.c:		      type.placeheld.c
	@echo "Substituting placeholders for type.c..."
	@cp $< $@
	$(SED) -i 's|<type_t_to_str_arr_placeholder>|$(shell ./scripts/type_enum_to_type_t_str_list.sh)|' $@
	$(SED) -i 's|<first_type_placeholder>|$(shell ./scripts/get_end_type_enum.sh)|g' $@

bminor_parse.c:	bminor.$(YACC)
	@echo "Generating parser $@..."
	$(YACC) $(YACCFLAGS) --defines=token.h --output=$@ $<

token.h:		    bminor_parse.c

bminor_scan.c:	bminor.$(LEX) token.h
	@echo "Generating scanner $@..."
	$(LEX) $(LEXFLAGS) -o $@ $<

main.c:			    main.placeheld.c token.h bminor.bison
	@echo "Substituting placeholders for main.c..."
	@cp $< $@
	$(SED) -i 's|<token_str_arr_placeholder>|$(shell ./scripts/bison_to_token_arr.sh bminor.bison)|' $@
