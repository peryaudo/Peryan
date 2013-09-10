#ifndef PERYAN_FILE_SOURCE_READER_H__
#define PERYAN_FILE_SOURCE_READER_H__

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include "SourceReader.h"

namespace Peryan {

class Options;

class FileSourceReader : public SourceReader {
private:
	Options& options_;
	std::string mainFileName_;
	std::map<std::string, std::ifstream *> ifstreams;
	std::vector<std::string> includePaths;
public:
	FileSourceReader(Options& options) : options_(options), mainFileName_(options.mainFileName) {}

	virtual std::string getMainName() { return mainFileName_; }
	virtual std::istream *open(const std::string& fileName);
	virtual void close(const std::string& fileName);
};

}

#endif

