#include "gtest/gtest.h"

#include "../../src/WarningPrinter.h"
#include "../../src/Options.h"
#include "../../src/Lexer.h"
#include "../../src/Parser.h"
#include "../../src/StringSourceReader.h"
#include "../../src/AST.h"
#include "../../src/ASTPrinter.h"

namespace {

class ParserTest : public ::testing::Test {
protected:
	Peryan::Options opt;
	Peryan::StringSourceReader ssr;
	Peryan::WarningPrinter wp;
	Peryan::Lexer lexer;
	Peryan::Parser parser;

public:
	ParserTest() : ssr("main.pr"), wp(), lexer(ssr, opt, wp), parser(lexer, opt, wp) {
	}

	std::string parseAndPrint(std::string str) {
		ssr.setString("main.pr", str);

		using namespace Peryan;

		GlobalScope *gs = parser.getSymbolTable().getGlobalScope();
		Type *Int_ = static_cast<BuiltInTypeSymbol *>(gs->resolve("Int"));
		Type *String_ = static_cast<BuiltInTypeSymbol *>(gs->resolve("String"));
		Type *Void_ = static_cast<BuiltInTypeSymbol *>(gs->resolve("Void"));
		Type *Bool_ = static_cast<BuiltInTypeSymbol *>(gs->resolve("Bool"));

		FuncSymbol *fsMes = new FuncSymbol("mes", gs, 0);
		fsMes->setType(new FuncType(String_, Void_));

		FuncSymbol *fsPos = new FuncSymbol("pos", gs, 0);
		fsPos->setType(new FuncType(Int_, new FuncType(Int_, Void_)));

		FuncSymbol *fsAwait = new FuncSymbol("await", gs, 0);
		fsAwait->setType(new FuncType(Int_, Void_));

		FuncSymbol *fsWait = new FuncSymbol("wait", gs, 0);
		fsWait->setType(new FuncType(Int_, Void_));

		gs->define(fsMes);
		gs->define(fsPos);
		gs->define(fsAwait);
		gs->define(fsWait);

		gs->define(new VarSymbol("a", Int_, 0));
		gs->define(new VarSymbol("b", Int_, 0));
		gs->define(new VarSymbol("c", Int_, 0));
		gs->define(new VarSymbol("d", Bool_, 0));

		gs->define(new VarSymbol("foo", Int_, 0));
		gs->define(new VarSymbol("bar", Int_, 0));
		gs->define(new VarSymbol("baz", Int_, 0));

		try {
			parser.parse();

			Peryan::ASTPrinter printer;

			return printer.toString(parser.getTransUnit());
		} catch (Peryan::ParserError pe) {
			std::cout<<pe.toString(lexer)<<std::endl;
			throw pe;
		} catch (Peryan::SemanticsError se) {
			std::cout<<se.toString(lexer)<<std::endl;
			throw se;
		}

	}
};

TEST_F(ParserTest, VariableDeclaration) {
	const std::string source = "var thisIsVar :: Int = 5\n";
	const std::string expected = "(TransUnit (VarDefStmt (Identifier \"thisIsVar\") (IntLiteralExpr 5)))";
	ASSERT_EQ(expected, parseAndPrint(source));
}

TEST_F(ParserTest, BasicInstructions) {
	const std::string source =
		"mes \"Hello, World.\"\n"
		"await 0\n"
		"pos 20, 25\n";

	const std::string expected =
		"(TransUnit"
			" (InstStmt (Identifier \"mes\") (StrLiteralExpr \"Hello, World.\"))"
			" (InstStmt (Identifier \"await\") (IntLiteralExpr 0))"
			" (InstStmt (Identifier \"pos\") (IntLiteralExpr 20) (IntLiteralExpr 25)))";

	ASSERT_EQ(expected, parseAndPrint(source));
}

TEST_F(ParserTest, Expression) {
	const std::string source = "mes String(114 - 5 - 1 * 4)\n";

	const std::string expected =
		"(TransUnit (InstStmt (Identifier \"mes\") (ConstructorExpr (TypeSpec \"String\")"
			" (BinaryExpr <MINUS>"
				" (BinaryExpr <MINUS> (IntLiteralExpr 114) (IntLiteralExpr 5))"
					" (BinaryExpr <STAR> (IntLiteralExpr 1) (IntLiteralExpr 4))))))";

	ASSERT_EQ(expected, parseAndPrint(source));
}

TEST_F(ParserTest, ParsingEquals1) {
	const std::string source = "(a = b) c";
	/*const std::string expected =
		"(TransUnit (InstStmt"
			" (BinaryExpr <EQL> (Identifier \"a\") (Identifier \"b\")) (Identifier \"c\")))";*/

	ASSERT_THROW(parseAndPrint(source), Peryan::SemanticsError);
}

TEST_F(ParserTest, ParsingEquals2) {
	const std::string source = "d = b = c";
	const std::string expected =
		"(TransUnit (AssignStmt <EQL> (Identifier \"d\")"
			" (BinaryExpr <EQL> (DerefExpr (Identifier \"b\")) (DerefExpr (Identifier \"c\")))))";

	ASSERT_EQ(expected, parseAndPrint(source));
}

TEST_F(ParserTest, UnaryExpression) {
	const std::string source = "a = -+-b";
	const std::string expected =
		"(TransUnit (AssignStmt <EQL> (Identifier \"a\")"
			" (UnaryExpr <MINUS> (UnaryExpr <PLUS> (UnaryExpr <MINUS> (DerefExpr (Identifier \"b\")))))))";

	ASSERT_EQ(expected, parseAndPrint(source));
}

TEST_F(ParserTest, IfStatement) {
	const std::string source =
		"if foo = bar {\n"
		"\tmes \"foo is bar.\"\n"
		"} else if foo = baz {\n"
		"\tmes \"foo is baz.\"\n"
		"} else {\n"
		"\tmes \"neither.\"\n"
		"}\n";

	const std::string expected =
		"(TransUnit (IfStmt"
			" (BinaryExpr <EQL> (DerefExpr (Identifier \"foo\")) (DerefExpr (Identifier \"bar\")))"
			" (CompStmt (InstStmt (Identifier \"mes\") (StrLiteralExpr \"foo is bar.\")))"
			" (BinaryExpr <EQL> (DerefExpr (Identifier \"foo\")) (DerefExpr (Identifier \"baz\")))"
			" (CompStmt (InstStmt (Identifier \"mes\") (StrLiteralExpr \"foo is baz.\")))"
			" (CompStmt (InstStmt (Identifier \"mes\") (StrLiteralExpr \"neither.\")))))";

	ASSERT_EQ(expected, parseAndPrint(source));
}

TEST_F(ParserTest, Labels) {
	const std::string source =
		"*fooL\n"
		"goto *fooL\n"
		"wait 2*bar\n";

	const std::string expected =
		"(TransUnit"
			" (LabelStmt (Label \"fooL\"))"
			" (GotoStmt (Label \"fooL\"))"
			" (InstStmt (Identifier \"wait\")"
				" (BinaryExpr <STAR> (IntLiteralExpr 2) (DerefExpr (Identifier \"bar\")))))";

	opt.hspCompat = true;

	ASSERT_EQ(expected, parseAndPrint(source));
}

TEST_F(ParserTest, Externs) {
	const std::string source =
		"extern hogefunc :: Int -> Double -> String\n"
		"extern hagefunc :: Void -> Void\n"
		"hogefunc 36, 114.514\n";

	const std::string expected =
		"(TransUnit"
			" (ExternStmt (Identifier \"hogefunc\"))"
			" (ExternStmt (Identifier \"hagefunc\"))"
			" (InstStmt (Identifier \"hogefunc\")"
				" (IntLiteralExpr 36)"
				" (FloatLiteralExpr 114.514)))";

	ASSERT_EQ(expected, parseAndPrint(source));
}

TEST_F(ParserTest, Return) {
	const std::string source =
		"func hoge(x :: Int) :: Void {\n"
		"\treturn\n"
		"}\n";

	ASSERT_NO_THROW(parseAndPrint(source));

}

TEST_F(ParserTest, ArrayLiteral) {
	const std::string source =
		"extern tookArray :: [Int] -> Void\n"
		"tookArray [1, 2, 3, 4]\n";

	const std::string expected =
		"(TransUnit"
			" (ExternStmt (Identifier \"tookArray\"))"
			" (InstStmt (Identifier \"tookArray\")"
				" (ArrayLiteralExpr"
					" (IntLiteralExpr 1)"
					" (IntLiteralExpr 2)"
					" (IntLiteralExpr 3)"
					" (IntLiteralExpr 4))))";

	ASSERT_EQ(expected, parseAndPrint(source));
}

TEST_F(ParserTest, ArraySubscript) {
	const std::string source =
		"var ary :: [Int] = [3, 1, 4, 1, 5]\n"
		"var refd :: Int = ary[2]\n";

	const std::string expected =
		"(TransUnit"
			" (VarDefStmt (Identifier \"ary\")"
				" (ArrayLiteralExpr"
					" (IntLiteralExpr 3)"
					" (IntLiteralExpr 1)"
					" (IntLiteralExpr 4)"
					" (IntLiteralExpr 1)"
					" (IntLiteralExpr 5)))"
			" (VarDefStmt (Identifier \"refd\") (DerefExpr (SubscrExpr (Identifier \"ary\") (IntLiteralExpr 2)))))";

	ASSERT_EQ(expected, parseAndPrint(source));
}

TEST_F(ParserTest, InfiniteLoop) { // Not Apple's street name in California!
	const std::string source =
		"repeat\n"
			"\tmes \"hi\"\n"
		"loop\n";

	const std::string expected =
		"(TransUnit"
			" (RepeatStmt () (InstStmt (Identifier \"mes\") (StrLiteralExpr \"hi\"))))";

	ASSERT_EQ(expected, parseAndPrint(source));
}

TEST_F(ParserTest, ArrayConstructor) {
	const std::string source = "var ary :: [Int] = [Int](5)\n";

	const std::string expected = "(TransUnit"
		" (VarDefStmt (Identifier \"ary\")"
			" (ConstructorExpr (ArrayTypeSpec (TypeSpec \"Int\")) (IntLiteralExpr 5))))";

	ASSERT_EQ(expected, parseAndPrint(source));
}

TEST_F(ParserTest, ShorterIf) {
	const std::string source =
		"if a = b : mes \"a = b\" : mes \"yes\" : else : mes \"a != b\"\n"
		"mes \"finished\"\n";

	const std::string expected =
		"(TransUnit"
			" (IfStmt (BinaryExpr <EQL> (DerefExpr (Identifier \"a\")) (DerefExpr (Identifier \"b\")))"
				" (CompStmt"
					" (InstStmt (Identifier \"mes\") (StrLiteralExpr \"a = b\"))"
					" (InstStmt (Identifier \"mes\") (StrLiteralExpr \"yes\")))"
				" (CompStmt"
					" (InstStmt (Identifier \"mes\") (StrLiteralExpr \"a != b\"))))"
			" (InstStmt (Identifier \"mes\") (StrLiteralExpr \"finished\")))";

	ASSERT_EQ(expected, parseAndPrint(source));
}

/*TEST_F(ParserTest, PartialCall) {
	const std::string source = "(partial pos(1)) 1\n";

	const std::string expected =
		"(TransUnit"
			" (InstStmt (FuncCallExpr (Identifier \"pos\") (IntLiteralExpr 1)) (IntLiteralExpr 1)))";

	opt.verbose = true;
	ASSERT_EQ(expected, parseAndPrint(source));
}*/

/*TEST_F(ParserTest, FuncExpr) {
	const std::string source = "func(x::Int)::Void { wait x : } 5\n";
	const std::string expected =
		"(TransUnit"
			" (InstStmt"
				" (FuncExpr (Identifier \"x\")"
					" (CompStmt (InstStmt (DerefExpr (Identifier \"x\")))))"
				" (IntLiteralExpr 5)))";

	opt.verbose = true;
	ASSERT_EQ(expected, parseAndPrint(source));
}*/

}
