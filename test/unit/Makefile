TARGET = peryan_test
SRC = ../../src
OBJS = $(SRC)/Lexer.o $(SRC)//Token.o $(SRC)/Parser.o $(SRC)/SymbolRegister.o $(SRC)/SymbolResolver.o $(SRC)/TypeResolver.o
OBJS += LexerTest.o ASTPrinterTest.o ParserTest.o  SemanticsTest.o gtest_main.o gtest-all.o

include ../../Makefile.in

LexerTest.o: LexerTest.cc $(SRC)/WarningPrinter.h $(SRC)/Lexer.h $(SRC)/StringSourceReader.h $(SRC)/Token.h $(SRC)/Tokens.def $(SRC)/SourceReader.h

ParserTest.o: ParserTest.cc $(SRC)/WarningPrinter.h $(SRC)/Options.h $(SRC)/Lexer.h $(SRC)/StringSourceReader.h $(SRC)/Token.h $(SRC)/Tokens.def $(SRC)/SourceReader.h $(SRC)/AST.h $(SRC)/SymbolTable.h $(SRC)/ASTPrinter.h

ASTPrinterTest.o: ASTPrinterTest.cc $(SRC)/AST.h $(SRC)/ASTPrinter.h

SemanticsTest.o: SemanticsTest.cc $(SRC)/WarningPrinter.h $(SRC)/Options.h $(SRC)/Lexer.h $(SRC)/StringSourceReader.h $(SRC)/Token.h $(SRC)/Tokens.def $(SRC)/SourceReader.h $(SRC)/AST.h $(SRC)/SymbolTable.h $(SRC)/ASTPrinter.h

gtest-all.o : gtest/gtest.h gtest/gtest-all.cc
	$(CXX) $(CXXFLAGS) -I. -c gtest/gtest-all.cc

gtest_main.o : gtest/gtest.h gtest/gtest_main.cc
	$(CXX) $(CXXFLAGS) -I. -c gtest/gtest_main.cc

test: $(TARGET)
	./$(TARGET)
