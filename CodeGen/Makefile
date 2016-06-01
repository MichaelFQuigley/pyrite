CPP_FILES=$(wildcard *.cpp)
INCLUDEJSON=jsoncpp/include
LIBJSON=jsoncpp/src/build/debug/lib_json/libjsoncpp.a

all:
	g++ -std=c++11 $(CPP_FILES) $(LIBJSON) -I$(INCLUDEJSON) -o codeGen.o

clean:
	rm *.o *.out
