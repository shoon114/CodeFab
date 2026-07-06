#ifdef _DEBUG
#include "gmock/gmock.h"
#include "Tokenizer.h"
#include <iostream>
#include <sstream>
#include <algorithm>

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

// GetCodeFromUser
TEST(TokenizerTest, GetCodeFromUser_StoresInputForLaterUse) {
	Tokenizer tokenizer = MakeTokenizerWithInput("int a = 10;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("int", "a", "=", "10", ";"));
}

// SplitIntoWords
TEST(TokenizerTest, SplitIntoWords_SplitsOnWhitespace) {
	Tokenizer tokenizer = MakeTokenizerWithInput("int a = 10\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("int", "a", "=", "10"));
}

TEST(TokenizerTest, SplitIntoWords_SplitsOperatorsAndParenthesesWithoutSpaces) {
	Tokenizer tokenizer = MakeTokenizerWithInput("if(a>10)\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("if", "(", "a", ">", "10", ")"));
}

TEST(TokenizerTest, SplitIntoWords_RecognizesTwoCharacterOperators) {
	Tokenizer tokenizer = MakeTokenizerWithInput("a >= 10 == b != c <= d\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("a", ">=", "10", "==", "b", "!=", "c", "<=", "d"));
}

TEST(TokenizerTest, SplitIntoWords_KeepsMultipleStatementsInOneList) {
	Tokenizer tokenizer = MakeTokenizerWithInput("int a =10; int b =20;\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre(
		"int", "a", "=", "10", ";",
		"int", "b", "=", "20", ";"));
}

TEST(TokenizerTest, SplitIntoWords_HandlesEmptyInput) {
	Tokenizer tokenizer = MakeTokenizerWithInput("\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_TRUE(words.empty());
}

// CreateTokenForCode
TEST(TokenizerTest, CreateTokenForCode_ClassifiesNumber) {
	Tokenizer tokenizer = MakeTokenizerWithInput("10\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_FALSE(tokens.empty());
	EXPECT_EQ(tokens[0].type, TokenType::Number);
	EXPECT_EQ(tokens[0].lexeme, "10");
	EXPECT_DOUBLE_EQ(tokens[0].realValue, 10.0);
}

TEST(TokenizerTest, CreateTokenForCode_ClassifiesIdentifier) {
	Tokenizer tokenizer = MakeTokenizerWithInput("a\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_FALSE(tokens.empty());
	EXPECT_EQ(tokens[0].type, TokenType::Identifier);
	EXPECT_EQ(tokens[0].lexeme, "a");
}

TEST(TokenizerTest, CreateTokenForCode_ClassifiesKeyword) {
	Tokenizer tokenizer = MakeTokenizerWithInput("if\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_FALSE(tokens.empty());
	EXPECT_EQ(tokens[0].type, TokenType::KwIf);
}

TEST(TokenizerTest, CreateTokenForCode_ClassifiesOperatorsAndPunctuation) {
	Tokenizer tokenizer = MakeTokenizerWithInput("if(a>10)\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_EQ(tokens.size(), 6u);
	EXPECT_EQ(tokens[0].type, TokenType::KwIf);
	EXPECT_EQ(tokens[1].type, TokenType::LParen);
	EXPECT_EQ(tokens[2].type, TokenType::Identifier);
	EXPECT_EQ(tokens[3].type, TokenType::Gt);
	EXPECT_EQ(tokens[4].type, TokenType::Number);
	EXPECT_EQ(tokens[5].type, TokenType::RParen);
}

TEST(TokenizerTest, CreateTokenForCode_HandlesMultipleStatementsInOneLine) {
	Tokenizer tokenizer = MakeTokenizerWithInput("int a =10; int b =20;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	int semicolonCount = static_cast<int>(std::count_if(
		tokens.begin(), tokens.end(),
		[](const Token& token) { return token.type == TokenType::Semicolon; }));

	EXPECT_EQ(semicolonCount, 2);
}

TEST(TokenizerTest, CreateTokenForCode_TracksLineAndColumn) {
	Tokenizer tokenizer = MakeTokenizerWithInput("int a = 10;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_FALSE(tokens.empty());
	EXPECT_EQ(tokens[0].line, 1);
	EXPECT_EQ(tokens[0].column, 1);
}

TEST(TokenizerTest, CreateTokenForCode_AppendsEndOfFileToken) {
	Tokenizer tokenizer = MakeTokenizerWithInput("a\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_FALSE(tokens.empty());
	EXPECT_EQ(tokens.back().type, TokenType::EndOfFile);
}

TEST(TokenizerTest, CreateTokenForCode_HandlesEmptyInputWithoutCrashing) {
	Tokenizer tokenizer = MakeTokenizerWithInput("\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_FALSE(tokens.empty());
	EXPECT_EQ(tokens[0].type, TokenType::EndOfFile);
}

// print keyword
TEST(TokenizerTest, CreateTokenForCode_ClassifiesPrintKeyword) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 5;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_GE(tokens.size(), 3u);
	EXPECT_EQ(tokens[0].type, TokenType::Print);
}

// 산술 연산자 / 우선순위 - 파서가 아니라 토크나이저가 올바른 토큰 시퀀스를 만드는지만 확인
TEST(TokenizerTest, CreateTokenForCode_MultiplicationAndAddition) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 1 + 2 * 3;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_EQ(tokens.size(), 8u);
	EXPECT_EQ(tokens[0].type, TokenType::Print);
	EXPECT_EQ(tokens[1].type, TokenType::Number);
	EXPECT_EQ(tokens[2].type, TokenType::Plus);
	EXPECT_EQ(tokens[3].type, TokenType::Number);
	EXPECT_EQ(tokens[4].type, TokenType::Star);
	EXPECT_EQ(tokens[5].type, TokenType::Number);
	EXPECT_EQ(tokens[6].type, TokenType::Semicolon);
	EXPECT_EQ(tokens[7].type, TokenType::EndOfFile);
}

TEST(TokenizerTest, CreateTokenForCode_ParenthesizedExpression) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print (1 + 2) * 3;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_EQ(tokens.size(), 10u);
	EXPECT_EQ(tokens[0].type, TokenType::Print);
	EXPECT_EQ(tokens[1].type, TokenType::LParen);
	EXPECT_EQ(tokens[2].type, TokenType::Number);
	EXPECT_EQ(tokens[3].type, TokenType::Plus);
	EXPECT_EQ(tokens[4].type, TokenType::Number);
	EXPECT_EQ(tokens[5].type, TokenType::RParen);
	EXPECT_EQ(tokens[6].type, TokenType::Star);
	EXPECT_EQ(tokens[7].type, TokenType::Number);
	EXPECT_EQ(tokens[8].type, TokenType::Semicolon);
	EXPECT_EQ(tokens[9].type, TokenType::EndOfFile);
}

TEST(TokenizerTest, CreateTokenForCode_ChainedSubtraction) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 10 - 4 - 3;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_EQ(tokens.size(), 8u);
	EXPECT_EQ(tokens[1].type, TokenType::Number);
	EXPECT_EQ(tokens[2].type, TokenType::Minus);
	EXPECT_EQ(tokens[3].type, TokenType::Number);
	EXPECT_EQ(tokens[4].type, TokenType::Minus);
	EXPECT_EQ(tokens[5].type, TokenType::Number);
}

TEST(TokenizerTest, CreateTokenForCode_ChainedDivision) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 8 / 2 / 2;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_EQ(tokens.size(), 8u);
	EXPECT_EQ(tokens[1].type, TokenType::Number);
	EXPECT_EQ(tokens[2].type, TokenType::Slash);
	EXPECT_EQ(tokens[3].type, TokenType::Number);
	EXPECT_EQ(tokens[4].type, TokenType::Slash);
	EXPECT_EQ(tokens[5].type, TokenType::Number);
}

TEST(TokenizerTest, CreateTokenForCode_UnaryMinusProducesMinusThenNumber) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print -3 + 2;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_EQ(tokens.size(), 7u);
	EXPECT_EQ(tokens[1].type, TokenType::Minus);
	EXPECT_EQ(tokens[2].type, TokenType::Number);
	EXPECT_EQ(tokens[2].lexeme, "3");
	EXPECT_EQ(tokens[3].type, TokenType::Plus);
	EXPECT_EQ(tokens[4].type, TokenType::Number);
}

// 비교 연산자
TEST(TokenizerTest, CreateTokenForCode_LessThanOperator) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 1 < 2;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_EQ(tokens.size(), 6u);
	EXPECT_EQ(tokens[2].type, TokenType::Lt);
}

TEST(TokenizerTest, CreateTokenForCode_GreaterThanOperator) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 3 > 5;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_EQ(tokens.size(), 6u);
	EXPECT_EQ(tokens[2].type, TokenType::Gt);
}

// 문자열 리터럴 - 공백을 포함해도 하나의 단어/토큰이어야 함
TEST(TokenizerTest, SplitIntoWords_KeepsStringLiteralWithSpacesAsOneWord) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print \"Hello, \" + \"CodeFab!\";\n");

	std::vector<std::string> words = tokenizer.SplitIntoWords();

	EXPECT_THAT(words, ElementsAre("print", "\"Hello, \"", "+", "\"CodeFab!\"", ";"));
}

TEST(TokenizerTest, CreateTokenForCode_StringConcatenation) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print \"Hello, \" + \"CodeFab!\";\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	ASSERT_EQ(tokens.size(), 6u);
	EXPECT_EQ(tokens[1].type, TokenType::String);
	EXPECT_EQ(tokens[1].lexeme, "Hello, ");
	EXPECT_EQ(tokens[2].type, TokenType::Plus);
	EXPECT_EQ(tokens[3].type, TokenType::String);
	EXPECT_EQ(tokens[3].lexeme, "CodeFab!");
}

// 숫자 리터럴 (정수 / 소수)
TEST(TokenizerTest, CreateTokenForCode_ParsesIntegerLiteral) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 5;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	EXPECT_EQ(tokens[1].type, TokenType::Number);
	EXPECT_EQ(tokens[1].lexeme, "5");
	EXPECT_DOUBLE_EQ(tokens[1].realValue, 5.0);
}

TEST(TokenizerTest, CreateTokenForCode_ParsesDecimalLiteralWithTrailingZero) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 5.0;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	EXPECT_EQ(tokens[1].type, TokenType::Number);
	EXPECT_EQ(tokens[1].lexeme, "5.0");
	EXPECT_DOUBLE_EQ(tokens[1].realValue, 5.0);
}

TEST(TokenizerTest, CreateTokenForCode_ParsesDecimalLiteral) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print 3.14;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	EXPECT_EQ(tokens[1].type, TokenType::Number);
	EXPECT_EQ(tokens[1].lexeme, "3.14");
	EXPECT_DOUBLE_EQ(tokens[1].realValue, 3.14);
}

// boolean 리터럴
TEST(TokenizerTest, CreateTokenForCode_ClassifiesTrueLiteral) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print true;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	EXPECT_EQ(tokens[1].type, TokenType::KwTrue);
}

TEST(TokenizerTest, CreateTokenForCode_ClassifiesFalseLiteral) {
	Tokenizer tokenizer = MakeTokenizerWithInput("print false;\n");

	TokenList tokens = tokenizer.CreateTokenForCode();

	EXPECT_EQ(tokens[1].type, TokenType::KwFalse);
}
#endif
