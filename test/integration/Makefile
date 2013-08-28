all: 

clean:
	rm -rf compiled/*
	rm -rf actual/*
	rm -rf ./tester

test:
	PERYAN_RUNTIME_PATH=../../runtime ../../src/peryan tester.pr ./tester
	./tester

# for emergency debugging
# ruby run_tester.rb

.PHONY: all test clean
