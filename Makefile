all:
	cd build/unix; make all

test:
	cd build/unix; make test

clean:
	cd build/unix; make clean

.PHONY: all test clean
