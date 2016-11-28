CPP_FILES=$(wildcard *.cpp)
CPP_FLAGS+=-Wall -g -O3
LD_FLAGS= -lpthread -lm -ltinfo -ldl
INCLUDEJSON=jsoncpp/include
LIBJSON=libs/libjsoncpp.a
LIBTYPES=libs/libpyritetypes.a
INCLUDETYPES=types
INCLUDELLVM=`llvm-config-3.6 --includedir`
LLVM_FLAGS=`llvm-config-3.6 --cppflags --libs all`
LLVM_FLAGS+=`llvm-config-3.6 --ldflags`

all:
	make -C types/ && clang++ -std=c++11 $(CPP_FILES) $(LIBTYPES) $(LIBJSON) -I$(INCLUDETYPES) -I$(INCLUDEJSON) -I$(INCLUDELLVM) $(CPP_FLAGS) -o codeGen.o $(LD_FLAGS) $(LLVM_FLAGS) 

clean:
	rm *.o *.out
