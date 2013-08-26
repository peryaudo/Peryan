#ifndef PERYAN_PARSER_H__
#define PERYAN_PARSER_H__

#include <deque>

#include "Token.h"
#include "Lexer.h"
#include "AST.h"
#include "SymbolTable.h"

namespace Peryan {

class Options;
class WarningPrinter;

class ParserError : public std::exception {
private:
	Position position_;
	std::string message_;
public:
	ParserError(Position position, std::string message)
		: std::exception(), position_(position), message_(message) {}
	Position getPosition() { return position_; }
	std::string getMessage() { return message_; }
	std::string toString(const Lexer& lexer);

	virtual ~ParserError() throw() {}
};

class Parser {
private:
	Parser(const Parser&);
	Parser& operator=(const Parser&);

private:
	Lexer& lexer_;

	TransUnit *transUnit_;
	SymbolTable symbolTable_;
	std::deque<Token> tokens_;

	Options& options_;
	WarningPrinter& wp_;

	std::deque<int> markers_; // memorize current virtual "top" index in tokens_
	int p_; // lookahead 

	void mark() {
		if (markers_.size() == 0) {
			markers_.push_back(0);
			return;
		} else {
			markers_.push_back(markers_.back());
		}
	}

	bool isSpeculating() {
		return markers_.size() > 0;
	}

	void release() {
		if (markers_.size() > 0) {
			markers_.pop_back();
		}
	}

	void consume(int n = 1) {
		if (markers_.size() > 0) {
			markers_.back() += n;
			return;
		}

		for (int i = 0; i < n; ++i) {
			if (tokens_.size() == 0)
				break;

			if (tokens_.size() == 1 && tokens_.back().getType() == Token::END)
				break;

			tokens_.pop_front();
		}
		return;
	}

	Token lt(int n = 0) throw (LexerError) {
		if (markers_.size() > 0) {
			n += markers_.back();
		}
		while (n >= tokens_.size()) {
			if (tokens_.size() > 0 && tokens_.back().getType() == Token::END) {
				return tokens_.back();
			}

			Token token = lexer_.getNextToken();
			//std::cout<<token.toString()<<std::endl;
			tokens_.push_back(token);
		}
		return tokens_[n];
	}

	Token::Type la(int n = 0) throw (LexerError) {
		return lt(n).getType();
	}

	Position getPosition() throw (LexerError) { return lt().getPosition(); }

	TransUnit *parseTransUnit() throw (LexerError, ParserError);

	Expr *parseExpr(bool allowTopEql = true) throw (LexerError, ParserError);

	Identifier *parseIdentifier() throw (LexerError, ParserError);
	bool speculateTypeSpec() throw (LexerError);
	TypeSpec *parseTypeSpec() throw (LexerError, ParserError);
	Label *parseLabel() throw (LexerError, ParserError);

	Expr *parseXorExpr(bool allowTopEql = true) throw (LexerError, ParserError);
	Expr *parseOrExpr(bool allowTopEql = true) throw (LexerError, ParserError);
	Expr *parseAndExpr(bool allowTopEql = true) throw (LexerError, ParserError);
	Expr *parseEqlExpr(bool allowTopEql = true) throw (LexerError, ParserError);
	Expr *parseRelExpr(bool allowTopEql = true) throw (LexerError, ParserError);
	Expr *parseShiftExpr(bool allowTopEql = true) throw (LexerError, ParserError);
	Expr *parseAddExpr(bool allowTopEql = true) throw (LexerError, ParserError);
	Expr *parseMultExpr(bool allowTopEql = true) throw (LexerError, ParserError);
	Expr *parseUnaryExpr(bool allowTopEql = true) throw (LexerError, ParserError);
	Expr *parsePostfixExpr(bool allowTopEql = true) throw (LexerError, ParserError);
	Expr *parsePrimaryExpr(bool allowTopEql = true) throw (LexerError, ParserError);
	Expr *parseArrayLiteralExpr() throw (LexerError, ParserError);
	Expr *parseFuncExpr() throw (LexerError, ParserError);

	std::vector<Stmt *> parseStmt(bool isTopLevel = false, bool withoutTerm = false) throw (LexerError, ParserError);
	FuncDefStmt *parseFuncDefStmt() throw (LexerError, ParserError);
	std::vector<Stmt *> parseVarDefStmt(bool withoutTerm = false) throw (LexerError, ParserError);
	CompStmt *parseCompStmt() throw (LexerError, ParserError);
	IfStmt *parseIfStmt(bool withoutTerm = false) throw (LexerError, ParserError);
	RepeatStmt *parseRepeatStmt(bool withoutTerm = false) throw (LexerError, ParserError);
	LabelStmt *parseLabelStmt(bool withoutTerm = false) throw (LexerError, ParserError);
	Stmt *parseGotoGosubStmt(bool withoutTerm = false) throw (LexerError, ParserError);
	Stmt *parseInstOrAssignStmt(bool withoutTerm = false) throw (LexerError, ParserError);
	Stmt *parseExternStmt() throw (LexerError, ParserError);

public:
	Parser(Lexer& lexer, Options& options, WarningPrinter& wp)
		: lexer_(lexer), transUnit_(NULL), symbolTable_(), options_(options), wp_(wp) {};

	void parse() throw (LexerError, ParserError, SemanticsError);

	TransUnit *getTransUnit() { return transUnit_; }

	SymbolTable& getSymbolTable() { return symbolTable_; }
};

};

#endif
