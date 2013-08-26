#ifndef PERYAN_STRING_SOURCE_READER_H__
#define PERYAN_STRING_SOURCE_READER_H__

#include <sstream>
#include <map>

#include "SourceReader.h"

namespace Peryan {

class StringSourceReader : public SourceReader {
private:
	std::string mainName_;
	std::map<std::string, std::stringstream *> streams;

public:
	StringSourceReader(const std::string& mainName) : mainName_(mainName) {
		streams["peryandefs"] = new std::stringstream("");
	}

	void setString(const std::string& name, const std::string& content) {
		streams[name] = new std::stringstream(content);
		return;
	}

	virtual std::string getMainName() { return mainName_; }
	virtual std::istream *open(const std::string& fileName) { return streams[fileName]; }
	virtual void close(const std::string& fileName) {
		if (streams.count(fileName)) {
			delete streams[fileName];
			streams.erase(fileName);
		}
		return;
	}
};

};

#endif
