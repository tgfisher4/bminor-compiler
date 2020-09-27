CC 		= gcc
CFLAGS 	= -g -Wall -std=gnu99
LD 		= gcc
LDFLAGS = 
SCGEN	= flex
SCGENFLAGS = 
SCSPECEXT = flex

TARGETS = bminor

all:		$(TARGETS)

clean:
	@echo Cleaning...
	@rm -f $(TARGETS) bminor.c *.o
	@rm -f token.h
	@rm -f bminor.$(SCSPECEXT)
	@rm -f main.c
	@rm -f bminor.c
	@rm -f valgrind-out.txt

bminor: 	main.o bminor.o token.h
	@echo "Linking bminor..."
	$(LD) $(LDFLAGS) -o $@ $^

token.h:	token.h.placeheld
	@echo "Substituting placeholders for token.h..."
	@cp $< $@
	@sed -i 's|<keywords_placeholder>|$(shell ./space_list_to_comma_list.sh -u -f keywords.txt)|' $@
	@sed -i 's|<literal_tokens_placeholder>|$(shell ./space_list_to_comma_list.sh -f literal_tokens.txt)|' $@

%.o:		%.c 
	@echo "Compiling $@..."
	$(CC) $(CFLAGS) -c -o $@ $<

bminor.$(SCSPECEXT):	bminor.$(SCSPECEXT).placeheld
	@echo "Substituting placeholders for bminor.$(SCSPECEXT)..."
	@cp $< $@
	@sed -i 's|<keyword_rules_placeholder>|$(shell ./keyword_list_to_regex_list.sh keywords.txt)|' $@
	@sed -i 's|<literal_token_rules_placeholder>|$(shell ./literal_token_list_to_regex_list.sh literal_tokens.txt)|' $@

bminor.c:	bminor.$(SCSPECEXT)
	@echo "Generating scanner $@..."
	$(SCGEN) $(SCGENFLAGS) -o $@ $<

main.c:		main.c.placeheld token.h
	@echo "Substituting placeholders for main.c..."
	@cp $< $@
	@sed -i 's|<token_str_arr_placeholder>|$(shell ./token_h_to_token_arr.sh token.h)|' $@
