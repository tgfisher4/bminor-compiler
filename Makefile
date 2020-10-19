CC 		= gcc
CFLAGS 	= -g -Wall -std=gnu99
LD 		= gcc
LDFLAGS = 
LEX	= flex
LEXFLAGS = 
YACC  = bison
YACCFLAGS = --verbose

AST_COMP = expr.o decl.o stmt.o type.o

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
	@rm -f main.c
	@rm -f bminor_parse.output
	@rm -f *_tests/*_tests/*.out
	@rm -f valgrind-out.txt

bminor: 	main.o bminor_scan.o bminor_parse.o $(AST_COMP) token.h
	@echo "Linking bminor..."
	$(LD) $(LDFLAGS) -o $@ $^


#token.h:	token.h.placeheld
#	@echo "Substituting placeholders for token.h..."
#	@cp $< $@
#	@sed -i 's|<keywords_placeholder>|$(shell ./reformat_space_list.sh  -u -f keywords.txt)|' $@
#	@sed -i 's|<literal_tokens_placeholder>|$(shell ./reformat_space_list.sh -f literal_tokens.txt)|' $@

%.o:		%.c 
	@echo "Compiling $@..."
	$(CC) $(CFLAGS) -c -o $@ $<

bminor.$(LEX):	bminor.placeheld.$(LEX)
	@echo "Substituting placeholders for bminor.$(LEX)..."
	@cp $< $@
	@sed -i 's|<keyword_rules_placeholder>|$(shell ./scripts/keyword_list_to_regex_list.sh keywords.txt)|' $@
	@sed -i 's|<literal_token_rules_placeholder>|$(shell ./scripts/literal_token_list_to_regex_list.sh literal_tokens.txt)|' $@

bminor.$(YACC):		bminor.placeheld.$(YACC) $(AST_COMP)
	@echo "Substituting placeholders for bminor.$(YACC)..."
	@cp $< $@
	@sed -i 's|<keywords_placeholder>|$(shell ./scripts/reformat_space_list.sh -t -u -f keywords.txt)|' $@
	@sed -i 's|<literal_tokens_placeholder>|$(shell ./scripts/reformat_space_list.sh -t -l -f literal_tokens.txt)|' $@

bminor_parse.c:	bminor.$(YACC)
	@echo "Generating parser $@..."
	$(YACC) $(YACCFLAGS) --defines=token.h --output=$@ $<

token.h: bminor_parse.c

bminor_scan.c:	bminor.$(LEX) token.h
	@echo "Generating scanner $@..."
	$(LEX) $(LEXFLAGS) -o $@ $<

main.c:		main.placeheld.c token.h bminor.bison 
	@echo "Substituting placeholders for main.c..."
	@cp $< $@
	@sed -i 's|<token_str_arr_placeholder>|$(shell ./scripts/bison_to_token_arr.sh bminor.bison)|' $@
