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

	Token lt(unsigned int n = 0);

	Token::Type la(unsigned int n = 0) {
		return lt(n).getType();
	}

	Position getPosition() { return lt().getPosition(); }

	TransUnit *parseTransUnit();

	Expr *parseExpr(bool allowTopEql = true);

	Identifier *parseIdentifier();
	bool speculateTypeSpec();
	TypeSpec *parseTypeSpec();
	Label *parseLabel();

	Expr *parseXorExpr(bool allowTopEql = true);
	Expr *parseOrExpr(bool allowTopEql = true);
	Expr *parseAndExpr(bool allowTopEql = true);
	Expr *parseEqlExpr(bool allowTopEql = true);
	Expr *parseRelExpr(bool allowTopEql = true);
	Expr *parseShiftExpr(bool allowTopEql = true);
	Expr *parseAddExpr(bool allowTopEql = true);
	Expr *parseMultExpr(bool allowTopEql = true);
	Expr *parseUnaryExpr(bool allowTopEql = true);
	Expr *parsePostfixExpr(bool allowTopEql = true);
	Expr *parsePrimaryExpr(bool allowTopEql = true);
	Expr *parseArrayLiteralExpr();
	Expr *parseFuncExpr();

	std::vector<Stmt *> parseStmt(bool isTopLevel = false, bool withoutTerm = false);
	FuncDefStmt *parseFuncDefStmt();
	std::vector<Stmt *> parseVarDefStmt(bool withoutTerm = false);
	CompStmt *parseCompStmt();
	IfStmt *parseIfStmt(bool withoutTerm = false);
	RepeatStmt *parseRepeatStmt(bool withoutTerm = false);
	LabelStmt *parseLabelStmt(bool withoutTerm = false);
	Stmt *parseGotoGosubStmt(bool withoutTerm = false);
	Stmt *parseInstOrAssignStmt(bool withoutTerm = false);
	ExternStmt *parseExternStmt();
	NamespaceStmt *parseNamespaceStmt();

public:
	Parser(Lexer& lexer, Options& options, WarningPrinter& wp)
		: lexer_(lexer), transUnit_(NULL), symbolTable_(), options_(options), wp_(wp) {};

	void parse();

	TransUnit *getTransUnit() { return transUnit_; }

	SymbolTable& getSymbolTable() { return symbolTable_; }
};

}

#endif
