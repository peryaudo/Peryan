all: runtime.ll

runtime.ll: runtime.c common.h
	gcc -O3 -Wall -c runtime.c -o runtime.o

clean:
	rm -rf runtime.o
