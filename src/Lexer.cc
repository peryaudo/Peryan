#include <stack>
#include <algorithm>
#include <string>
#include <set>
#include <sstream>

#include "Token.h"
#include "SourceReader.h"
#include "WarningPrinter.h"
#include "Lexer.h"

namespace Peryan {

Lexer::Lexer(SourceReader& sr, Options& opt, WarningPrinter& wp)
		: sr_(sr), opt_(opt), wp_(wp), p_(0), isPrevWS_(false) {
	initializeKeywords();
};

void Lexer::initializeKeywords() {

#define PUNCTUATOR(X,Y) keywords.push_back(std::make_pair(std::string(Y), Token::X));
#define KEYWORD(X,Y) keywords.push_back(std::make_pair(std::string(Y), Token::KW_ ## X));
#include "Tokens.def"

	std::sort(keywords.rbegin(), keywords.rend());
	return;
}


Token Lexer::getNextToken() {
	if (breadcrumbs_.size() == 0) {
		readSources();
	}

	
	if (lookahead() == 0) {
		return Token(Token::END, getPosition());
	}

	// skip whitespaces and comments
	while (true) {
		if (lookahead() == ' ' || lookahead() == '\t') {
			consume();
		} else if (lookahead(0) == '/' && lookahead(1) == '*') {
			Position begPos = getPosition();
			consume(2);
			while (!(lookahead(0) == '*' && lookahead(1) == '/')) {
				consume();
				if (lookahead() == 0)
					throw LexerError(begPos, "error: comment is not closed");
			}
			consume(2);
		} else if (lookahead(0) == '/' && lookahead(1) == '/') {
			consume(2);

			while (!((lookahead(0) == '\r' && lookahead(1) == '\n')
						|| lookahead() == '\n' || lookahead() == 0))
				consume();
			// don't consume return characters
		} else if (lookahead(0) == ';') {
			consume(1);

			while (!((lookahead(0) == '\r' && lookahead(1) == '\n')
						|| lookahead() == '\n' || lookahead() == 0))
				consume();
			// don't consume return characters
		} else {
			break;
		}
	}

	const Position curPos = getPosition();

	if (lookahead(0) == '{' && lookahead(1) == '\"') {
		return Token(Token::STRING, readHereDocument(), curPos);
	}

	// identifier might conflict with keywords and punctuators
	// so we should know the length
	const unsigned int identifierLength = readIdentifierLength();

	// match keywords and punctuator tokens
	for (std::vector<std::pair<std::string, Token::Type> >::iterator it = keywords.begin();
									it != keywords.end(); ++it) {
		const std::string& keyword = it->first;

		bool match = true;
		for (int i = 0, len = keyword.size(); i < len; ++i) {
			if (lookahead(i) != keyword[i]) {
				match = false;
				break;
			}
		}

		if (match) {
			// this is the longest match in keywords, so it is correct that
			// the original string is still longer than keyword, it is actually identifier
			if (keyword.size() < identifierLength) {
				break;
			}

			const bool isPrevWS = isPrevWS_;

			consume(keyword.size());

			if (it->second == Token::STAR) {
				const bool lookaheadIsAlphabet = ('a' <= lookahead() && lookahead() <= 'z')
						 || ('A' <= lookahead() && lookahead() <= 'Z')
						 || ('_' == lookahead());

				return Token(Token::STAR, lookaheadIsAlphabet, curPos);

			} else if (it->second == Token::LBRACK || it->second == Token::LPAREN) {
				return Token(it->second, isPrevWS, curPos);
			} else {
				return Token(it->second, curPos);
			}
		}
	}

	// there's no match, so it is identifier.
	if (identifierLength > 0) {
		std::string name = readIdentifier();
		if ('A' <= name[0] && name[0] <= 'Z') {
			return Token(Token::TYPE_ID, name, curPos);
		} else {
			return Token(Token::ID, name, curPos);
		}
	}


	switch (lookahead()) {
	case '$':
		consume(1);
		return Token(Token::INTEGER, readHexLiteral(), curPos);
	case '0':
		if (lookahead(1) == 'x') {
			consume(2);
			return Token(Token::INTEGER, readHexLiteral(), curPos);
		}
		if (lookahead(1) == 'b') {
			consume(2);
			return Token(Token::INTEGER, readBinaryLiteral(), curPos);
		}
	case '1': case '2': case '3':
	case '4': case '5': case '6':
	case '7': case '8': case '9':
		if (isFloatLiteral())
			return Token(Token::FLOAT, readFloatLiteral(), curPos);
		else
			return Token(Token::INTEGER, readDecimalLiteral(), curPos);

	case '\"':
		return Token(Token::STRING, readCharOrStringLiteral('\"'), curPos);
	case '\'':
		{
			const std::string str = readCharOrStringLiteral('\'');
			if (str.size() != 1)
				throw LexerError(getPosition(), "error: char literal should have one character");
			return Token(Token::CHAR, str[0], curPos);
		}
	case '\\':
		if (lookahead(1) == '\r' || lookahead(1) == '\n') {
			consume(2);
		} else if (lookahead(1) == '\r' && lookahead(2) == '\n') {
			consume(3);
		} else {
			consume(1);
		}
		return getNextToken();
	case '\r':
	case '\n':
		while (lookahead() == '\r' || lookahead() == '\n') {
			consume();
		}
		return Token(Token::TERM, curPos);
	}

	throw LexerError(getPosition(), "error: invalid character");
}

unsigned int Lexer::readIdentifierLength() {
	int len = 0;
	if ('0' <= lookahead(0) && lookahead(0) <= '9')
		return 0;

	while (('a' <= lookahead(len) && lookahead(len) <= 'z')
	 || ('A' <= lookahead(len) && lookahead(len) <= 'Z')
	 || ('0' <= lookahead(len) && lookahead(len) <= '9')
	 || (lookahead(len) == '_')) {
		len++;
	}
	return len;
}
std::string Lexer::readIdentifier() {
	std::string identifier;
	while (('a' <= lookahead() && lookahead() <= 'z')
	 || ('A' <= lookahead() && lookahead() <= 'Z')
	 || ('0' <= lookahead() && lookahead() <= '9')
	 || (lookahead() == '_')) {
		identifier += lookahead();
		consume();
	}
	return identifier;
}

std::string Lexer::readCharOrStringLiteral(char terminator) {
	std::string str;
	Position begPos = getPosition();
	consume();
	while (lookahead() != terminator) {
		if (lookahead() == '\r' || lookahead() == '\n' || lookahead() == 0) {
			throw LexerError(begPos, "error: string literal should be closed");
		}
		if (lookahead() == '\\') {
			switch (lookahead(1)) {
			case 't': str += '\t'; break;
			case 'n': str += '\n'; break;
			case 'r': str += '\r'; break;
			case 'e': str += '\x1b'; break;
			case '\\': str += '\\'; break;
			case '\"': str += '\"'; break;
			default : str += lookahead(1); break;
			}
			consume(2);
		} else {
			str += lookahead();
			consume();
		}
	}
	consume();
	return str;
}

std::string Lexer::readHereDocument() {
	std::string str;
	Position begPos = getPosition();
	consume(2);
	while (lookahead(0) != '\"' || lookahead(1) != '}') {
		if (lookahead() == 0) {
			throw LexerError(begPos, "error: string literal should be closed");
		}
		if (lookahead() == '\\') {
			switch (lookahead(1)) {
			case 't': str += '\t'; break;
			case 'n': str += '\n'; break;
			case 'r': str += '\r'; break;
			case 'e': str += '\x1b'; break;
			case '\\': str += '\\'; break;
			case '\"': str += '\"'; break;
			default : str += lookahead(1); break;
			}
			consume(2);
		} else {
			str += lookahead();
			consume();
		}
	}
	consume(2);
	return str;
}

int Lexer::readHexLiteral() {
	int num = 0;

	bool nothing = true;
	while (('a' <= lookahead() && lookahead() <= 'f')
	 || ('A' <= lookahead() && lookahead() <= 'F')
	 || ('0' <= lookahead() && lookahead() <= '9')) {
		nothing = false;
		num *= 16;

		if ('a' <= lookahead() && lookahead() <= 'f') {
			num += lookahead() - 'a' + 10;

		} else if ('A' <= lookahead() && lookahead() <= 'Z') {
			num += lookahead() - 'A' + 10;

		} else if ('0' <= lookahead() && lookahead() <= '9') {
			num += lookahead() - '0';

		}

		consume();
	}

	if (nothing) {
		throw LexerError(getPosition(), "error: invalid hex literal");
	}

	return num;
}

int Lexer::readDecimalLiteral() {
	int num = 0;

	while ('0' <= lookahead() && lookahead() <= '9') {
		num *= 10;
		num += lookahead() - '0';
		consume();
	}

	return num;
}

int Lexer::readBinaryLiteral() {
	int num = 0;
	bool nothing = true;
	while ('0' == lookahead() || lookahead() == '1') {
		nothing = false;
		num *= 2;
		num += lookahead() - '0';
		consume();
	}
	if (nothing)
		throw LexerError(getPosition(), "error: invalid binary literal");
	return num;
}

bool Lexer::isFloatLiteral() {
	int cnt = 0;
	while ('0' <= lookahead(cnt) && lookahead(cnt) <= '9')
		cnt++;
	return lookahead(cnt) == '.' && '0' <= lookahead(cnt + 1) && lookahead(cnt + 1) <= '9';
}

double Lexer::readFloatLiteral() {
	int integer = readDecimalLiteral();
	consume();
	int fractional = readDecimalLiteral();
	int fracLength = 1;
	while (static_cast<double>(fractional) / fracLength >= 1.0) {
		fracLength *= 10;
	}
	return integer + static_cast<double>(fractional) / fracLength;
}

void Lexer::readSources() {
	std::set<std::string> imported;
	imported.insert(sr_.getMainName());

	std::stack<Stream> streams;
	streams.push(Stream(sr_.open(sr_.getMainName()), sr_.getMainName()));

	streams.push(Stream(sr_.open("peryandefs"), "peryandefs"));

	//breadcrumbs_.push_back(Breadcrumb(0, 0, 0, sr_.getMainName()));
	breadcrumbs_.push_back(Breadcrumb(0, 0, 0, "peryandefs"));

	while (!streams.empty()) {
		Stream& stream = streams.top();
		if (stream.is->eof()) {
			sr_.close(stream.name);
			streams.pop();

			if (source_.size() != 0) {
				source_ += "\n";
			}

			if (!streams.empty()) {
				breadcrumbs_.push_back(
					Breadcrumb(source_.size(), streams.top().pos,
						streams.top().line, streams.top().name));
			}
			continue;
		}

		std::string line;
		getline(*(stream.is), line);

		stream.pos += line.size();
		stream.line++;

		if (line.find("#import") == 0) {
			const size_t start = line.find("\"");
			const size_t finish = line.find("\"", start + 1);

			if (start == std::string::npos || finish == std::string::npos) {
				throw LexerError(-1, "error: invalid import directive");
			}

			const std::string sourceName = line.substr(start + 1, finish - start - 1);

			// the file haven't imported yet
			if (!imported.count(sourceName)) {
				streams.push(Stream(sr_.open(sourceName), sourceName));
				imported.insert(sourceName);

				breadcrumbs_.push_back(Breadcrumb(source_.size(), 0, 0, sourceName));

				//source_ += "\n";
			}
		} else if (line.find("#include") == 0) {
			const size_t start = line.find("\"");
			const size_t finish = line.find("\"", start + 1);

			if (start == std::string::npos || finish == std::string::npos) {
				throw LexerError(-1, "error: invalid include directive");
			}

			const std::string sourceName = line.substr(start + 1, finish - start - 1);

			// don't care whether the file has been imported
			streams.push(Stream(sr_.open(sourceName), sourceName));

			if (!imported.count(sourceName))
				imported.insert(sourceName);

			breadcrumbs_.push_back(Breadcrumb(source_.size(), 0, 0, sourceName));

			//source_ += "\n";
		} else {
			source_ += line;
		}
	}

	// std::cout<<"source_: "<<source_<<std::endl;

	return;
}

void Lexer::getline(std::istream& is, std::string& str) {
	str = "";
	char c;
	while (c = is.get(), !is.eof() && c != 0) {
		str += c;
		if (c == '\n')
			break;
	}
	return;
}

std::string Lexer::getPrettyPrint(Position pos, std::string message) const {
	const Breadcrumb& bc =
		*(std::upper_bound(breadcrumbs_.begin(), breadcrumbs_.end(), pos, Breadcrumb::compare) - 1);

	const std::string& name = bc.name;
	int lineNum = 1 + bc.line;

	Position lastTerm = bc.totalPos - 1;
	for (Position i = bc.totalPos; i < pos; ++i) {
		if (source_[i] == '\n') {
			lineNum++;
			lastTerm = i;
		}
	}
	int posInLine = pos - lastTerm;

	int tabs = 0;
	std::string line;
	for (Position i = lastTerm + 1; i < static_cast<int>(source_.size()) && source_[i] != '\n'; ++i) {
		line += source_[i];
		if (source_[i] == '\t')
			tabs++;
	}

	std::stringstream ss;
	ss<<name<<":"<<lineNum<<":"<<posInLine<<": "<<message<<std::endl;
	ss<<"\t"<<line<<std::endl;
	ss<<"\t"<<std::string(posInLine - 1 - tabs + tabs * 8, ' ')<<"^"<<std::endl;

	return ss.str();
}

std::string LexerError::toString(const Lexer& lexer)
{
	if (position_ == -1) {
		return message_;
	} else {
		return lexer.getPrettyPrint(position_, message_);
	}
}

}
