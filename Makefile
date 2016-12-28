GREEN=\e[0;32m
END_GREEN=$<\e[0m

all: codegen parser standardlib

codegen:
	printf '${GREEN}Building code gen utilities...\n${END_GREEN}'
	$(MAKE) -C Codegen/types
	$(MAKE) -C Codegen

parser:
	printf '${GREEN}Building parser...\n${END_GREEN}'
	cd Parser && sh build_lex.sh

standardlib:
	printf '${GREEN}Building stdlib...\n${END_GREEN}'
	$(MAKE) -C stdlib && $(MAKE) -C stdlib

.PHONY : all codegen parser standardlib
