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

	TokenList CreateTokens(const std::string& input) {
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
		return tokenizer.CreateTokenForCode();
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

// ===== CreateTokenForCode: type / lexeme / line / column =====

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesNumber) {
	TokenList tokens = CreateTokens("10\n");

	ASSERT_FALSE(tokens.empty());
	EXPECT_EQ(tokens[0].type, TokenType::Number);
	EXPECT_EQ(tokens[0].lexeme, "10");
	EXPECT_DOUBLE_EQ(tokens[0].realValue, 10.0);
	EXPECT_EQ(tokens[0].line, 0);
	EXPECT_EQ(tokens[0].column, 0);
	EXPECT_EQ(tokens.back().type, TokenType::EndOfFile);
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesIdentifier) {
	TokenList tokens = CreateTokens("a\n");

	ASSERT_FALSE(tokens.empty());
	EXPECT_EQ(tokens[0].type, TokenType::Identifier);
	EXPECT_EQ(tokens[0].lexeme, "a");
	EXPECT_EQ(tokens[0].line, 0);
	EXPECT_EQ(tokens[0].column, 0);
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesIfKeyword) {
	TokenList tokens = CreateTokens("if\n");

	ASSERT_FALSE(tokens.empty());
	EXPECT_EQ(tokens[0].type, TokenType::KwIf);
	EXPECT_EQ(tokens[0].line, 0);
	EXPECT_EQ(tokens[0].column, 0);
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesPrintKeywordAndOperandsWithColumns) {
	TokenList tokens = CreateTokens("print 5;\n");

	ASSERT_EQ(tokens.size(), 4u);
	EXPECT_EQ(tokens[0].type, TokenType::Print);
	EXPECT_EQ(tokens[0].line, 0);
	EXPECT_EQ(tokens[0].column, 0);

	EXPECT_EQ(tokens[1].type, TokenType::Number);
	EXPECT_EQ(tokens[1].lexeme, "5");
	EXPECT_EQ(tokens[1].column, 1);

	EXPECT_EQ(tokens[2].type, TokenType::Semicolon);
	EXPECT_EQ(tokens[2].column, 2);

	EXPECT_EQ(tokens[3].type, TokenType::EndOfFile);
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesOperatorsAndParenthesesWithColumns) {
	TokenList tokens = CreateTokens("if(a>10)\n");

	ASSERT_EQ(tokens.size(), 7u);
	EXPECT_EQ(tokens[0].type, TokenType::KwIf);
	EXPECT_EQ(tokens[0].column, 0);

	EXPECT_EQ(tokens[1].type, TokenType::LParen);
	EXPECT_EQ(tokens[1].column, 1);

	EXPECT_EQ(tokens[2].type, TokenType::Identifier);
	EXPECT_EQ(tokens[2].column, 2);

	EXPECT_EQ(tokens[3].type, TokenType::Gt);
	EXPECT_EQ(tokens[3].column, 3);

	EXPECT_EQ(tokens[4].type, TokenType::Number);
	EXPECT_EQ(tokens[4].column, 4);

	EXPECT_EQ(tokens[5].type, TokenType::RParen);
	EXPECT_EQ(tokens[5].column, 5);

	EXPECT_EQ(tokens[6].type, TokenType::EndOfFile);
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesComparisonOperators) {
	TokenList lessThan = CreateTokens("print 1 < 2;\n");
	ASSERT_GE(lessThan.size(), 3u);
	EXPECT_EQ(lessThan[2].type, TokenType::Lt);
	EXPECT_EQ(lessThan[2].column, 2);

	TokenList greaterThan = CreateTokens("print 3 > 5;\n");
	ASSERT_GE(greaterThan.size(), 3u);
	EXPECT_EQ(greaterThan[2].type, TokenType::Gt);
	EXPECT_EQ(greaterThan[2].column, 2);
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesChainedArithmeticWithColumns) {
	TokenList tokens = CreateTokens("print 10 - 4 - 3;\n");

	ASSERT_EQ(tokens.size(), 8u);
	EXPECT_EQ(tokens[1].type, TokenType::Number);
	EXPECT_EQ(tokens[1].lexeme, "10");
	EXPECT_EQ(tokens[1].column, 1);

	EXPECT_EQ(tokens[2].type, TokenType::Minus);
	EXPECT_EQ(tokens[2].column, 2);

	EXPECT_EQ(tokens[3].type, TokenType::Number);
	EXPECT_EQ(tokens[3].lexeme, "4");
	EXPECT_EQ(tokens[3].column, 3);

	EXPECT_EQ(tokens[4].type, TokenType::Minus);
	EXPECT_EQ(tokens[4].column, 4);

	EXPECT_EQ(tokens[5].type, TokenType::Number);
	EXPECT_EQ(tokens[5].lexeme, "3");
	EXPECT_EQ(tokens[5].column, 5);
}

TEST_F(TokenizerTest, CreateTokenForCode_UnaryMinusProducesMinusThenNumber) {
	TokenList tokens = CreateTokens("print -3 + 2;\n");

	ASSERT_EQ(tokens.size(), 6u);
	EXPECT_EQ(tokens[1].type, TokenType::Minus);
	EXPECT_EQ(tokens[1].column, 1);

	EXPECT_EQ(tokens[2].type, TokenType::Number);
	EXPECT_EQ(tokens[2].lexeme, "3");
	EXPECT_EQ(tokens[2].column, 2);

	EXPECT_EQ(tokens[3].type, TokenType::Plus);
	EXPECT_EQ(tokens[3].column, 3);
}

TEST_F(TokenizerTest, CreateTokenForCode_StringLiteralLexemeHasQuotesStripped) {
	TokenList tokens = CreateTokens("print \"Hello, \" + \"CodeFab!\";\n");

	ASSERT_EQ(tokens.size(), 5u);
	EXPECT_EQ(tokens[0].type, TokenType::Print);
	EXPECT_EQ(tokens[0].column, 0);

	EXPECT_EQ(tokens[1].type, TokenType::String);
	EXPECT_EQ(tokens[1].lexeme, "Hello, ");
	EXPECT_EQ(tokens[1].column, 1);

	EXPECT_EQ(tokens[2].type, TokenType::Plus);
	EXPECT_EQ(tokens[2].column, 2);

	EXPECT_EQ(tokens[3].type, TokenType::String);
	EXPECT_EQ(tokens[3].lexeme, "CodeFab!");
	EXPECT_EQ(tokens[3].column, 3);

	EXPECT_EQ(tokens[4].type, TokenType::Semicolon);
	EXPECT_EQ(tokens[4].column, 4);
}

TEST_F(TokenizerTest, CreateTokenForCode_ParsesIntegerAndDecimalLiterals) {
	EXPECT_DOUBLE_EQ(CreateTokens("print 5;\n")[1].realValue, 5.0);
	EXPECT_EQ(CreateTokens("print 5;\n")[1].lexeme, "5");

	EXPECT_DOUBLE_EQ(CreateTokens("print 5.0;\n")[1].realValue, 5.0);
	EXPECT_EQ(CreateTokens("print 5.0;\n")[1].lexeme, "5.0");

	EXPECT_DOUBLE_EQ(CreateTokens("print 3.14;\n")[1].realValue, 3.14);
	EXPECT_EQ(CreateTokens("print 3.14;\n")[1].lexeme, "3.14");
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesBooleanLiteralsWithColumns) {
	TokenList trueTokens = CreateTokens("print true;\n");
	ASSERT_GE(trueTokens.size(), 2u);
	EXPECT_EQ(trueTokens[1].type, TokenType::KwTrue);
	EXPECT_EQ(trueTokens[1].column, 1);

	TokenList falseTokens = CreateTokens("print false;\n");
	ASSERT_GE(falseTokens.size(), 2u);
	EXPECT_EQ(falseTokens[1].type, TokenType::KwFalse);
	EXPECT_EQ(falseTokens[1].column, 1);
}

TEST_F(TokenizerTest, CreateTokenForCode_TracksLineNumberAcrossMultipleStatements) {
	TokenList tokens = CreateTokens(
		"var a = 10;\n"
		"var b = 20;\n"
		"print a + b;\n");

	ASSERT_EQ(tokens.size(), 16u);

	EXPECT_EQ(tokens[0].type, TokenType::KwVar);
	EXPECT_EQ(tokens[0].line, 0);
	EXPECT_EQ(tokens[0].column, 0);

	EXPECT_EQ(tokens[1].type, TokenType::Identifier);
	EXPECT_EQ(tokens[1].line, 0);
	EXPECT_EQ(tokens[1].column, 1);

	EXPECT_EQ(tokens[5].type, TokenType::KwVar);
	EXPECT_EQ(tokens[5].line, 1);
	EXPECT_EQ(tokens[5].column, 0);

	EXPECT_EQ(tokens[10].type, TokenType::Print);
	EXPECT_EQ(tokens[10].line, 2);
	EXPECT_EQ(tokens[10].column, 0);

	EXPECT_EQ(tokens[15].type, TokenType::EndOfFile);
}

TEST_F(TokenizerTest, CreateTokenForCode_BlockScopeShadowingProducesBraceTokens) {
	TokenList tokens = CreateTokens(
		"var x = \"global\";\n"
		"{\n"
		"  var x = \"inner\";\n"
		"  print x;\n"
		"}\n"
		"print x;\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwVar, TokenType::Identifier, TokenType::Assign, TokenType::String, TokenType::Semicolon,
		TokenType::LBrace,
		TokenType::KwVar, TokenType::Identifier, TokenType::Assign, TokenType::String, TokenType::Semicolon,
		TokenType::Print, TokenType::Identifier, TokenType::Semicolon,
		TokenType::RBrace,
		TokenType::Print, TokenType::Identifier, TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_IfElseProducesExpectedTypeSequence) {
	TokenList tokens = CreateTokens("if (false) print \"no\"; else print \"kfc\";\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwIf, TokenType::LParen, TokenType::KwFalse, TokenType::RParen,
		TokenType::Print, TokenType::String, TokenType::Semicolon,
		TokenType::KwElse,
		TokenType::Print, TokenType::String, TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_AppendsEndOfFileToken) {
	TokenList tokens = CreateTokens("a\n");

	ASSERT_FALSE(tokens.empty());
	EXPECT_EQ(tokens.back().type, TokenType::EndOfFile);
}

TEST_F(TokenizerTest, CreateTokenForCode_HandlesEmptyInputWithoutCrashing) {
	TokenList tokens = CreateTokens("\n");

	ASSERT_EQ(tokens.size(), 1u);
	EXPECT_EQ(tokens[0].type, TokenType::EndOfFile);
}
#endif
