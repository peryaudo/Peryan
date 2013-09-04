#include "gtest/gtest.h"

#include <sstream>
#include <map>

#include "../../src/WarningPrinter.h"
#include "../../src/Options.h"
#include "../../src/Lexer.h"
#include "../../src/StringSourceReader.h"


namespace {

class LexerTest : public ::testing::Test {
protected:
	Peryan::StringSourceReader ssr;
	Peryan::Lexer lexer;
	Peryan::Options opt;
	Peryan::WarningPrinter wp;

public:
	LexerTest() : ssr("main.pr"), lexer(ssr, opt, wp) {
	}

	void lexAndCompare(const char *expected[], int len) {
		std::vector<Peryan::Token> tokens;

		while (true) {
			Peryan::Token cur = lexer.getNextToken();
			tokens.push_back(cur);
			if (cur.getType() == Peryan::Token::END)
				break;
			//std::cout<<cur.toString()<<std::endl;
		}

		ASSERT_TRUE(len == tokens.size());

		for (int i = 0; i < len; ++i) {
			std::stringstream ss;
			ss<<i<<": ";
			ASSERT_EQ(ss.str() + std::string(expected[i]), ss.str() + tokens[i].toString());

			//std::cout<<lexer.prettilyPrintLine(tokens[i].getPosition(), "tokenized from here");
		}

		return;
	}
};

TEST_F(LexerTest, VariableDeclaration) {
	ssr.setString("main.pr", "var thisIsVar :: Int = 5\n");

	const char *expected[] = {
		"<KW_VAR>",
		"<ID, thisIsVar>",
		"<CLNCLN>",
		"<TYPE_ID, Int>",
		"<EQL>",
		"<INTEGER, 5>",
		"<TERM>",
		"<END>"
	};

	lexAndCompare(expected, sizeof(expected) / sizeof(expected[0]));
}

TEST_F(LexerTest, ImportDirective) {
	ssr.setString("main.pr",
		"foo\n"
		"#import \"second.pr\"\n"
		"bar\n");

	ssr.setString("second.pr",
		"baz\n"
		"#import \"third.pr\"\n"
		"#import \"third.pr\"\n"
		"foobar\n");

	ssr.setString("third.pr",
		"piyo");

	const char *expected[] = {
		"<ID, foo>",	"<TERM>",
		"<ID, baz>",	"<TERM>",
		"<ID, piyo>",	"<TERM>",
		"<ID, foobar>",	"<TERM>",
		"<ID, bar>",	"<TERM>",
		"<END>"
	};

	lexAndCompare(expected, sizeof(expected) / sizeof(expected[0]));
}

TEST_F(LexerTest, VariousTypesOfTokens) {
	ssr.setString("main.pr",
		"\tvar /* hoge */ string :: String = String(\"this is string\\t\")\n"
		"\t\t12 3 \r\n"
		"123.123\n"
		"//piyo\n"
		"; guwa\n"
		"0xABC\n"
		"0b1011\n"
		"'A'\n"
	);
	const char *expected[] = {
		"<KW_VAR>", "<ID, string>", "<CLNCLN>", "<TYPE_ID, String>",
		"<EQL>", "<TYPE_ID, String>", "<LPAREN>", "<STRING, this is string\t>", "<RPAREN>", "<TERM>",
		"<INTEGER, 12>", "<INTEGER, 3>", "<TERM>",
		"<FLOAT>", "<TERM>",
		"<TERM>",
		"<TERM>",
		"<INTEGER, 2748>", "<TERM>",
		"<INTEGER, 11>", "<TERM>",
		"<CHAR, A>", "<TERM>",
		"<END>"
	};

	lexAndCompare(expected, sizeof(expected) / sizeof(expected[0]));
}

TEST_F(LexerTest, PrettyPrint) {
	ssr.setString("main.pr",
		"foo\n"
		"bar\n"
		"#import \"sub.pr\"\n"
		"foobar\n"
		"114514\n");

	ssr.setString("sub.pr",
		"alpha\n"
		"beta theta\n"); // print tokens[7] (theta) in this line

	std::vector<Peryan::Token> tokens;

	while (true) {
		Peryan::Token cur = lexer.getNextToken();
		tokens.push_back(cur);
		if (cur.getType() == Peryan::Token::END)
			break;
	}

	ASSERT_EQ("<ID, theta>", tokens[7].toString());

	std::string actual = lexer.getPrettyPrint(tokens[7].getPosition(), "error: test");
	std::string expected(
		"sub.pr:2:6: error: test\n"
		"\tbeta theta\n"
		"\t     ^\n");

	ASSERT_EQ(expected, actual);

}

};
