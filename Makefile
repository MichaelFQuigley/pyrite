all:
	bash compile.sh foo

parser:
	bash updateParser.sh

clean:
	rm *.ll *.o *.bc *.pyc
