C_FILES=$(wildcard *.c)
LL_FILES=$(wildcard *.ll)
STDLIB=stdlib

all: | $(C_FILES) clean_ ll
	llvm-link-3.6 $(LL_FILES) > $(STDLIB).bc;
	llvm-dis-3.6 $(STDLIB).bc > $(STDLIB).ll;
	rm *.bc

ll:
	clang -S -emit-llvm -c $(C_FILES) 


clean_ :
	-rm *.ll

.PHONY : clean_
