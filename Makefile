all:
	cd src; make
	cd test; make
	cd runtime; make

clean:
	cd src; make clean
	cd test; make clean
	cd runtime; make clean

test: all
	cd test; make test

.PHONY: all test clean
