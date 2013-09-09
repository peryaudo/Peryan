TARGETS = unixcl.o 

ifeq ($(OS),Windows_NT)
	TARGETS += win32.o
else
	TARGETS +=
endif

all: $(TARGETS)

unixcl.o: unixcl.c common.h
	gcc -O3 -Wall -c unixcl.c -o unixcl.o

win32.o: win32.c common.h
	gcc -O3 -Wall -c win32.c -o win32.o

clean:
	rm -rf unixcl.o win32.o
