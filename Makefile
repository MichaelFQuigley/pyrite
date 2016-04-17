all:
	bash compile.sh

parser:
	bash updateParser.sh

clean:
	rm *.ll *.o
