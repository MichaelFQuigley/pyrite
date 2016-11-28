CPP_FILES=$(wildcard *.cpp)
O_FILES=$(wildcard *.o)
CPP_FLAGS+=-Wall -g -O3
LD_FLAGS= -lpthread -lm -ltinfo -ldl
LIB_PATH=../libs/libpyritetypes.a
LLVM_FLAGS=`llvm-config-3.6 --cppflags --libs all`
LLVM_FLAGS+=`llvm-config-3.6 --ldflags`
INCLUDELLVM=`llvm-config-3.6 --includedir`
SRCINCLUDEPATH=.

all:
	clang++ -c -std=c++11 $(CPP_FILES) -I$(SRCINCLUDEPATH) -I$(INCLUDELLVM) $(CPP_FLAGS) $(LD_FLAGS) $(LLVM_FLAGS) 
	ar rcs $(LIB_PATH) $(O_FILES)
clean:
	rm *.o *.out *.a
