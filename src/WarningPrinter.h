#ifndef PERYAN_WARNING_PRINTER_H__
#define PERYAN_WARNING_PRINTER_H__

#include "Token.h"

#include <vector>
#include <algorithm>

namespace Peryan {

class Lexer;

class WarningPrinter {
private:
	std::vector<std::pair<Position, std::string> > warnings_;
public:
	WarningPrinter() {}
	void add(Position position, std::string message) {
		warnings_.push_back(make_pair(position, message));
		return;
	}

	void print(Lexer& lexer);
};

};

#endif
