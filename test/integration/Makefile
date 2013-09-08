all: 

clean:
	rm -rf compiled/*
	rm -rf actual/*
	rm -rf ./tester

test:
	../../src/peryan --runtime-path ../../runtime tester.pr ./tester
	./tester

# for emergency debugging
# ruby run_tester.rb

.PHONY: all test clean
