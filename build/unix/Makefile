CXX = g++

ifeq ($(OS),Windows_NT)
	EXEEXT = .exe
	LIBS = `llvm-config --libs` -static-libgcc -static-libstdc++ -static
else
	EXEEXT = 
	LIBS = `llvm-config --libs`
endif

CXXFLAGS = -Wall -coverage -g
CPPFLAGS = 
LDFLAGS = `llvm-config --ldflags` -lpthread -lz -lncurses -ldl

OBJDIR = obj
TEST_OBJDIR = test/obj
DEPDIR = dep
BINDIR = bin
TEST_BINDIR = test/bin

TARGETS = $(PERYAN_TARGET) $(BINDIR)/peryandefs $(BINDIR)/unixcl.o $(PERYAN_UNIT_TEST_TARGET) $(PERYAN_INTEGRATION_TEST_TARGET)

PERYAN_TARGET = $(BINDIR)/peryan$(EXEEXT)
PERYAN_SRCDIR = ../../src
PERYAN_SRCS = Main.cc WarningPrinter.cc Token.cc FileSourceReader.cc Lexer.cc Parser.cc \
	      SymbolRegister.cc SymbolResolver.cc TypeResolver.cc LLVMCodeGen.cc
PERYAN_OBJS = $(addprefix $(OBJDIR)/, $(patsubst %.cc, %.o, $(PERYAN_SRCS)))

PERYAN_UNIT_TEST_TARGET = $(TEST_BINDIR)/peryan_unit_test$(EXEEXT)
PERYAN_UNIT_TEST_SRCDIR = ../../test/unit
PERYAN_UNIT_TEST_SRCS = ASTPrinterTest.cc LexerTest.cc ParserTest.cc SemanticsTest.cc
PERYAN_UNIT_TEST_OBJS = $(addprefix $(TEST_OBJDIR)/, $(patsubst %.cc, %.o, $(PERYAN_UNIT_TEST_SRCS)) gtest-all.o gtest_main.o) \
		   $(addprefix $(OBJDIR)/, $(patsubst %.cc, %.o, $(filter-out LLVMCodeGen.cc Main.cc, $(PERYAN_SRCS))))

PERYAN_INTEGRATION_TEST_TARGET = $(TEST_BINDIR)/peryan_integration_test$(EXEEXT) 
PERYAN_INTEGRATION_TEST_SRCDIR = ../../test/integration

PERYAN_RUNTIME_SRCDIR = ../../runtime

all: $(TARGETS)

test: $(PERYAN_UNIT_TEST_TARGET) $(PERYAN_INTEGRATION_TEST_TARGET)
	$(PERYAN_UNIT_TEST_TARGET)
	cd $(TEST_BINDIR); rm -rf actual; rm -rf compiled; mkdir actual; mkdir compiled; ./peryan_integration_test$(EXEEXT);
	mkdir -p test/coverage/
	lcov -c -d ./obj -b . -o test/coverage/peryan.info --rc lcov_branch_coverage=1
	lcov --remove test/coverage/peryan.info "/usr/*" -o test/coverage/peryan.info --rc lcov_branch_coverage=1
	genhtml test/coverage/peryan.info --branch-coverage -o test/coverage/ --rc lcov_branch_coverage=1

unittest: $(PERYAN_UNIT_TEST_TARGET)
	$(PERYAN_UNIT_TEST_TARGET)

DEPS = $(addprefix $(DEPDIR)/, $(patsubst %.cc, %.d, $(PERYAN_SRCS))) \
       $(addprefix $(DEPDIR)/, $(patsubst %.cc, %.d, $(PERYAN_UNIT_TEST_SRCS)))

clean:
	$(RM) -rf bin dep obj test

-include $(DEPS)

$(PERYAN_TARGET): $(PERYAN_OBJS)
	mkdir -p $(BINDIR)
	$(CXX) -o $@ $(PERYAN_OBJS) $(LIBS) $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS)

$(PERYAN_UNIT_TEST_TARGET): $(PERYAN_UNIT_TEST_OBJS)
	mkdir -p $(TEST_BINDIR)
	$(CXX) -o $@ $(PERYAN_UNIT_TEST_OBJS) $(LIBS) $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS)

$(BINDIR)/unixcl.o: $(addprefix $(PERYAN_RUNTIME_SRCDIR)/, unixcl.c common.h)
	mkdir -p $(BINDIR)
	gcc -Wall -c $< -o $@

$(TEST_OBJDIR)/%.o: $(PERYAN_UNIT_TEST_SRCDIR)/%.cc
	mkdir -p $(TEST_OBJDIR)
	mkdir -p $(DEPDIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $< -MMD -MF $(DEPDIR)/$(*F).d

$(OBJDIR)/%.o: $(PERYAN_SRCDIR)/%.cc
	mkdir -p $(OBJDIR)
	mkdir -p $(DEPDIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $< -MMD -MF $(DEPDIR)/$(*F).d

$(OBJDIR)/LLVMCodeGen.o:
	mkdir -p $(OBJDIR)
	mkdir -p $(DEPDIR)
	$(CXX) $(CXXFLAGS) `llvm-config --cxxflags` $(CPPFLAGS) -c -o $@ $(PERYAN_SRCDIR)/LLVMCodeGen.cc -MMD -MF $(DEPDIR)/LLVMCodeGen.d

$(TEST_OBJDIR)/gtest-all.o : $(addprefix $(PERYAN_UNIT_TEST_SRCDIR)/, gtest/gtest-all.cc gtest/gtest.h)
	mkdir -p $(TEST_OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(PERYAN_UNIT_TEST_SRCDIR) -c $< -o $@ 

$(TEST_OBJDIR)/gtest_main.o : $(addprefix $(PERYAN_UNIT_TEST_SRCDIR)/, gtest/gtest_main.cc gtest/gtest.h)
	mkdir -p $(TEST_OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(PERYAN_UNIT_TEST_SRCDIR) -c $< -o $@

$(BINDIR)/peryandefs: $(PERYAN_RUNTIME_SRCDIR)/peryandefs
	mkdir -p $(BINDIR)
	cp $< $@

$(PERYAN_INTEGRATION_TEST_TARGET): $(PERYAN_INTEGRATION_TEST_SRCDIR)/tester.pr \
						 $(PERYAN_TARGET) $(BINDIR)/unixcl.o $(BINDIR)/peryandefs
	mkdir -p $(TEST_BINDIR)
	$(PERYAN_TARGET) --runtime-path $(BINDIR) $< -o $@

.PHONY: all test unittest clean
