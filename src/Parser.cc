// This is a recursive descent implementation of LL(*) parser which parses Peryan grammar.
// Complete EBNF for the grammar is not available now.
#include <cassert>

#include "Token.h"
#include "Lexer.h"
#include "AST.h"
#include "SymbolTable.h"
#include "Options.h"
#include "WarningPrinter.h"
#include "Parser.h"
#include "SymbolRegister.h"
#include "SymbolResolver.h"
#include "TypeResolver.h"

//#define DBG_PRINT(TYPE, FUNC_NAME) std::cout<<#TYPE<<#FUNC_NAME<<" "<<lt().toString()<<std::endl
//#define DBG_PRINT(TYPE, FUNC_NAME) std::cout<<lexer_.getPrettyPrint(lt().getPosition(), #TYPE #FUNC_NAME)
//#define DBG_PRINT(TYPE, FUNC_NAME) if (*(#TYPE) == '+') \
//	std::cout<<lexer_.getPrettyPrint(lt().getPosition(), #TYPE #FUNC_NAME)


#ifndef DBG_PRINT
#define DBG_PRINT(TYPE, FUNC_NAME)
#endif

namespace Peryan {

std::string ParserError::toString(const Lexer& lexer) {
	return lexer.getPrettyPrint(position_, message_);
}

void Parser::parse() throw (LexerError, ParserError, SemanticsError) {
	if (options_.verbose) std::cerr<<"parsing...";
	transUnit_ = parseTransUnit();
	if (options_.verbose) std::cerr<<"ok."<<std::endl;

	if (options_.verbose) std::cerr<<"registering symbols...";
	SymbolRegister symRegister(getSymbolTable());
	symRegister.visit(getTransUnit());
	if (options_.verbose) std::cerr<<"ok."<<std::endl;

	if (options_.verbose) std::cerr<<"resolving symbols...";
	SymbolResolver symResolver(getSymbolTable());
	symResolver.visit(getTransUnit());
	if (options_.verbose) std::cerr<<"ok."<<std::endl;

	if (options_.verbose) std::cerr<<"resolving types...";
	TypeResolver typeResolver(getSymbolTable());
	typeResolver.visit(getTransUnit());
	if (options_.verbose) {
		std::cerr<<"ok."<<std::endl;
		std::cerr<<"parsing finished."<<std::endl;
	}

	return;
}

// TranslationUnit : { TopLevelStatement }
TransUnit *Parser::parseTransUnit() throw (LexerError, ParserError) {
	DBG_PRINT(+, parseTransUnit);

	TransUnit *transUnit = new TransUnit();

	std::vector<Stmt *> curStmts;
	while (curStmts = parseStmt(true), curStmts.size() > 0) {
		transUnit->stmts.insert(transUnit->stmts.end(), curStmts.begin(), curStmts.end());
	}

	DBG_PRINT(-, parseTransUnit);
	return transUnit;
}

// Statement : VariableDefinition
// 	     | InstructionAndAssignmentStatement
// 	     | CompoundStatement TERM
// 	     | IfStatement
// 	     | RepeatStatement
// 	     | WhileStatement //*
// 	     | DoStatement //*
// 	     | ForStatement //*
// 	     | SwitchStatement //*
// 	     | LabelStatement
// 	     | JumpStatement
// 	     | TERM
// 	     ;
// *: not yet implemented
//
// JumpStatement : "goto" Label (":" | TERM)
// 	         | "gosub" Label (":" | TERM)
// 	         | "continue"  (":" | TERM)
// 	         | "swbreak"  (":" | TERM) //*
// 	         | "break"  (":" | TERM)
// 	         ;
std::vector<Stmt *> Parser::parseStmt(bool isTopLevel, bool withoutTerm) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseStmt);

	if (!withoutTerm)
		while (la() == Token::TERM)
			consume();

	if (la() == Token::END)
		return std::vector<Stmt *>();

	if (isTopLevel) {
		assert(!withoutTerm);

		if (la(0) == Token::KW_FUNC && la(1) == Token::ID) {
			return std::vector<Stmt *>(1, parseFuncDefStmt());
		} else if (la(0) == Token::KW_EXTERN) {
			return std::vector<Stmt *>(1, parseExternStmt());
		} else if (la(0) == Token::KW_NAMESPACE) {
			return std::vector<Stmt *>(1, parseNamespaceStmt());
		}
	}

	Stmt *stmt = NULL;

	switch (la(0)) {
	case Token::KW_VAR	:
		return parseVarDefStmt(withoutTerm);

	case Token::LBRACE	:
		stmt = parseCompStmt();
		if (!withoutTerm) {
			if (la() != Token::TERM)
				throw ParserError(getPosition(), "error: no terminal character");
			consume();
		}
		break;

	case Token::KW_IF	: stmt = parseIfStmt(withoutTerm); break;
	case Token::KW_REPEAT	: stmt = parseRepeatStmt(withoutTerm); break;
	case Token::STAR	: stmt = parseLabelStmt(withoutTerm); break;

	// jump statements
	case Token::KW_GOTO	:
	case Token::KW_GOSUB	:
		stmt = parseGotoGosubStmt(withoutTerm);
		break;

	case Token::KW_CONTINUE :
		stmt = new ContinueStmt(lt(0));
		consume();
		if (!withoutTerm) {
			if (la() != Token::TERM && la() != Token::CLN)
				throw ParserError(getPosition(), "error: no terminal character");
			consume();
		}
		break;

	case Token::KW_BREAK	:
		stmt = new BreakStmt(lt(0));
		consume();
		if (!withoutTerm) {
			if (la() != Token::TERM && la() != Token::CLN)
				throw ParserError(getPosition(), "error: no terminal character");
			consume();
		}
		break;

	case Token::KW_RETURN	:
		{
			Token token = lt(0);
			consume();

			Expr *expr = NULL;
			if (la() != Token::TERM && la() != Token::CLN)
				expr = parseExpr();

			stmt = new ReturnStmt(token, expr);

			if (!withoutTerm) {
				if (la() != Token::TERM && la() != Token::CLN)
					throw ParserError(getPosition(), "error: no terminal character");
				consume();
			}

			break;
		}


	// assume instruction statement or assignment statement
	// to make the parser LL(k) (neither backtrack nor packrat)
	default			: stmt = parseInstOrAssignStmt(withoutTerm); break;
	}

	assert(stmt != NULL);

	DBG_PRINT(-, parseStmt);
	return std::vector<Stmt *>(1, stmt);
}

// FunctionDefinition : "func" IDENTIFIER  "(" ParameterDeclarationList? ")"
// 					[ "::" TypeSpecifier ] CompoundStatement TERM ;
FuncDefStmt *Parser::parseFuncDefStmt() throw (LexerError, ParserError) {
	DBG_PRINT(+, parseFuncDefStmt);
	assert(la() == Token::KW_FUNC);

	Token token = lt();
	consume();

	if (la() != Token::ID)
		throw ParserError(getPosition(), "error: identifier expected");
	Identifier *name = parseIdentifier();

	if (la() != Token::LPAREN)
		throw ParserError(getPosition(), "error: '(' expected");
	consume();

	std::vector<Identifier *> params;
	bool first = true;
	while (la() != Token::RPAREN) {
		if (la() == Token::TERM)
			throw ParserError(getPosition(), "error: no closing ')' in the line");

		if (!first && la() == Token::COMMA)
			consume();
		first = false;

		if (la() != Token::ID)
			throw ParserError(getPosition(), "error: identifier expected");

		Identifier *curParam = parseIdentifier();

		// TODO: will be optional
		if (la() != Token::CLNCLN)
			throw ParserError(getPosition(), "error: STUB: type inference not implemented yet");
		consume();

		curParam->typeSpec = parseTypeSpec();

		params.push_back(curParam);
	}

	consume();

	if (la() != Token::CLNCLN)
		throw ParserError(getPosition(), "error: '::' expected");
	consume();

	TypeSpec *retType = parseTypeSpec();

	if (la() != Token::LBRACE)
		throw ParserError(getPosition(), "error: '{' expected");

	CompStmt *body = parseCompStmt();

	if (la() != Token::TERM)
		throw ParserError(getPosition(), "error: no terminal character");
	consume();


	FuncDefStmt *fds = new FuncDefStmt(token, name, body);
	fds->params = params;
	fds->retTypeSpec = retType;

	DBG_PRINT(-, parseFuncDefStmt);
	return fds;
}

// ExternStmt : "extern" ID "::" TypeSpec (":" | TERM)
ExternStmt *Parser::parseExternStmt() throw (LexerError, ParserError) {
	DBG_PRINT(+, parseExternStmt);
	assert(la() == Token::KW_EXTERN);

	Token token = lt();
	consume();

	ExternStmt *es = new ExternStmt(token, parseIdentifier());

	if (la() != Token::CLNCLN)
		throw ParserError(getPosition(), "error: '::' expected");
	consume();

	es->id->typeSpec = parseTypeSpec();

	if (la() != Token::TERM && la() != Token::CLN)
		throw ParserError(getPosition(), "error: no terminal character");
	consume();

	DBG_PRINT(-, parseExternStmt);
	return es;
}

// JumpStatement : "goto" Label (":" | TERM)
// 	         | "gosub" Label (":" | TERM)
Stmt *Parser::parseGotoGosubStmt(bool withoutTerm) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseGotoGosubStmt);
	assert(la() == Token::KW_GOTO || la() == Token::KW_GOSUB);

	Stmt *stmt = NULL;
	const Token gotoGosub = lt();
	consume();

	if (la() != Token::STAR)
		throw ParserError(getPosition(), "error: '*' expected");
	Label *label = parseLabel();

	if (gotoGosub.getType() == Token::KW_GOTO) {
		stmt = new GotoStmt(gotoGosub, label);
	} else {
		stmt = new GosubStmt(gotoGosub, label);
	}

	if (!withoutTerm) {
		if (la() != Token::TERM && la() != Token::CLN)
			throw ParserError(getPosition(), "error: no terminal character");
		consume();
	}

	DBG_PRINT(-, parseGotoGosubStmt);
	return stmt;
}

// Identifier : (ID | "Int" | "String" | "Char" | "Float" | "Double" | "Bool") ;
Identifier *Parser::parseIdentifier() throw (LexerError, ParserError) {
	DBG_PRINT(+, parseIdentifier);
	assert(la() == Token::ID);

	Identifier *id = new Identifier(lt());
	/*if (la() == Token::ID) {
		id = new Identifier(lt());
	 } else if (la() == Token::KW_INT || la() == Token::KW_STRING
			|| la() == Token::KW_CHAR || la() == Token::KW_FLOAT
			|| la() == Token::KW_DOUBLE || la() == Token::KW_BOOL) {
		id = new Keyword(lt());
	} else {
		assert(false);
	}*/
	consume();

	DBG_PRINT(-, parseIdentifier);
	return id;
}

// Label : "*" ID ;
Label *Parser::parseLabel() throw (LexerError, ParserError) {
	DBG_PRINT(+, parseLabel);
	assert(la() == Token::STAR);

	if (la() == Token::STAR && (lt().hasTrailingAlphabet())) {
		consume();

		Label *label = new Label(lt());
		consume();

		DBG_PRINT(-, parseLabel);
		return label;
	} else {
		throw ParserError(getPosition(), "error: label expected");
	}
}

// CompoundStatement : "{" { Statement } "}" ;
CompStmt *Parser::parseCompStmt() throw (LexerError, ParserError) {
	DBG_PRINT(+, parseCompStmt);
	assert(la() == Token::LBRACE);

	CompStmt *compStmt = new CompStmt(lt());
	consume();

	while (la() != Token::RBRACE) {
		if (la() == Token::END)
			throw ParserError(getPosition(), "error: no closing '}'");
		while (la() == Token::TERM)
			consume();
		std::vector<Stmt *> curStmts = parseStmt(false);
		assert(curStmts.size() != 0);

		compStmt->stmts.insert(compStmt->stmts.end(), curStmts.begin(), curStmts.end());
	}

	// la() == Token::RBRACE
	consume();

	DBG_PRINT(-, parseCompStmt);
	return compStmt;
}

// LabelStatement : Label TERM ;
LabelStmt *Parser::parseLabelStmt(bool withoutTerm) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseLabelStmt);
	assert(la() == Token::STAR);

	Token token = lt();

	Label *label = parseLabel();
	
	if (!withoutTerm) {
		if (la() != Token::TERM)
			throw ParserError(getPosition(), "error: no terminal character");
		consume();
	}

	DBG_PRINT(-, parseLabelStmt);
	return new LabelStmt(token, label);
}

//VariableDefinition : "var" IDENTIFIER [ "::" TypeSpecifier ] [ "=" Expression ]
//	{ "," IDENTIFIER [ "::" TypeSpecifier ] [ "=" Expression ] } (":" | TERM) ;
std::vector<Stmt *> Parser::parseVarDefStmt(bool withoutTerm) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseVarDefStmt);
	assert(la() == Token::KW_VAR);

	consume();

	std::vector<Stmt *> stmts;

	bool first = true;
	while (la() != Token::TERM && la() != Token::CLN) {
		if (!first && la() == Token::COMMA)
			consume();
		first = false;

		if (la() != Token::ID)
			throw ParserError(getPosition(), "error: identifier expected");

		Identifier *id = parseIdentifier();

		if (la() == Token::CLNCLN) {
			consume();

			id->typeSpec = parseTypeSpec();
		} else {
			id->typeSpec = NULL;
		}

		Expr *init = NULL;
		if (la() == Token::EQL) {
			consume();

			init = parseExpr();
		}
		stmts.push_back(new VarDefStmt(id->token, id, init));
	}

	if (!withoutTerm) {
		consume();
	}

	DBG_PRINT(-, parseVarDefStmt);
	return stmts;
}

//TypeSpecifier : "(" TypeSpecifier ")"
//		| {"const" | "ref"}
//			( "[" TypeSpecifier "]"
//	 		| TYPE_ID "->" TypeSpecifier // Right-to-left
//	 		| TypeSpecifier "." TYPE_ID // Left-to-right
//	 		| TYPE_ID )
// 		;

bool Parser::speculateTypeSpec() throw (LexerError) {
	mark();

	bool isSuccess = true;
	try {
		parseTypeSpec();
	} catch (ParserError pe) {
		isSuccess = false;
	}

	release();
	return isSuccess;
}

TypeSpec *Parser::parseTypeSpec() throw (LexerError, ParserError) {
	DBG_PRINT(+, parseTypeSpec);

	bool isConst = false, isRef = false;

	while (la() == Token::KW_CONST || la() == Token::KW_REF) {
		if (la() == Token::KW_CONST) {
			isConst = true;
		} else if (la() == Token::KW_REF) {
			isRef = true;
		}

		consume();
	}

	TypeSpec *typeSpec = NULL, *first = NULL;
	switch (la()) {
	case Token::TYPE_ID:
		if (!isSpeculating())
			typeSpec = new TypeSpec(lt(), isConst, isRef);
		consume();

		first = typeSpec;
		break;
	case Token::LBRACK:
		{
			consume();

			TypeSpec *elm = parseTypeSpec();
			if (!isSpeculating())
				typeSpec = new ArrayTypeSpec(isConst, isRef, elm);

			if (la() != Token::RBRACK)
				throw ParserError(getPosition(), "error: no closing ']'");
			consume();

			if (!isSpeculating())
				first = typeSpec;
			break;
		}
	case Token::LPAREN:
		consume();

		typeSpec = parseTypeSpec();

		if (la() != Token::RPAREN)
			throw ParserError(getPosition(), "error: no closing ')'");
		consume();

		break;
	default:
		throw ParserError(getPosition(), "error: invalid or unsupported type specifier");
	}

	if (la() == Token::RARROW) {
		consume();

		if (!isSpeculating()) {
			if (first != NULL) {
				first->isConst = false;
				first->isRef = false;
			}
		}

		TypeSpec* rhs = parseTypeSpec();
		if (!isSpeculating()) {
			typeSpec = new FuncTypeSpec(isConst, isRef, typeSpec, rhs);
		}
	}

	DBG_PRINT(-, parseTypeSpec);
	return typeSpec;
}

// IfStatement : "if" Expression CompoundStatement
// 		{ "else" {":"} "if" Expression CompoundStatement }
// 		[ "else" CompoundStatement ] (":" | TERM)
//	       | "if" Expression ":" StatementWithoutTerm { ":" StatementWithoutTerm }
//		 [":" "else" ":" StatementWithoutTerm { ":" StatementWithoutTerm } ] TERM ; //*

IfStmt *Parser::parseIfStmt(bool withoutTerm) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseIfStmt);

	assert(la() == Token::KW_IF);

	Token token = lt();
	consume();

	Expr *ifCond = parseExpr();
	CompStmt *ifThen = NULL;
	std::vector<Expr *> elseIfCond;
	std::vector<CompStmt *> elseIfThen;
	CompStmt *elseThen = NULL;

	if (la() == Token::LBRACE) {
		// "if" Expression CompoundStatement
		// { "else" {":"} "if" Expression CompoundStatement }
		// [ "else" CompoundStatement ] (":" | TERM)
		ifThen = parseCompStmt();

		while ((la(0) == Token::KW_ELSE && la(1) == Token::KW_IF)
			|| (la(0) == Token::KW_ELSE && la(1) == Token::CLN && la(2) == Token::KW_IF)) {
			if (la(1) == Token::CLN) {
				consume(3);
			} else {
				consume(2);
			}

			elseIfCond.push_back(parseExpr());

			if (la() != Token::LBRACE)
				throw ParserError(getPosition(), "error: no closing '}'");

			elseIfThen.push_back(parseCompStmt());
		}

		if (la() == Token::KW_ELSE) {
			consume();

			if (la() != Token::LBRACE)
				throw ParserError(getPosition(), "error: no closing '}'");

			elseThen = parseCompStmt();
		}

		if (!withoutTerm) {
			if (la() != Token::TERM && la() != Token::CLN)
				throw ParserError(getPosition(), "error: no terminal character");
			consume();
		}
	} else if (la() == Token::CLN) {
		// "if" Expression ":" StatementWithoutTerm { ":" StatementWithoutTerm }
		//	[":" "else" ":" StatementWithoutTerm { ":" StatementWithoutTerm } ] TERM ; //*
		ifThen = new CompStmt(lt());
		consume();

		while (la() != Token::TERM && la() != Token::KW_ELSE) {
			std::vector<Stmt *> curStmts = parseStmt(false, true);
			if (la() == Token::END)
				throw ParserError(getPosition(), "error: no terminal character");
			if (la() == Token::CLN) {
				consume();
			}
			ifThen->stmts.insert(ifThen->stmts.end(), curStmts.begin(), curStmts.end());
		}

		if (la() == Token::KW_ELSE) {
			elseThen = new CompStmt(lt());
			consume();

			if (la() != Token::CLN) {
				throw ParserError(getPosition(), "error: no terminal character");
			}
			consume();

			while (la() != Token::TERM) {
				if (la() == Token::END)
					throw ParserError(getPosition(), "error: no closing '\\n'");
				std::vector<Stmt *> curStmts = parseStmt(false, true);
				if (la() == Token::CLN)
					consume();
				elseThen->stmts.insert(elseThen->stmts.end(), curStmts.begin(), curStmts.end());
			}
		}

		assert(la() == Token::TERM);
		if (!withoutTerm)
			consume();
	} else {
		throw ParserError(getPosition(), "error: invalid if statement");
	}

	IfStmt *ifStmt = new IfStmt(token, ifCond, ifThen, elseThen);
	ifStmt->elseIfCond = elseIfCond;
	ifStmt->elseIfThen = elseIfThen;

	DBG_PRINT(-, parseIfStmt);
	return ifStmt;
}

// RepeatStatement : "repeat" { Expression } (":" | TERM) { Statement } "loop" (":" | TERM) ;
RepeatStmt *Parser::parseRepeatStmt(bool withoutTerm) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseRepeatStmt);
	assert(la() == Token::KW_REPEAT);

	Token token = lt();

	consume();

	Expr *count = NULL;
	if (la() == Token::CLN || la() == Token::TERM) {
		consume();

		count = NULL;
	} else {
		count = parseExpr();

		if (la() != Token::CLN && la() != Token::TERM)
			throw ParserError(getPosition(), "error: no terminal character");
	}
	
	RepeatStmt *rs = new RepeatStmt(token, count);

	while (la() != Token::KW_LOOP) {
		if (la() == Token::END)
			throw ParserError(getPosition(), "error: no closing 'loop'");

		while (la() == Token::TERM)
			consume();
		std::vector<Stmt *> stmts = parseStmt(false);
		if (stmts.size() == 0)
			throw ParserError(getPosition(), "error: no closing 'loop'");

		rs->stmts.insert(rs->stmts.end(), stmts.begin(), stmts.end());
	}

	consume();

	if (!withoutTerm) {
		if (la() != Token::TERM && la() != Token::CLN)
			throw ParserError(getPosition(), "error: no terminal character");
		consume();
	}

	DBG_PRINT(-, parseRepeatStmt);
	return rs;
}
// InstructionAndAssignmentStatement :
//	ExpressionWithoutTopLevelEqual (InstructionStatement' | AssignmentStatement')
// InstructionStatement' : ParameterList? (":" | TERM) ;
// ParameterList : Expression { "," Expression } ;

// AssignmentStatement' : AssignmentOperator [ Expression ] (":" | TERM) ;
// AssignmentOperator : "=" | "++" | "--" | "+=" | "-=" | "*=" | "/=" ;

Stmt *Parser::parseInstOrAssignStmt(bool withoutTerm) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseInstOrAssignStmt);

	Token topToken = lt();
	Expr *topExpr = parseExpr(false);

	switch (la()) {
	case Token::PLUSPLUS:
	case Token::MINUSMINUS:
		{
			Token token = lt();
			consume();

			if (!withoutTerm) {
				if (la() != Token::TERM && la() != Token::CLN)
					throw ParserError(getPosition(), "error: no terminal character");
				consume();
			}

			return new AssignStmt(topExpr, token, NULL);
		}
	case Token::EQL:
	case Token::PLUSEQ:
	case Token::MINUSEQ:
	case Token::STAREQ:
	case Token::SLASHEQ:
		{
			Token token = lt();
			consume();

			Expr *rhs = parseExpr();

			if (!withoutTerm) {
				if (la() != Token::TERM && la() != Token::CLN)
					throw ParserError(getPosition(), "error: no terminal character");
				consume();
			}

			return new AssignStmt(topExpr, token, rhs);
		}
	default: ;
	}

	InstStmt *instStmt = new InstStmt(topToken, topExpr);

	bool first = true;
	while (la() != Token::TERM && la() != Token::CLN) {
		if (la() == Token::END)
			throw ParserError(getPosition(), "error: no terminal character");

		if (!first && la() == Token::COMMA)
			consume();
		first = false;

		instStmt->params.push_back(parseExpr());
	}

	if (!withoutTerm)
		consume();

	DBG_PRINT(-, parseInstOrAssignStmt);
	return instStmt;
}

// Expression : XorExpression ;
Expr *Parser::parseExpr(bool allowTopEql) throw (LexerError, ParserError) {
	DBG_PRINT(+-, parseExpr);
	return parseXorExpr(allowTopEql);
}

// XorExpression : OrExpression { "^" OrExpression } ; 
// Associativity: Left-to-right
Expr *Parser::parseXorExpr(bool allowTopEql) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseXorExpr);

	Expr *lhs = parseOrExpr(allowTopEql);

	while (la() == Token::CARET) {
		Token token = lt();
		consume();

		Expr *rhs = parseOrExpr(true);

		lhs = new BinaryExpr(lhs, token, rhs);
	}

	DBG_PRINT(-, parseXorExpr);
	return lhs;
}

// OrExpression : AndExpression { "|" AndExpression } ;
// Associativity: Left-to-right
Expr *Parser::parseOrExpr(bool allowTopEql) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseOrExpr);

	Expr *lhs = parseAndExpr(allowTopEql);

	while (la() == Token::PIPE) {
		Token token = lt();
		consume();

		Expr *rhs = parseAndExpr(true);

		lhs = new BinaryExpr(lhs, token, rhs);
	}

	DBG_PRINT(-, parseOrExpr);
	return lhs;
}

// AndExpression : EqualityExpression { "&" EqualityExpression } ;
// Associativity: Left-to-right
Expr *Parser::parseAndExpr(bool allowTopEql) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseAndExpr);

	Expr *lhs = parseEqlExpr(allowTopEql);

	while (la() == Token::AMP) {
		Token token = lt();
		consume();

		Expr *rhs = parseEqlExpr(true);

		lhs = new BinaryExpr(lhs, token, rhs);
	}

	DBG_PRINT(-, parseAndExpr);
	return lhs;
}

// EqualityExpression : RelationalExpression { ("=" | "==" | "!=" | "!") RelationalExpression } ;
// Associativity: Left-to-right
Expr *Parser::parseEqlExpr(bool allowTopEql) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseEqlExpr);

	Expr *lhs = parseRelExpr(allowTopEql);

	while (la() == Token::EQL || la() == Token::EQEQ
		|| la() == Token::EXCLEQ || la() == Token::EXCL) {

		if (la() == Token::EQL && !allowTopEql)
			return lhs;

		Token token = lt();
		consume();

		Expr *rhs = parseRelExpr(true);

		lhs = new BinaryExpr(lhs, token, rhs);
	}

	DBG_PRINT(-, parseEqlExpr);
	return lhs;
}

// RelationalExpression : ShiftExpression [ ("<" | "<=" | ">" | ">=" ) ShiftExpression ] ;
// Associativity: Left-to-right
Expr *Parser::parseRelExpr(bool allowTopEql) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseRelExpr);

	Expr *lhs = parseShiftExpr(allowTopEql);

	while (la() == Token::LT || la() == Token::LTEQ
		|| la() == Token::GT || la() == Token::GTEQ) {
		Token token = lt();
		consume();

		Expr *rhs = parseShiftExpr(true);

		lhs = new BinaryExpr(lhs, token, rhs);
	}

	DBG_PRINT(-, parseRelExpr);
	return lhs;
}

// ShiftExpression : AdditiveExpression { ("<<" | ">>" ) AdditiveExpression } ;
// Associativity: Left-to-right
Expr *Parser::parseShiftExpr(bool allowTopEql) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseShiftExpr);

	Expr *lhs = parseAddExpr(allowTopEql);

	while (la() == Token::LTLT || la() == Token::GTGT) {
		Token token = lt();
		consume();

		Expr *rhs = parseAddExpr(true);

		lhs = new BinaryExpr(lhs, token, rhs);
	}

	DBG_PRINT(-, parseShiftExpr);
	return lhs;
}

// AdditiveExpression: MultiplicativeExpression { ("+" | "-") MultiplicativeExpression } ;
// Associativity: Left-to-right
Expr *Parser::parseAddExpr(bool allowTopEql) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseAddExpr);

	Expr *lhs = parseMultExpr(allowTopEql);

	while (la() == Token::PLUS || la() == Token::MINUS) {
		Token token = lt();
		consume();

		Expr *rhs = parseMultExpr(true);

		lhs = new BinaryExpr(lhs, token, rhs);
	}

	DBG_PRINT(-, parseAddExpr);
	return lhs;
}

// MultiplicativeExpression : UnaryExpression { ("*" | "/" | "%") UnaryExpression } ;
// Associativity: Left-to-right
Expr *Parser::parseMultExpr(bool allowTopEql) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseMultExpr);

	Expr *lhs = parseUnaryExpr(allowTopEql);

	while (la() == Token::STAR || la() == Token::SLASH || la() == Token::PERC) {
		Token token = lt();
		consume();

		Expr *rhs = parseUnaryExpr(true);

		lhs = new BinaryExpr(lhs, token, rhs);
	}

	DBG_PRINT(-, parseMultExpr);
	return lhs;
}

// UnaryExpression : ("!" | "+" | "-")  UnaryExpression
// 		   | PostfixExpression
// 		   ;
// Associativity: Right-to-left
Expr *Parser::parseUnaryExpr(bool allowTopEql) throw (LexerError, ParserError) {
	DBG_PRINT(+, parseUnaryExpr);

	if (la() == Token::EXCL || la() == Token::PLUS || la() == Token::MINUS) {
		Token token = lt();
		consume();
		DBG_PRINT(-, parseUnaryExpr);
		return new UnaryExpr(token, parseUnaryExpr(true));
	} else {
		DBG_PRINT(-, parseUnaryExpr);
		return parsePostfixExpr(allowTopEql);
	}
}

// PostfixExpression : (PrimaryExpression | {"partial"} FunctionExpression)
// 		{ '[' Expression ']' | '(' Expression ')' | '.' IDENTIFIER } ;
// Associativity: Left-to-right
Expr *Parser::parsePostfixExpr(bool allowTopEql) throw (LexerError, ParserError) {
	DBG_PRINT(+, parsePostfixExpr);

	Expr *expr = NULL;

	bool partial = false;
	Position partialPosition = 0;
	if (la() == Token::KW_PARTIAL) {
		partial = true;
		partialPosition = lt().getPosition();

		consume();
	}

	// speculate it will be correct TypeSpec (LL(*) parsing!)
	if (speculateTypeSpec()) {
		TypeSpec *ts = parseTypeSpec();
		if (la() == Token::LPAREN) {
			Token token = lt();
			consume();

			std::vector<Expr *> params;

			bool first = true;
			while (la() != Token::RPAREN) {
				if (la() == Token::END || la() == Token::TERM)
					throw ParserError(getPosition(), "error: no closing ')'");

				if (!first && la() == Token::COMMA)
					consume();
				first = false;

				params.push_back(parseExpr(true));
			}

			consume();

			ConstructorExpr *ce = new ConstructorExpr(token, ts);
			ce->params = params;

			DBG_PRINT(+, parsePostfixExpr);
			return ce;
		} else if (la() == Token::DOT) {
			Token token = lt();
			consume();

			if (la() != Token::ID)
				throw ParserError(getPosition(), "error: identifier expected");
			Identifier *id = parseIdentifier();

			expr = new StaticMemberExpr(ts, token, id);
		} else {
			throw ParserError(getPosition(), "error: '(' or '.' expected");
		}
	} else {
		if (la() == Token::KW_FUNC) {
			expr = parseFuncExpr();
		} else {
			expr = parsePrimaryExpr(allowTopEql);
		}
	}

	Token token = lt();

	while (true) {
		if (la() == Token::LBRACK && (allowTopEql ? true : !(lt().hasWSBefore()))) {
			consume();

			Expr *subscr = parseExpr(true);

			if (la() != Token::RBRACK)
				throw ParserError(getPosition(), "error: no closing ']'");
			consume();

			expr = new SubscrExpr(expr, token, subscr);
		} else if (la() == Token::LPAREN && (allowTopEql ? true : !(lt().hasWSBefore()))) {
			consume();

			std::vector<Expr *> params;

			bool first = true;
			while (la() != Token::RPAREN) {
				if (la() == Token::END || la() == Token::TERM)
					throw ParserError(getPosition(), "error: no closing ')'");

				if (!first && la() == Token::COMMA)
					consume();
				first = false;

				params.push_back(parseExpr(true));
			}

			consume();

			FuncCallExpr *fce = new FuncCallExpr(token, expr);
			fce->params = params;
			fce->partial = partial;

			expr = fce;
			partial = false;
		} else if (la() == Token::DOT) {
			consume();

			if (la() != Token::ID)
				throw ParserError(getPosition(), "error: identifier expected");
			Identifier *id = parseIdentifier();

			expr = new MemberExpr(expr, token, id);
		} else {
			break;
		}
	}

	if (partial) {
		throw ParserError(partialPosition, "error: this is not function applying");
	}

	DBG_PRINT(-, parsePostfixExpr);
	return expr;
}

// FunctionExpression: "func" "(" ParameterDeclarationList? ")" [ "::" TypeSpecifier ] CompoundStatement ; //*
Expr *Parser::parseFuncExpr() throw (LexerError, ParserError) {
	assert(la() == Token::KW_FUNC);
	Token token = lt();
	consume();

	if (la() != Token::LPAREN)
		throw ParserError(getPosition(), "error: '(' expected");
	consume();

	std::vector<Identifier *> params;
	bool first = true;
	while (la() != Token::RPAREN) {
		if (la() == Token::TERM)
			throw ParserError(getPosition(), "error: no closing ')' in the line");

		if (!first && la() == Token::COMMA)
			consume();
		first = false;

		if (la() != Token::ID)
			throw ParserError(getPosition(), "error: identifier expected");

		Identifier *curParam = parseIdentifier();

		// TODO: will be optional
		if (la() != Token::CLNCLN)
			throw ParserError(getPosition(), "error: STUB: type inference not implemented yet");
		consume();

		curParam->typeSpec = parseTypeSpec();

		params.push_back(curParam);
	}

	consume();

	if (la() != Token::CLNCLN)
		throw ParserError(getPosition(), "error: '::' expected");
	consume();

	TypeSpec *retType = parseTypeSpec();

	if (la() != Token::LBRACE)
		throw ParserError(getPosition(), "error: '{' expected");

	CompStmt *body = parseCompStmt();

	FuncExpr *fe = new FuncExpr(token, body);
	fe->params = params;
	fe->retTypeSpec = retType;

	return fe;
}

// PrimaryExpression : Literal
// 		     | "this" //*
// 		     | "(" Expression ")"
// 		     | IDENTIFIER
// 		     | Label
// 		     ;
Expr *Parser::parsePrimaryExpr(bool allowTopEql) throw (LexerError, ParserError) {
	DBG_PRINT(+, parsePrimaryExpr);

	Expr *expr = NULL;
	switch (la()) {
	case Token::INTEGER:
		expr = new IntLiteralExpr(lt());
		consume();
		break;
	case Token::FLOAT:
		expr = new FloatLiteralExpr(lt());
		consume();
		break;
	case Token::STRING:
		expr = new StrLiteralExpr(lt());
		consume();
		break;
	case Token::CHAR:
		expr = new CharLiteralExpr(lt());
		consume();
		break;
	case Token::KW_TRUE:
	case Token::KW_FALSE:
		expr = new BoolLiteralExpr(lt());
		consume();
		break;
	case Token::LBRACK:
		expr = parseArrayLiteralExpr();
		break;
	case Token::LPAREN:
		consume();

		expr = parseExpr(true);

		if (la() != Token::RPAREN)
			throw ParserError(getPosition(), "error: no closing ')'");
		consume();

		break;
	case Token::ID:
	/* case Token::KW_INT:
	case Token::KW_STRING:
	case Token::KW_CHAR:
	case Token::KW_FLOAT:
	case Token::KW_DOUBLE:
	case Token::KW_BOOL:*/
		DBG_PRINT(-, parsePrimaryExpr);
		return parseIdentifier();
	case Token::STAR:
		DBG_PRINT(-, parsePrimaryExpr);
		return parseLabel();
	default: throw ParserError(getPosition(), "error: invalid primary expression");
	}

	DBG_PRINT(-, parsePrimaryExpr);
	return expr;
}

Expr *Parser::parseArrayLiteralExpr() throw (LexerError, ParserError) {
	DBG_PRINT(+, parseArrayLiteralExpr);
	assert(la() == Token::LBRACK);

	ArrayLiteralExpr *ale = new ArrayLiteralExpr(lt());

	consume();

	bool first = true;
	while (la() != Token::RBRACK) {
		if (la() == Token::END || la() == Token::TERM)
			throw ParserError(getPosition(), "error: no closing ']'");

		if (!first && la() == Token::COMMA)
			consume();
		first = false;

		ale->elements.push_back(parseExpr(true));
	}

	consume();

	DBG_PRINT(-, parseArrayLiteralExpr);
	return ale;
}

NamespaceStmt *Parser::parseNamespaceStmt() throw (LexerError, ParserError) {
	DBG_PRINT(+, parseNamespaceStmt);

	assert(la() == Token::KW_NAMESPACE);
	Token token = lt();
	consume();

	TypeSpec *name = parseTypeSpec();
	if (name->getASTType() != AST::TYPE_SPEC) {
		throw ParserError(name->token.getPosition(), "error: you can't use this name as a namespace");
	}

	NamespaceStmt *ns = new NamespaceStmt(token, name);

	if (la() != Token::LBRACE)
		throw ParserError(getPosition(), "error: '{' expected");
	consume();

	while (la() != Token::RBRACE) {
		if (la() == Token::END)
			throw ParserError(getPosition(), "error: no closing '}'");

		while (la() == Token::TERM)
			consume();
		std::vector<Stmt *> curStmts = parseStmt(true);
		assert(curStmts.size() != 0);

		ns->stmts.insert(ns->stmts.end(), curStmts.begin(), curStmts.end());
	}

	// la() == Token::RBRACE
	consume();

	DBG_PRINT(-, parseNamespaceStmt);

	return ns;
}


};

