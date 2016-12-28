CPP_FILES=$(wildcard *.cpp)
O_FILES=$(wildcard *.o)
CPP_FLAGS+=-O3
LIB_PATH=../libs/libjsoncpp.a

all: build lib

lib:
	 ar rcs $(LIB_PATH) $(O_FILES)

build:
	clang++ -DJSON_IS_AMALGAMATION -std=c++11 -c $(CPP_FILES)

clean:
	rm $(LIB_PATH) *.o
