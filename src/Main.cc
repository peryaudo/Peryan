#include <iostream>
#include <cstdlib>

#include "Options.h"
#include "WarningPrinter.h"
#include "ASTPrinter.h"
#include "FileSourceReader.h"
#include "Lexer.h"
#include "Parser.h"
#include "LLVMCodeGen.h"

int main(int argc, char *argv[])
{
	Peryan::Options opt;

	for (int i = 1; i < argc; ++i) {
		const std::string cur(argv[i]);
		if (cur.find("-I") == 0) {
			opt.includePaths.push_back(cur.substr(2));
		} else if (cur == "--dump-ast") {
			opt.dumpAST = true;
		} else if (cur == "--verbose") {
			opt.verbose = true;
		} else if (cur == "--strict") {
			opt.strict = true;
		} else if (cur.find("-") != std::string::npos) {
			std::cerr<<"warning: unknown option"<<std::endl;
		} else {
			if (opt.mainFileName.empty()) {
				opt.mainFileName = cur;
			} else if (opt.outputFileName.empty()) {
				opt.outputFileName = cur;
			} else {
				std::cerr<<"warning: unknown argument"<<std::endl;
			}
		}
	}

	if (opt.mainFileName.empty() || opt.outputFileName.empty()) {
		std::cerr<<"Peryan Compiler (C) peryaudo"<<std::endl;
		std::cerr<<"Usage : peryan <input> <output>"<<std::endl<<std::endl;
		return -1;
	}

	if (opt.verbose) {
		std::cerr<<"Peryan Compiler (C) peryaudo"<<std::endl<<std::endl;
	}

	// Get Peryan runtime path
	char *runtimePath = getenv("PERYAN_RUNTIME_PATH");
	if (runtimePath == NULL) {
		std::cerr<<"error: please set PERYAN_RUNTIME_PATH"<<std::endl;
		return -1;
	}
	char *tmpDir = getenv("TMPDIR");
	if (tmpDir == NULL) {
		std::cerr<<"error: please set TMPDIR"<<std::endl;
		return -1;
	}

	// Set default including paths
	opt.includePaths.push_back(runtimePath);
	opt.includePaths.push_back(".");

	// Lexer, Parser, Code Generator

	Peryan::WarningPrinter warnings;

	Peryan::FileSourceReader sourceReader(opt);

	Peryan::Lexer lexer(sourceReader, warnings);

	Peryan::Parser parser(lexer, opt, warnings);

	warnings.print(lexer);

	try {
		parser.parse();
	} catch (Peryan::LexerError le) {
		std::cerr<<le.toString(lexer)<<std::endl;
		return -1;
	} catch (Peryan::ParserError pe) {
		std::cerr<<pe.toString(lexer)<<std::endl;
		return -1;
	} catch (Peryan::SemanticsError se) {
		std::cerr<<se.toString(lexer)<<std::endl;
		return -1;
	} catch (...) {
		std::cerr<<"error: unknown error"<<std::endl;
		return -1;
	}

	if (opt.dumpAST) {
		Peryan::ASTPrinter printer;
		std::cerr<<printer.toString(parser.getTransUnit())<<std::endl;
	}

	Peryan::LLVMCodeGen codeGen(parser, std::string(tmpDir) + std::string("/tmp.ll"));
	
	if (opt.verbose) std::cerr<<"generating LLVM IR...";
	codeGen.generate();
	if (opt.verbose) std::cerr<<"ok."<<std::endl;

	{
		std::stringstream ss;
		ss<<"llvm-link \""<<tmpDir<<"/tmp.ll\" \""<<runtimePath<<"/runtime.ll\" -S -o \"";
		ss<<tmpDir<<"/tmp_linked.ll\"";
		system(ss.str().c_str());
	}
	{
		std::stringstream ss;
		ss<<"llc -o="<<tmpDir<<"/tmp.s \""<<tmpDir<<"/tmp_linked.ll\"";
		system(ss.str().c_str());
	}
	{
		std::stringstream ss;
		ss<<"clang -o \""<<opt.outputFileName<<"\" \""<<tmpDir<<"/tmp.s\"";
		system(ss.str().c_str());
	}

	if (opt.verbose) std::cerr<<std::endl<<"compilation finished."<<std::endl;

	return 0;
}
