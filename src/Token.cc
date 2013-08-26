#include <sstream>

#include "Token.h"

namespace Peryan {

const char *Token::tokenNames_[] = {
#define TOKEN(X) #X,
#include "Tokens.def"
};


std::string Token::toString() const {
	std::string str;

	str += "<";
	str += tokenNames_[getType()];
	switch (getType()) {
	case ID:
	case TYPE_ID:
	case STRING:
		str += ", ";
		str += getString();
		break;
	case INTEGER:
		str += ", ";
		{
			std::stringstream ss;
			ss<<getInteger();
			str += ss.str();
		}
		break;
	case CHAR:
		str += ", ";
		str += getChar();
		break;
	default: ;
	}
	str += ">";

	return str;
}

}
