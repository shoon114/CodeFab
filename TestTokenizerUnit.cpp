#ifdef _DEBUG
#include "gmock/gmock.h"
#include "Tokenizer.h"
#include <iostream>
#include <sstream>

using namespace testing;

class TokenizerTest : public Test {
protected:
	std::vector<std::string> SplitWords(const std::string& input) {
		std::istringstream fakeInput(input);
		std::streambuf* originalCinBuffer = std::cin.rdbuf(fakeInput.rdbuf());

		Tokenizer tokenizer;

		try {
			tokenizer.GetCodeFromUser();
		}
		catch (const std::exception& e) {
			std::cin.rdbuf(originalCinBuffer);
			std::cerr << "Tokenizer 실행 중 에러 발생: " << e.what() << std::endl;
			throw;
		}
		catch (...) {
			std::cin.rdbuf(originalCinBuffer);
			std::cerr << "Tokenizer 실행 중 알 수 없는 치명적인 에러 발생!" << std::endl;
			throw;
		}

		std::cin.rdbuf(originalCinBuffer);
		return tokenizer.SplitIntoWords();
	}
};

TEST_F(TokenizerTest, SplitIntoWords_AdditionAndMultiplication) {
	EXPECT_THAT(SplitWords("print 1 + 2 * 3;\n"),
		ElementsAre("print", "1", "+", "2", "*", "3", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_ParenthesizedExpression) {
	EXPECT_THAT(SplitWords("print (1 + 2) * 3;\n"),
		ElementsAre("print", "(", "1", "+", "2", ")", "*", "3", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_ChainedSubtraction) {
	EXPECT_THAT(SplitWords("print 10 - 4 - 3;\n"),
		ElementsAre("print", "10", "-", "4", "-", "3", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_ChainedDivision) {
	EXPECT_THAT(SplitWords("print 8 / 2 / 2;\n"),
		ElementsAre("print", "8", "/", "2", "/", "2", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_UnaryMinus) {
	EXPECT_THAT(SplitWords("print -3 + 2;\n"),
		ElementsAre("print", "-", "3", "+", "2", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_LessThan) {
	EXPECT_THAT(SplitWords("print 1 < 2;\n"),
		ElementsAre("print", "1", "<", "2", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_GreaterThan) {
	EXPECT_THAT(SplitWords("print 3 > 5;\n"),
		ElementsAre("print", "3", ">", "5", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_StringConcatenation) {
	EXPECT_THAT(SplitWords("print \"Hello, \" + \"CodeFab!\";\n"),
		ElementsAre("print", "\"Hello, \"", "+", "\"CodeFab!\"", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_IntegerLiteral) {
	EXPECT_THAT(SplitWords("print 5;\n"),
		ElementsAre("print", "5", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_DecimalLiteralWithTrailingZero) {
	EXPECT_THAT(SplitWords("print 5.0;\n"),
		ElementsAre("print", "5.0", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_DecimalLiteral) {
	EXPECT_THAT(SplitWords("print 3.14;\n"),
		ElementsAre("print", "3.14", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_TrueLiteral) {
	EXPECT_THAT(SplitWords("print true;\n"),
		ElementsAre("print", "true", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_FalseLiteral) {
	EXPECT_THAT(SplitWords("print false;\n"),
		ElementsAre("print", "false", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_VariableDeclarationAndUse) {
	EXPECT_THAT(SplitWords(
		"var a = 10;\n"
		"var b = 20;\n"
		"print a + b;\n"),
		ElementsAre(
			"var", "a", "=", "10", ";",
			"var", "b", "=", "20", ";",
			"print", "a", "+", "b", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_Reassignment) {
	EXPECT_THAT(SplitWords(
		"a = a + 5;\n"
		"print a;\n"),
		ElementsAre(
			"a", "=", "a", "+", "5", ";",
			"print", "a", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_BlockScopeShadowing) {
	EXPECT_THAT(SplitWords(
		"var x = \"global\";\n"
		"{\n"
		"  var x = \"inner\";\n"
		"  print x;\n"
		"}\n"
		"print x;\n"),
		ElementsAre(
			"var", "x", "=", "\"global\"", ";",
			"{",
			"var", "x", "=", "\"inner\"", ";",
			"print", "x", ";",
			"}",
			"print", "x", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_InnerBlockModifiesOuterVariable) {
	EXPECT_THAT(SplitWords(
		"var count = 0;\n"
		"{\n"
		"  count = count + 1;\n"
		"}\n"
		"print count;\n"),
		ElementsAre(
			"var", "count", "=", "0", ";",
			"{",
			"count", "=", "count", "+", "1", ";",
			"}",
			"print", "count", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_NestedScopes) {
	EXPECT_THAT(SplitWords(
		"var outer = \"A\";\n"
		"{\n"
		"  var inner = \"B\";\n"
		"  {\n"
		"    print outer + inner;\n"
		"  }\n"
		"}\n"),
		ElementsAre(
			"var", "outer", "=", "\"A\"", ";",
			"{",
			"var", "inner", "=", "\"B\"", ";",
			"{",
			"print", "outer", "+", "inner", ";",
			"}",
			"}"));
}

TEST_F(TokenizerTest, SplitIntoWords_IfWithoutElse) {
	EXPECT_THAT(SplitWords(
		"if (true) print \"bbq\";\n"),
		ElementsAre(
			"if", "(", "true", ")", "print", "\"bbq\"", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_IfElse) {
	EXPECT_THAT(SplitWords(
		"if (false) print \"no\"; else print \"kfc\";\n"),
		ElementsAre(
			"if", "(", "false", ")", "print", "\"no\"", ";",
			"else", "print", "\"kfc\"", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_NestedIfElseBindsToNearestIf) {
	EXPECT_THAT(SplitWords(
		"if (true)\n"
		"{\n"
		"  if (false) print \"kfc\";\n"
		"  else print \"bbq\";\n"
		"}\n"),
		ElementsAre(
			"if", "(", "true", ")",
			"{",
			"if", "(", "false", ")", "print", "\"kfc\"", ";",
			"else", "print", "\"bbq\"", ";",
			"}"));
}

TEST_F(TokenizerTest, SplitIntoWords_ForLoop) {
	EXPECT_THAT(SplitWords(
		"for (var j = 0; j < 3; j = j + 1) { print j; }\n"),
		ElementsAre(
			"for", "(", "var", "j", "=", "0", ";",
			"j", "<", "3", ";",
			"j", "=", "j", "+", "1", ")",
			"{", "print", "j", ";", "}"));
}
#endif
