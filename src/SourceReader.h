#ifndef PERYAN_SOURCE_READER_H__
#define PERYAN_SOURCE_READER_H__

namespace Peryan {

class SourceReader {
public:
	SourceReader() {}
	virtual std::string getMainName() = 0;
	virtual std::istream *open(const std::string& fileName) = 0;
	virtual void close(const std::string& fileName) = 0;
	virtual ~SourceReader() {};
};

};

#endif

