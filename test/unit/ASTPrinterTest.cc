#include "gtest/gtest.h"

#include "../../src/AST.h"
#include "../../src/ASTPrinter.h"

namespace {

TEST(ASTPRinterTest, SimpleTransUnit) {
	using namespace Peryan;

	ASTPrinter printer;
	TransUnit *tu = new TransUnit();

	Token fooToken(Token::ID, std::string("foo"), 0);
	FuncCallExpr *inst =  new FuncCallExpr(fooToken, new Identifier(fooToken));
	inst->params.push_back(new Identifier(Token(Token::ID, std::string("bar"), 0)));
	inst->params.push_back(new Identifier(Token(Token::ID, std::string("baz"), 0)));

	tu->stmts.push_back(inst);

	const std::string actual = printer.toString(tu);
	ASSERT_EQ("(TransUnit (FuncCallExpr (Identifier \"foo\") (Identifier \"bar\") (Identifier \"baz\")))", actual);

}

}
