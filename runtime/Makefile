all: runtime.ll

runtime.ll: runtime.c
	clang -O0 -S -emit-llvm runtime.c -o runtime.ll

clean:
	rm -rf runtime.ll
