TARGET = peryan
OBJS = Token.o Lexer.o Parser.o Main.o FileSourceReader.o SymbolRegister.o SymbolResolver.o TypeResolver.o LLVMCodeGen.o WarningPrinter.o

include ../Makefile.in

Token.o: Token.cc Token.h Tokens.def

Lexer.o: Lexer.cc Token.h Tokens.def SourceReader.h WarningPrinter.h Lexer.h

Parser.o: Parser.cc Token.h Tokens.def Lexer.h AST.h SymbolTable.h Options.h WarningPrinter.h Parser.h SymbolRegister.h SymbolResolver.h TypeResolver.h

Main.o: Main.cc Options.h WarningPrinter.h AST.h ASTPrinter.h SourceReader.h FileSourceReader.h Token.h Tokens.def Lexer.h SymbolTable.h Parser.h CodeGen.h LLVMCodeGen.h

FileSourceReader.o: FileSourceReader.cc FileSourceReader.h Options.h SourceReader.h

WarningPrinter.o: WarningPrinter.cc WarningPrinter.h Lexer.h Tokens.def Token.h

SymbolRegister.o: SymbolRegister.cc SymbolRegister.h SymbolTable.h AST.h Token.h Tokens.def

SymbolResolver.o: SymbolResolver.cc SymbolResolver.h SymbolTable.h AST.h Token.h Tokens.def

TypeResolver.o: TypeResolver.cc TypeResolver.h SymbolTable.h AST.h Token.h Tokens.def

LLVMCodeGen.o: LLVMCodeGen.cc SymbolTable.h AST.h LLVMCodeGen.h Parser.h CodeGen.h Token.h Tokens.def Lexer.h
	$(CXX) $(CXXFLAGS) `llvm-config --cxxflags` $(CPPFLAGS) -c -o $@ $<
