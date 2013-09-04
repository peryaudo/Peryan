#include "gtest/gtest.h"

#include "../../src/WarningPrinter.h"
#include "../../src/Options.h"
#include "../../src/Lexer.h"
#include "../../src/Parser.h"
#include "../../src/StringSourceReader.h"

namespace {

class SemanticsTest : public ::testing::Test {
protected:
	Peryan::Options opt;
	Peryan::StringSourceReader ssr;
	Peryan::WarningPrinter wp;
	Peryan::Lexer lexer;
	Peryan::Parser parser;

public:
	SemanticsTest() : ssr("main.pr"), wp(), lexer(ssr, opt, wp), parser(lexer, opt, wp) {}

	void parse() {
		try {
			parser.parse();
		} catch (Peryan::ParserError pe) {
			std::cout<<pe.toString(lexer)<<std::endl;
			throw pe;
		} catch (Peryan::SemanticsError se) {
			std::cout<<se.toString(lexer)<<std::endl;
			throw se;
		}
	}
};

TEST_F(SemanticsTest, VariableDefinition1) {
	const std::string source =
		"var foo :: Int = 1\n"
		"var bar :: Int = 2\n"
		"var baz :: Int = 3\n";

	ssr.setString("main.pr", source);

	ASSERT_NO_THROW(parser.parse());
}

TEST_F(SemanticsTest, VariableDefinition2) {
	const std::string source =
		"var foo :: Int = 1\n"
		"var bar :: Int = 2\n"
		"var foo :: Int = 3\n";

	ssr.setString("main.pr", source);

	ASSERT_THROW(parser.parse(), Peryan::SemanticsError);
}

TEST_F(SemanticsTest, FunctionDefinition1) {
	const std::string source =
		"var foo :: Int = 1\n"
		"func bar() :: Int { \n"
		"\tvar foo :: Int = 2\n"
		"\t{\n"
		"\t\tvar foo :: Int = 2\n"
		"\t}\n"
		"\treturn foo\n"
		"}\n";

	ssr.setString("main.pr", source);

	ASSERT_NO_THROW(parser.parse());
}

TEST_F(SemanticsTest, Scope) {
	const std::string source =
		"var foo :: Int = 1\n"
		"{\n"
		"\tvar bar :: Int = foo\n"
		"\tvar foo :: Int = 2\n"
		"}\n";

	ssr.setString("main.pr", source);

	ASSERT_NO_THROW(parser.parse());
}

TEST_F(SemanticsTest, UsingVariable1) {
	const std::string source =
		"var foo :: Int = 114, bar :: Int = 514, baz :: Int = 810\n"
		"foo = bar * baz\n";

	ssr.setString("main.pr", source);

	ASSERT_NO_THROW(parse());

}

TEST_F(SemanticsTest, UsingVariable2) {
	const std::string source =
		"var foo :: Int\n"
		"foo = bar\n"
		"var bar :: Int = 114514\n";

	ssr.setString("main.pr", source);

	ASSERT_THROW(parser.parse(), Peryan::SemanticsError);
}

TEST_F(SemanticsTest, TypeCheck1) {
	const std::string source =
		"var str :: String = \"foobar\"\n"
		"var bool :: Bool = true\n"
		"var integer :: Int\n"
		"integer = str * bool\n";

	ssr.setString("main.pr", source);

	ASSERT_THROW(parser.parse(), Peryan::SemanticsError);
}

TEST_F(SemanticsTest, FunctionDefinition2) {
	const std::string source =
		"func testfunc(foo :: Int, bar :: Int) :: Int {\n"
		"\treturn foo + bar\n"
		"}\n";

	ssr.setString("main.pr", source);

	ASSERT_NO_THROW(parser.parse());
}

TEST_F(SemanticsTest, DisallowedIdentifier) {
	const std::string source = "var cnt :: Int = 1\n";

	ssr.setString("main.pr", source);

	ASSERT_THROW(parser.parse(), Peryan::SemanticsError);
}

TEST_F(SemanticsTest, StringConcatenation) {
	const std::string source =
		"extern mes :: String -> Void\n"
		"mes \"Hello, \" + \"World!\"\n";

	ssr.setString("main.pr", source);

	ASSERT_NO_THROW(parser.parse());
}

TEST_F(SemanticsTest, SimpleTypeInference) {
	const std::string source =
		"var foobar = 114\n";

	ssr.setString("main.pr", source);

	ASSERT_NO_THROW(parse());
}

TEST_F(SemanticsTest, DoNotInstantiateNamespace) {
	const std::string source =
		"extern mes :: String -> Void\n"
		"namespace Hoge {\n"
			"\tfunc hige() :: Void {\n"
				"\t\tmes \"foo\"\n"
			"\t}\n"
		"}\n"
		"var foo :: Hoge\n";

	ssr.setString("main.pr", source);

	ASSERT_THROW(parse(), Peryan::SemanticsError);
}

TEST_F(SemanticsTest, AdvReturnStmt) {
	const std::string source =
		"extern funfun :: Int->Int\n"
		"func foo(x::Int) :: Int {\n"
		"\treturn funfun(x)\n"
		"}\n";

	ssr.setString("main.pr", source);

	ASSERT_NO_THROW(parse());
}

TEST_F(SemanticsTest, FunctionTypeInference) {
	const std::string source =
		"func hoge(x) {\n"
		"\treturn 123\n"
		"}\n"
		"var hige = hoge(\"foo\")\n";

	ssr.setString("main.pr", source);

	ASSERT_NO_THROW(parse());
}

};

