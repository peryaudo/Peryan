#include <iostream>
#include <cstdlib>

#include "Options.h"
#include "WarningPrinter.h"
#include "ASTPrinter.h"
#include "FileSourceReader.h"
#include "Lexer.h"
#include "Parser.h"
#include "LLVMCodeGen.h"

#ifdef _WIN32
#include <windows.h>
#endif

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
		}/* else if (cur == "--runtime") {
			if (i + 1 >= argc) {
				std::cerr<<"error: no runtime specified for --runtime"<<std::endl;
				return 1;
			} else {
				opt.runtime = std::string(argv[i + 1]);
				if (opt.runtime != "win32"
					&& opt.runtime != "unixcl"
				) {
					std::cerr<<"error: unknown runtime "<<opt.runtime<<std::endl;
					return 1;
				}
				++i;
			}
		}*/ else if (cur == "-w") {
			opt.inhibitWarnings = true;
		} else if(cur == "-o") {
			if (i + 1 >= argc) {
				std::cerr<<"error: no runtime specified for -o"<<std::endl;
				return 1;
			} else {
				opt.outputFileName = std::string(argv[i + 1]);
				++i;
			}
		} else if (cur.find("-") != std::string::npos) {
			std::cerr<<"error: unknown option "<<cur<<std::endl;
			return 1;
		} else {
			if (opt.mainFileName.empty()) {
				opt.mainFileName = cur;
			} else {
				std::cerr<<"error: unknown argument "<<cur<<std::endl;
				return 1;
			}
		}
	}

	if (opt.mainFileName.empty()) {
		std::cerr<<"Peryan Compiler (C) peryaudo"<<std::endl;
		std::cerr<<"Usage : peryan [options] [-o <output>] <input>"<<std::endl<<std::endl;

		std::cerr<<"Options :"<<std::endl;
		std::cerr<<" -o <output>\t\tSpecify the output file name"<<std::endl;
		std::cerr<<" --runtime-path <dir>\tSpecify the runtime directory"<<std::endl;
		// std::cerr<<" --runtime (unixcl|win32)\t\tSpecify the runtime to link"<<std::endl;
		std::cerr<<" --tmp-dir <dir>\tSpecify a temporary directory"<<std::endl;
		std::cerr<<" --verbose, -v\t\tDisplay the internal progress of the compiler"<<std::endl;
		std::cerr<<" --hsp-compatible, -hsp\tEnable HSP compatible behaviors"<<std::endl;
		std::cerr<<" --dump-ast\t\tDump the abstruct syntax tree generated internally (for debug)"<<std::endl;
		std::cerr<<" --dump-tokens\t\tDump the tokens generated internally (for debug)"<<std::endl;

		std::cerr<<std::endl;

		return 1;
	}

	if (opt.outputFileName.empty()) {
#ifdef _WIN32
		opt.outputFileName = "a.exe";
#else
		opt.outputFileName = "a.out";
#endif
	}

	if (opt.runtime.empty()) {
#ifdef _WIN32
		opt.runtime = "win32";
#else
		opt.runtime = "unixcl";
#endif
	}

	if (opt.verbose) {
		std::cerr<<"Peryan Compiler (C) peryaudo"<<std::endl<<std::endl;
	}

	// Get Peryan runtime path if not specified
	if (opt.runtimePath.empty()) {
#ifdef _WIN32
		char runtimePath[2048];
		GetModuleFileName(GetModuleHandle(NULL), runtimePath, sizeof(runtimePath) / sizeof(runtimePath[0]));

		opt.runtimePath = runtimePath;
		opt.runtimePath = opt.runtimePath.substr(0, opt.runtimePath.rfind("\\"));
#else
		char *runtimePath = getenv("PERYAN_RUNTIME_PATH");
		if (runtimePath == NULL) {
			std::cerr<<"error: please set PERYAN_RUNTIME_PATH"<<std::endl;
			return 1;
		}
		opt.runtimePath = runtimePath;
#endif
	}

	if (opt.tmpDir.empty()) {
#ifdef _WIN32
		char tmpDir[2048];
		GetTempPath(sizeof(tmpDir) / sizeof(tmpDir[0]), tmpDir);
#else
		char *tmpDir = getenv("TMPDIR");

		if (tmpDir == NULL) {
			std::cerr<<"error: please set TMPDIR"<<std::endl;
			return 1;
		}
#endif
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
		if (opt.runtime == "unixcl")
		{
			ss<<"gcc -s -w -o \""<<opt.outputFileName<<"\"";
			ss<<" \""<<opt.tmpDir<<"/tmp.o\" \""<<opt.runtimePath<<"/"<<opt.runtime<<".o\"";
		}
		else if (opt.runtime == "win32")
		{
			//ss<<"ld --subsystem windows -o \""<<opt.outputFileName<<"\" -e _WinMainCRTStartup";
			//ss<< \""<<opt.tmpDir<<"\\tmp.o\" \""<<opt.runtimePath<<"\\peryanw32rt.lib\"";
			
			// to use Mircosoft Linker (LINK.exe), you should have your %PATH% include the VC directories.
			// the exect directory is, for example, in Microsoft Visual Studio 2012 Express,
			// - C:\Program Files (x86)\Microsoft Visual Studio 11.0\Common7\IDE
			// - C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\bin

			ss<<"LINK /NODEFAULTLIB /NOLOGO /MACHINE:X86 /OUT:\""<<opt.outputFileName<<"\"";
			ss<<" \""<<opt.tmpDir<<"\\tmp.o\" \""<<opt.runtimePath<<"\\peryanw32rt.lib\""; 
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
