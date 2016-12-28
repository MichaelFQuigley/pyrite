CPP_FILES=$(wildcard *.cpp)
O_FILES=$(wildcard *.o)
CPP_FLAGS+=-Wall -O3
LD_FLAGS+=`llvm-config-3.6 --system-libs`
LIB_PATH=../libs/libpyritetypes.a
LLVM_FLAGS=`llvm-config-3.6 --cppflags --libs all`
LLVM_FLAGS+=`llvm-config-3.6 --ldflags`
INCLUDELLVM=`llvm-config-3.6 --includedir`
SRCINCLUDEPATH=.

all: build lib

lib:
	ar rcs $(LIB_PATH) $(O_FILES)

build:
	clang++ -c -std=c++11 $(CPP_FILES) -I$(SRCINCLUDEPATH) -I$(INCLUDELLVM) $(CPP_FLAGS) $(LD_FLAGS) $(LLVM_FLAGS)

clean:
	rm *.o $(LIB_PATH)
