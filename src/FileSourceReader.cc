#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Options.h"
#include "FileSourceReader.h"

namespace Peryan {

std::istream *FileSourceReader::open(const std::string& fileName) {
	if (ifstreams.count(fileName)) {
		return ifstreams[fileName];
	}

	std::string fileName_ = fileName;

	std::ifstream *ifs = NULL;
	for (std::vector<std::string>::iterator it = options_.includePaths.begin();
		it != options_.includePaths.end(); ++it) {
		std::string cur = *it + std::string("/") + fileName;
		ifs = new std::ifstream(cur.c_str());
		if (ifs->fail()) {
			delete ifs;
			ifs = NULL;
		} else {
			break;
		}
	}

	if (ifs == NULL) {
		throw LexerError(-1, std::string("error: cannot find a file ")
					+ fileName_
					+ " in the include paths");
	}

	ifstreams[fileName] = ifs;

	return ifs;
};

void FileSourceReader::close(const std::string& fileName) {
	if (ifstreams.count(fileName)) {
		(*(ifstreams[fileName])).close();
		delete ifstreams[fileName];
		ifstreams.erase(fileName);
	}
	return;
};

}
