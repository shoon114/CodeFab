#ifdef _DEBUG
#include "gmock/gmock.h"
#include "Tokenizer.h"
#include <iostream>
#include <sstream>

using namespace testing;

namespace {
	Tokenizer MakeTokenizerWithInput(const std::string& input) {
		std::istringstream fakeInput(input);
		std::streambuf* originalCinBuffer = std::cin.rdbuf(fakeInput.rdbuf());

		Tokenizer tokenizer;
		tokenizer.GetCodeFromUser();

		std::cin.rdbuf(originalCinBuffer);
		return tokenizer;
	}
}


TEST(TokenizerTest, SplitIntoWords_AdditionAndMultiplication) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 1 + 2 * 3;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "1", "+", "2", "*", "3", ";"));
}

TEST(TokenizerTest, SplitIntoWords_ParenthesizedExpression) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print (1 + 2) * 3;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "(", "1", "+", "2", ")", "*", "3", ";"));
}

TEST(TokenizerTest, SplitIntoWords_ChainedSubtraction) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 10 - 4 - 3;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "10", "-", "4", "-", "3", ";"));
}

TEST(TokenizerTest, SplitIntoWords_ChainedDivision) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 8 / 2 / 2;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "8", "/", "2", "/", "2", ";"));
}

TEST(TokenizerTest, SplitIntoWords_UnaryMinus) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print -3 + 2;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "-", "3", "+", "2", ";"));
}


TEST(TokenizerTest, SplitIntoWords_LessThan) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 1 < 2;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "1", "<", "2", ";"));
}

TEST(TokenizerTest, SplitIntoWords_GreaterThan) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 3 > 5;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "3", ">", "5", ";"));
}


TEST(TokenizerTest, SplitIntoWords_StringConcatenation) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print \"Hello, \" + \"CodeFab!\";\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "\"Hello, \"", "+", "\"CodeFab!\"", ";"));
}


TEST(TokenizerTest, SplitIntoWords_IntegerLiteral) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 5;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "5", ";"));
}

TEST(TokenizerTest, SplitIntoWords_DecimalLiteralWithTrailingZero) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 5.0;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "5.0", ";"));
}

TEST(TokenizerTest, SplitIntoWords_DecimalLiteral) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 3.14;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "3.14", ";"));
}


TEST(TokenizerTest, SplitIntoWords_TrueLiteral) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print true;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "true", ";"));
}

TEST(TokenizerTest, SplitIntoWords_FalseLiteral) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print false;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "false", ";"));
}


TEST(TokenizerTest, SplitIntoWords_VariableDeclarationAndUse) {
	Tokenizer tokenizer = MakeTokenizerWithInput(
		"var a = 10;\n"
		"var b = 20;\n"
		"print a + b;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre(
		"var", "a", "=", "10", ";",
		"var", "b", "=", "20", ";",
		"print", "a", "+", "b", ";"));
}


TEST(TokenizerTest, SplitIntoWords_Reassignment) {
	Tokenizer tokenizer = MakeTokenizerWithInput(
		"a = a + 5;\n"
		"print a;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre(
		"a", "=", "a", "+", "5", ";",
		"print", "a", ";"));
}


TEST(TokenizerTest, SplitIntoWords_BlockScopeShadowing) {
	Tokenizer tokenizer = MakeTokenizerWithInput(
		"var x = \"global\";\n"
		"{\n"
		"  var x = \"inner\";\n"
		"  print x;\n"
		"}\n"
		"print x;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre(
		"var", "x", "=", "\"global\"", ";",
		"{",
		"var", "x", "=", "\"inner\"", ";",
		"print", "x", ";",
		"}",
		"print", "x", ";"));
}


TEST(TokenizerTest, SplitIntoWords_InnerBlockModifiesOuterVariable) {
	Tokenizer tokenizer = MakeTokenizerWithInput(
		"var count = 0;\n"
		"{\n"
		"  count = count + 1;\n"
		"}\n"
		"print count;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre(
		"var", "count", "=", "0", ";",
		"{",
		"count", "=", "count", "+", "1", ";",
		"}",
		"print", "count", ";"));
}


TEST(TokenizerTest, SplitIntoWords_NestedScopes) {
	Tokenizer tokenizer = MakeTokenizerWithInput(
		"var outer = \"A\";\n"
		"{\n"
		"  var inner = \"B\";\n"
		"  {\n"
		"    print outer + inner;\n"
		"  }\n"
		"}\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre(
		"var", "outer", "=", "\"A\"", ";",
		"{",
		"var", "inner", "=", "\"B\"", ";",
		"{",
		"print", "outer", "+", "inner", ";",
		"}",
		"}"));
}

TEST(TokenizerTest, SplitIntoWords_IfWithoutElse) {
	Tokenizer tokenizer = MakeTokenizerWithInput(
		"if (true) print \"bbq\";\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre(
		"if", "(", "true", ")", "print", "\"bbq\"", ";"));
}

TEST(TokenizerTest, SplitIntoWords_IfElse) {
	Tokenizer tokenizer = MakeTokenizerWithInput(
		"if (false) print \"no\"; else print \"kfc\";\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre(
		"if", "(", "false", ")", "print", "\"no\"", ";",
		"else", "print", "\"kfc\"", ";"));
}


TEST(TokenizerTest, SplitIntoWords_NestedIfElseBindsToNearestIf) {
	Tokenizer tokenizer = MakeTokenizerWithInput(
		"if (true)\n"
		"{\n"
		"  if (false) print \"kfc\";\n"
		"  else print \"bbq\";\n"
		"}\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre(
		"if", "(", "true", ")",
		"{",
		"if", "(", "false", ")", "print", "\"kfc\"", ";",
		"else", "print", "\"bbq\"", ";",
		"}"));
}


TEST(TokenizerTest, SplitIntoWords_ForLoop) {
	Tokenizer tokenizer = MakeTokenizerWithInput(
		"for (var j = 0; j < 3; j = j + 1) { print j; }\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre(
		"for", "(", "var", "j", "=", "0", ";",
		"j", "<", "3", ";",
		"j", "=", "j", "+", "1", ")",
		"{", "print", "j", ";", "}"));
}
#endif
