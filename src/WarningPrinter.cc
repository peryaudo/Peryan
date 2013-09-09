#include "WarningPrinter.h"
#include "Lexer.h"

namespace Peryan {

void WarningPrinter::print(Lexer& lexer) {
	for (std::vector<std::pair<Position, std::string> >::iterator it = warnings_.begin();
			it != warnings_.end(); ++it) {
		if (it->first == -1) {
			std::cerr<<(it->second)<<std::endl;
		} else {
			std::cerr<<lexer.getPrettyPrint(it->first, it->second)<<std::endl;
		}
	}
	return;
}

}
