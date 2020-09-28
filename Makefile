CC 		= gcc
CFLAGS 	= -g -Wall -std=gnu99
LD 		= gcc
LDFLAGS = 
LEX	= flex
LEXFLAGS = 
YACC  = bison
YACCFLAGS = --verbose

TARGETS = bminor

all:		$(TARGETS)

clean:
	@echo Cleaning...
	@rm -f $(TARGETS)
	@rm -f token.h
	@rm -f *.o
	@rm -f bminor.c bminor.$(LEX) bminor.$(YACC) bminor_parse.c bminor_scan.c
	@rm -f main.c
	@rm -f valgrind-out.txt

bminor: 	main.o bminor_scan.o bminor_parse.o token.h
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

bminor.$(LEX):	bminor.$(LEX).placeheld
	@echo "Substituting placeholders for bminor.$(LEX)..."
	@cp $< $@
	@sed -i 's|<keyword_rules_placeholder>|$(shell ./keyword_list_to_regex_list.sh keywords.txt)|' $@
	@sed -i 's|<literal_token_rules_placeholder>|$(shell ./literal_token_list_to_regex_list.sh literal_tokens.txt)|' $@

bminor.$(YACC):		bminor.$(YACC).placeheld
	@echo "Substituting placeholders for bminor.$(YACC)..."
	@cp $< $@
	@sed -i 's|<keywords_placeholder>|$(shell ./reformat_space_list.sh -t -u -f keywords.txt)|' $@
	@sed -i 's|<literal_tokens_placeholder>|$(shell ./reformat_space_list.sh -t -l -f literal_tokens.txt)|' $@

bminor_parse.c:	bminor.$(YACC)
	@echo "Generating parser $@..."
	$(YACC) $(YACCFLAGS) --defines=token.h --output=$@ $<

token.h : bminor_parse.c

bminor_scan.c:	bminor.$(LEX) token.h
	@echo "Generating scanner $@..."
	$(LEX) $(LEXFLAGS) -o $@ $<

main.c:		main.c.placeheld token.h bminor.bison
	@echo "Substituting placeholders for main.c..."
	@cp $< $@
	@sed -i 's|<token_str_arr_placeholder>|$(shell ./bison_to_token_arr.sh bminor.bison)|' $@
