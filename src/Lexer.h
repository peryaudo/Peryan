#ifndef PERYAN_LEXER_H__
#define PERYAN_LEXER_H__

#include <vector>
#include <string>
#include <iostream>

#include "Token.h"

namespace Peryan {

class SourceReader;
class Lexer;
class Options;
class WarningPrinter;

class LexerError : public std::exception {
private:
	Position position_;
	std::string message_;
public:
	LexerError(Position position, std::string message)
		: std::exception(), position_(position), message_(message) {}
	Position getPosition() { return position_; }
	std::string getMessage() { return message_; }
	std::string toString(const Lexer& lexer);

	virtual ~LexerError() throw() {}
};

class Lexer {
private:
	Lexer(const Lexer&);
	Lexer& operator=(const Lexer&);

private:
	SourceReader& sr_;
	Options& opt_;
	WarningPrinter& wp_;

	class Breadcrumb {
	public:
		Position totalPos; // position in source_
		int originalPos;   // position in the original stream
		int line;	   // number of lines from the top of the stream
		std::string name;  // stream name
		Breadcrumb(Position totalPos, int originalPos, int line, std::string name)
			: totalPos(totalPos), originalPos(originalPos), line(line), name(name) {}
		static bool compare(Position totalPos, const Breadcrumb& bc) {
			return totalPos < bc.totalPos;
		}
	};

	class Stream {
	public:
		std::istream *is;
		int pos;
		int line;
		std::string name;
		Stream(std::istream *is, std::string name) : is(is), pos(0), line(0), name(name) {}
	};

	std::vector<Breadcrumb> breadcrumbs_;

	std::vector<std::pair<std::string, Token::Type> > keywords;

	void initializeKeywords();

	void readSources() throw (LexerError);

	std::string source_;
	int p_;

	bool isPrevWS_;

	void consume(int n = 1) {
		if (p_ + n < source_.size()) {
			p_ += n;
		} else {
			p_ = source_.size();
		}

		isPrevWS_ = (source_[p_ - 1] == ' ' || source_[p_ - 1] == '\t');
	}

	char lookahead(int n = 0) {
		return p_ + n < source_.size() ? source_[p_ + n] : 0;
	}

	std::string readIdentifier();
	int readIdentifierLength();
	bool isFloatLiteral();
	double readFloatLiteral();
	int readHexLiteral() throw (LexerError);
	int readDecimalLiteral();
	int readBinaryLiteral() throw (LexerError);
	std::string readCharOrStringLiteral(char terminator) throw (LexerError);

	void getline(std::istream& is, std::string& str);

public:
	std::string getPrettyPrint(Position pos, std::string message = std::string()) const;

	Token getNextToken() throw (LexerError);
	Position getPosition() { return p_; }

	Lexer(SourceReader& fsr, Options& opt, WarningPrinter& wp);
};



};

#endif
