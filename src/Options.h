#ifndef PERYAN_OPTIONS_H__
#define PERYAN_OPTIONS_H__

#include <vector>
#include <string>

namespace Peryan {

class Options {
public:
	bool dumpAST;
	bool verbose;
	bool hspCompat;
	bool dumpTokens;
	bool inhibitWarnings;
	std::string mainFileName;
	std::vector<std::string> includePaths;
	std::string outputFileName;
	std::string runtimePath;
	std::string tmpDir;
	Options() : dumpAST(false), verbose(false), hspCompat(false), dumpTokens(false), inhibitWarnings(false) {}
};

}

#endif
