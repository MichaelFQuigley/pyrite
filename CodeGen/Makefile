CPP_FILES=$(wildcard *.cpp)
INCLUDEJSON=jsoncpp/include
LIBJSON=jsoncpp/src/build/debug/lib_json/libjsoncpp.a
INCLUDELLVM=llvm/include
INCLUDELLVM2=llvm/build/include


LIBLLVMDIR=llvm/build/lib/
LIBLLVM=$(LIBLLVMDIR)/libLLVMCore.a
CPP_FLAGS=-D__STDC_CONSTANT_MACROS 
CPP_FLAGS+=-D__STDC_LIMIT_MACROS
CPP_FLAGS+=-Wall

all:
	g++-4.8 -std=c++11 $(LIBLLVM) $(CPP_FILES) $(LIBJSON) -I$(INCLUDEJSON) -I$(INCLUDELLVM) -I$(INCLUDELLVM2) $(CPP_FLAGS) -o codeGen.o

clean:
	rm *.o *.out
