CPP_FILES=$(wildcard *.cpp)
CPP_FLAGS+=-Wall -g -O3
LD_FLAGS+=`llvm-config-3.9 --system-libs`
INCLUDEJSON=jsoncpp/include
LIBS=$(wildcard libs/*.a)
INCLUDETYPES=types
INCLUDELLVM=`llvm-config-3.9 --includedir`
LLVM_FLAGS=`llvm-config-3.9 --cppflags --libs all`
LLVM_FLAGS+=`llvm-config-3.9 --ldflags`

all: libjson libtypes
	clang++ -std=c++11 $(CPP_FILES) $(LIBS) -I$(INCLUDETYPES) -I$(INCLUDELLVM) $(CPP_FLAGS) $(LD_FLAGS) $(LLVM_FLAGS) -o codeGen.o

libjson:
	make -C jsoncpp/

libtypes:
	make -C types/

clean:
	rm *.o *.out
