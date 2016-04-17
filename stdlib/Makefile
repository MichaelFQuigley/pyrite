CSRC = $(wildcard *.c)
LIB_NAME=stdlib


all: $(CSRC)
	clang-3.6 -emit-llvm -S $(CSRC) -c -o $(LIB_NAME).ll
	llvm-link-3.6 $(LIB_NAME).ll -o $(LIB_NAME).bc
	llvm-link-3.6 $(LIB_NAME).ll > $(LIB_NAME).o
	chmod 775 $(LIB_NAME).o

clean:
	rm *.ll *.o
