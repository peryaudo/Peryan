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
	std::string mainFileName;
	std::vector<std::string> includePaths;
	std::string outputFileName;
	Options() : dumpAST(false), verbose(false), hspCompat(false), dumpTokens(false) {}
};

}

#endif
