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
#endif
