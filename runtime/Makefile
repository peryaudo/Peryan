all: runtime.ll

runtime.ll: runtime.c common.h
	clang -O3 -Wall -c runtime.c -o runtime.o

clean:
	rm -rf runtime.o
