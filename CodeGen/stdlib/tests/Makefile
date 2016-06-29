STDLIB_FILES=$(wildcard ../*.c)
gc-tests:
	clang -Wall -I`pwd`/.. ../gc_base.c gc_tests.c  -o gc_tests.o


types-tests:
	clang -Wall -I`pwd`/.. $(STDLIB_FILES) types_tests.c  -o types_tests.o
