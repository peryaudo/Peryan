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
	Peryan::WarningPrinter warnings;

	for (int i = 1; i < argc; ++i) {
		const std::string cur(argv[i]);
		if (cur.find("-I") == 0) {
			opt.includePaths.push_back(cur.substr(2));
		} else if (cur == "--dump-ast") {
			opt.dumpAST = true;
		} else if (cur == "--verbose" || cur == "-v") {
			opt.verbose = true;
		} else if (cur == "-hsp" || cur == "--hsp-compatible") {
			opt.hspCompat = true;
			warnings.add(-1, "warning: using HSP compatible mode");
		} else if (cur == "--dump-tokens") {
			opt.dumpTokens = true;
		} else if (cur == "--runtime-path") {
			if (i + 1 >= argc) {
				std::cerr<<"error: no directory specified for --runtime-path"<<std::endl;
				return 1;
			} else {
				opt.runtimePath = std::string(argv[i + 1]);
				++i;
			}
		} else if (cur == "--tmp-dir") {
			if (i + 1 >= argc) {
				std::cerr<<"error: no directory specified for --tmp-dir"<<std::endl;
				return 1;
			} else {
				opt.tmpDir = std::string(argv[i + 1]);
				++i;
			}
		} else if (cur == "--runtime") {
			if (i + 1 >= argc) {
				std::cerr<<"error: no runtime specified for --runtime"<<std::endl;
				return 1;
			} else {
				opt.runtime = std::string(argv[i + 1]);
				if (opt.runtime != "win32" && opt.runtime != "unixcl") {
					std::cerr<<"error: unknown runtime "<<opt.runtime<<std::endl;
					return 1;
				}
				++i;
			}
		} else if (cur == "-w") {
			opt.inhibitWarnings = true;
		} else if (cur.find("-") != std::string::npos) {
			std::cerr<<"error: unknown option "<<cur<<std::endl;
			return 1;
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
		return 1;
	}

	if (opt.runtime.empty()) {
		opt.runtime = "unixcl";
	}

	if (opt.verbose) {
		std::cerr<<"Peryan Compiler (C) peryaudo"<<std::endl<<std::endl;
	}

	// Get Peryan runtime path if not specified
	if (opt.runtimePath.empty()) {
		char *runtimePath = getenv("PERYAN_RUNTIME_PATH");
		if (runtimePath == NULL) {
			std::cerr<<"error: please set PERYAN_RUNTIME_PATH"<<std::endl;
			return 1;
		}
		opt.runtimePath = runtimePath;
	}

	if (opt.tmpDir.empty()) {
		char *tmpDir = getenv("TMPDIR");

		// for Windows
		if (tmpDir == NULL)
			tmpDir = getenv("TMP");
		if (tmpDir == NULL)
			tmpDir = getenv("TEMP");

		if (tmpDir == NULL) {
			std::cerr<<"error: please set TMPDIR"<<std::endl;
			return 1;
		}
		opt.tmpDir = tmpDir;
	}

	// Set default including paths
	opt.includePaths.push_back(opt.runtimePath);
	opt.includePaths.push_back(".");

	// Lexer and parser

	Peryan::FileSourceReader sourceReader(opt);

	Peryan::Lexer lexer(sourceReader, opt, warnings);

	Peryan::Parser parser(lexer, opt, warnings);

	try {
		parser.parse();
	} catch (Peryan::LexerError le) {
		if (!opt.inhibitWarnings) warnings.print(lexer);
		std::cerr<<le.toString(lexer)<<std::endl;
		return 1;
	} catch (Peryan::ParserError pe) {
		if (!opt.inhibitWarnings) warnings.print(lexer);
		std::cerr<<pe.toString(lexer)<<std::endl;
		return 1;
	} catch (Peryan::SemanticsError se) {
		if (!opt.inhibitWarnings) warnings.print(lexer);
		std::cerr<<se.toString(lexer)<<std::endl;
		return 1;
	} catch (...) {
		if (!opt.inhibitWarnings) warnings.print(lexer);
		std::cerr<<"error: unknown error"<<std::endl;
		return 1;
	}

	if (!opt.inhibitWarnings) warnings.print(lexer);

	if (opt.dumpAST) {
		Peryan::ASTPrinter printer(/* pretty = */ true);
		std::cerr<<printer.toString(parser.getTransUnit())<<std::endl;
	}

	// Code generator

	Peryan::LLVMCodeGen codeGen(parser, opt.tmpDir + std::string("/tmp.ll"));
	
	if (opt.verbose) std::cerr<<"generating LLVM IR...";
	codeGen.generate();
	if (opt.verbose) std::cerr<<"ok."<<std::endl;

	{
		std::stringstream ss;
		ss<<"llc -filetype=obj -o \""<<opt.tmpDir<<"/tmp.o\" \""<<opt.tmpDir<<"/tmp.ll\"";
		if (opt.verbose) std::cerr<<ss.str()<<std::endl;
		if (system(ss.str().c_str())) {
			std::cerr<<"error: error while compiling LLVM IR"<<std::endl;
			return 1;
		}
	}
	{
		std::stringstream ss;
		if (opt.runtime == "unixcl") {
			ss<<"gcc -s -w -o \""<<opt.outputFileName<<"\" \""<<opt.tmpDir<<"/tmp.o\" \""<<opt.runtimePath<<"/"<<opt.runtime<<".o\"";
		} else if (opt.runtime == "win32") {
			ss<<"gcc -s -DWIN32 -mwindows -v -o \""<<opt.outputFileName<<"\" \""<<opt.tmpDir<<"/tmp.o\" \""<<opt.runtimePath<<"/"<<opt.runtime<<".o\"";
		}
		if (opt.verbose) std::cerr<<ss.str()<<std::endl;
		if (system(ss.str().c_str())) {
			std::cerr<<"error: error while linking"<<std::endl;
			return 1;
		}
	}

	if (opt.verbose) std::cerr<<std::endl<<"compilation finished."<<std::endl;

	return 0;
}
