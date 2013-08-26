#ifndef PERYAN_TOKEN_H__
#define PERYAN_TOKEN_H__

#include <string>
#include <cassert>
#include <iostream>

namespace Peryan {

typedef int Position;

class Token {
public:
	typedef enum {
#define TOKEN(X) X,
#include "Tokens.def"
	} Type;

private:
	Type type_;
	Position position_;

	std::string str_;

	union {
		double double__;
		int int__;
		char char__;
	} data_;

	const static char *tokenNames_[];

	// to check the identifier is label in parser
	bool hasTrailingAlphabet_;

	// for [ and (
	bool hasWSBefore_;
public:
	Token() : type_(UNKNOWN) {}
	Token(Type type, Position position) : type_(type), position_(position) { }
	Token(Type type, bool boolArg, Position position) : type_(type), position_(position) {
		if (type_ == STAR) {
			hasTrailingAlphabet_ = boolArg;
		} else if (type_ == LBRACK || type_ == LPAREN) {
			hasWSBefore_ = boolArg;
		} else {
			assert(false);
		}
	}

	Token(Type type, const std::string& str, Position position)
		: type_(type), position_(position), str_(str) {}

	Token(Type type, double double__, Position position) : type_(type), position_(position) {
		data_.double__ = double__;
	}
	Token(Type type, int int__, Position position) : type_(type), position_(position) {
		data_.int__ = int__;
	}
	Token(Type type, char char__, Position position) : type_(type), position_(position) {
		data_.char__ = char__;
	}

	Type getType() const {
		return type_;
	}

	std::string toString() const;

	Position getPosition() const {
		return position_;
	}

	std::string getString() const {
		assert(type_ == STRING || type_ == ID || type_ == TYPE_ID);
		return str_;
	}
	double getFloat() const {
		assert(type_ == FLOAT);
		return data_.double__;
	}
	int getInteger() const {
		assert(type_ == INTEGER);
		return data_.int__;
	}
	char getChar() const {
		assert(type_ == CHAR);
		return data_.char__;
	}
	bool hasTrailingAlphabet() const {
		assert(type_ == STAR);
		return hasTrailingAlphabet_;
	}
	bool hasWSBefore() const {
		assert(type_ == LBRACK || type_ == LPAREN);
		return hasWSBefore_;
	}

	~Token() {}
};

};
#endif
