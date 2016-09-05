GREEN=\e[0;32m
END_GREEN=$<\e[0m

all: codegen parser standardlib

codegen:
	printf '${GREEN}Building code gen utilities...\n${END_GREEN}'
	cd CodeGen && $(MAKE)

parser:
	printf '${GREEN}Building parser...\n${END_GREEN}'
	cd Parser && sh build_lex.sh

standardlib:
	printf '${GREEN}Building stdlib...\n${END_GREEN}'
	cd stdlib && $(MAKE) && $(MAKE)

.PHONY : all codegen parser standardlib
