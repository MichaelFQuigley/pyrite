C_FILES=$(wildcard *.c)
LL_FILES=$(patsubst %.c, %.ll,$(C_FILES))
STDLIB=stdlib
C_FLAGS=-fno-strict-aliasing -Wall -S -O3 -emit-llvm -c

all: | $(C_FILES) clean_ ll
	llvm-link-3.6 $(LL_FILES) > $(STDLIB).bc;
	llvm-dis-3.6 $(STDLIB).bc > $(STDLIB).ll;
	rm *.bc

ll:
	clang $(C_FLAGS) $(C_FILES) 


clean_ :
	-rm *.ll

.PHONY : clean_ ll all
