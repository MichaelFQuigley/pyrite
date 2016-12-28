CPP_FILES=$(wildcard *.cpp)
CPP_FLAGS+=-Wall -g -O3
LD_FLAGS+=`llvm-config-3.6 --system-libs`
INCLUDEJSON=jsoncpp/include
LIBS=$(wildcard libs/*.a)
INCLUDETYPES=types
INCLUDELLVM=`llvm-config-3.6 --includedir`
LLVM_FLAGS=`llvm-config-3.6 --cppflags --libs all`
LLVM_FLAGS+=`llvm-config-3.6 --ldflags`

all: libjson libtypes
	clang++ -std=c++11 $(CPP_FILES) $(LIBS) -I$(INCLUDETYPES) -I$(INCLUDELLVM) -fno-rtti $(CPP_FLAGS) $(LD_FLAGS) $(LLVM_FLAGS) -o codeGen.o

libjson:
	make -C jsoncpp/

libtypes:
	make -C types/

clean:
	rm *.o *.out
