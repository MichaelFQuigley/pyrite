CPP_FILES=$(wildcard *.cpp)
INCLUDEJSON=jsoncpp/include
LIBJSON=jsoncpp/src/build/debug/lib_json/libjsoncpp.a
INCLUDELLVM=llvm/include
INCLUDELLVM2=llvm/build/include
CPP_FLAGS+=-Wall
LD_FLAGS= -lpthread -lm -ltinfo -ldl
LLVM_FLAGS=`llvm-config-3.6 --cppflags --libs all`
LLVM_FLAGS+=`llvm-config-3.6 --ldflags`

all:
	clang++ -std=gnu++11 $(CPP_FILES) $(LIBJSON) -I$(INCLUDEJSON) -I$(INCLUDELLVM) -I$(INCLUDELLVM2) $(CPP_FLAGS) -o codeGen.o $(LD_FLAGS) $(LLVM_FLAGS) 

clean:
	rm *.o *.out
