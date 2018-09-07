all: 1730sh

1730sh: 1730sh.o
	g++ -Wall -g -o 1730sh 1730sh.o

1730sh.o: 1730sh.cpp
	g++ -Wall -std=c++14 -c -g -O0 -pedantic-errors 1730sh.cpp

clean:
	-rm -f 1730sh
	-rm -f 1730sh.o