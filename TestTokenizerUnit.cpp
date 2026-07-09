#ifdef _DEBUG
#include "gmock/gmock.h"
#include "Tokenizer.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

using namespace testing;

class TokenizerTest : public Test {
protected:
	std::vector<std::filesystem::path> tempFiles;

	std::string CreateTempFile(const std::string& filename, const std::string& content) {
		std::filesystem::path path = std::filesystem::temp_directory_path() / filename;
		std::ofstream out(path);
		out << content;
		out.close();
		tempFiles.push_back(path);
		return path.string();
	}

	void TearDown() override {
		for (const std::filesystem::path& path : tempFiles) {
			std::filesystem::remove(path);
		}
	}

	Tokenizer MakeTokenizerWithInput(const std::string& input) {
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
		return tokenizer;
	}

	std::vector<std::string> SplitWords(const std::string& input) {
		return MakeTokenizerWithInput(input).SplitIntoWords();
	}

	TokenList CreateTokens(const std::string& input) {
		return MakeTokenizerWithInput(input).CreateTokenForCode();
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

	ASSERT_EQ(tokens.size(), 7u);
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

	ASSERT_EQ(tokens.size(), 6u);
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

// ===== Function declaration / call =====

TEST_F(TokenizerTest, SplitIntoWords_FunctionDeclaration) {
	EXPECT_THAT(SplitWords(
		"func add(a, b) {\n"
		"return a + b;\n"
		"}\n"),
		ElementsAre(
			"func", "add", "(", "a", ",", "b", ")", "{",
			"return", "a", "+", "b", ";",
			"}"));
}

TEST_F(TokenizerTest, SplitIntoWords_FunctionCallAndAssignment) {
	EXPECT_THAT(SplitWords("ret = add(3, 7);\n"),
		ElementsAre("ret", "=", "add", "(", "3", ",", "7", ")", ";"));
}

TEST_F(TokenizerTest, SplitIntoWords_BareReturnStatement) {
	EXPECT_THAT(SplitWords("return;\n"),
		ElementsAre("return", ";"));
}

TEST_F(TokenizerTest, CreateTokenForCode_FunctionDeclarationProducesExpectedTypeSequence) {
	TokenList tokens = CreateTokens(
		"func add(a, b) {\n"
		"return a + b;\n"
		"}\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwFunc, TokenType::Identifier, TokenType::LParen,
		TokenType::Identifier, TokenType::Comma, TokenType::Identifier, TokenType::RParen,
		TokenType::LBrace,
		TokenType::KwReturn, TokenType::Identifier, TokenType::Plus, TokenType::Identifier, TokenType::Semicolon,
		TokenType::RBrace,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_FunctionCallAssignedToVariable) {
	TokenList tokens = CreateTokens("ret = add(3, 7);\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Identifier, TokenType::Assign, TokenType::Identifier, TokenType::LParen,
		TokenType::Number, TokenType::Comma, TokenType::Number, TokenType::RParen, TokenType::Semicolon,
		TokenType::EndOfFile));

	EXPECT_EQ(tokens[0].lexeme, "ret");
	EXPECT_EQ(tokens[2].lexeme, "add");
	EXPECT_DOUBLE_EQ(tokens[4].realValue, 3.0);
	EXPECT_DOUBLE_EQ(tokens[6].realValue, 7.0);
}

TEST_F(TokenizerTest, CreateTokenForCode_BareReturnStatementHasNoOperand) {
	TokenList tokens = CreateTokens("return;\n");

	ASSERT_EQ(tokens.size(), 3u);
	EXPECT_EQ(tokens[0].type, TokenType::KwReturn);
	EXPECT_EQ(tokens[1].type, TokenType::Semicolon);
	EXPECT_EQ(tokens[2].type, TokenType::EndOfFile);
}

TEST_F(TokenizerTest, CreateTokenForCode_RecursiveFunctionDeclarationProducesExpectedTypeSequence) {
	TokenList tokens = CreateTokens(
		"func fact(n) {\n"
		"if (n <= 1) return 1;\n"
		"return n * fact(n - 1);\n"
		"}\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwFunc, TokenType::Identifier, TokenType::LParen, TokenType::Identifier, TokenType::RParen,
		TokenType::LBrace,
		TokenType::KwIf, TokenType::LParen, TokenType::Identifier, TokenType::LtEq, TokenType::Number, TokenType::RParen,
		TokenType::KwReturn, TokenType::Number, TokenType::Semicolon,
		TokenType::KwReturn, TokenType::Identifier, TokenType::Star, TokenType::Identifier,
		TokenType::LParen, TokenType::Identifier, TokenType::Minus, TokenType::Number, TokenType::RParen, TokenType::Semicolon,
		TokenType::RBrace,
		TokenType::EndOfFile));
}

// ===== Function call argument-count variations =====

TEST_F(TokenizerTest, CreateTokenForCode_FunctionCallWithNoArguments) {
	TokenList tokens = CreateTokens("hello();\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Identifier, TokenType::LParen, TokenType::RParen, TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_FunctionCallWithSingleArgument) {
	TokenList tokens = CreateTokens("add(5);\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Identifier, TokenType::LParen, TokenType::Number, TokenType::RParen, TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_FunctionCallWithThreeArguments) {
	TokenList tokens = CreateTokens("sum(1, 2, 3);\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Identifier, TokenType::LParen,
		TokenType::Number, TokenType::Comma, TokenType::Number, TokenType::Comma, TokenType::Number,
		TokenType::RParen, TokenType::Semicolon,
		TokenType::EndOfFile));
}

// ===== Nested function calls =====

TEST_F(TokenizerTest, CreateTokenForCode_NestedFunctionCallAsArgument) {
	TokenList tokens = CreateTokens("add(add(1, 2), 3);\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Identifier, TokenType::LParen,
		TokenType::Identifier, TokenType::LParen, TokenType::Number, TokenType::Comma, TokenType::Number, TokenType::RParen,
		TokenType::Comma, TokenType::Number,
		TokenType::RParen, TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_SingleArgumentNestedFunctionCall) {
	TokenList tokens = CreateTokens("fact(fact(2));\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Identifier, TokenType::LParen,
		TokenType::Identifier, TokenType::LParen, TokenType::Number, TokenType::RParen,
		TokenType::RParen, TokenType::Semicolon,
		TokenType::EndOfFile));
}

// ===== Function calls combined with operators in an expression =====

TEST_F(TokenizerTest, CreateTokenForCode_FunctionCallsCombinedWithArithmeticOperators) {
	TokenList tokens = CreateTokens("ret = add(1, 2) + add(3, 4) * 2;\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Identifier, TokenType::Assign,
		TokenType::Identifier, TokenType::LParen, TokenType::Number, TokenType::Comma, TokenType::Number, TokenType::RParen,
		TokenType::Plus,
		TokenType::Identifier, TokenType::LParen, TokenType::Number, TokenType::Comma, TokenType::Number, TokenType::RParen,
		TokenType::Star, TokenType::Number,
		TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_PrintOfFunctionCallResult) {
	TokenList tokens = CreateTokens("print add(1, 2);\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Print,
		TokenType::Identifier, TokenType::LParen, TokenType::Number, TokenType::Comma, TokenType::Number, TokenType::RParen,
		TokenType::Semicolon,
		TokenType::EndOfFile));
}

// ===== Class / this / super / instanceof keywords (case-insensitive) =====

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesClassKeywordCaseInsensitive) {
	EXPECT_EQ(CreateTokens("Class\n")[0].type, TokenType::KwClass);
	EXPECT_EQ(CreateTokens("class\n")[0].type, TokenType::KwClass);
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesThisKeywordCaseInsensitive) {
	EXPECT_EQ(CreateTokens("This\n")[0].type, TokenType::KwThis);
	EXPECT_EQ(CreateTokens("this\n")[0].type, TokenType::KwThis);
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesSuperKeywordCaseInsensitive) {
	EXPECT_EQ(CreateTokens("Super\n")[0].type, TokenType::KwSuper);
	EXPECT_EQ(CreateTokens("super\n")[0].type, TokenType::KwSuper);
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesInstanceofKeywordCaseInsensitive) {
	EXPECT_EQ(CreateTokens("instanceof\n")[0].type, TokenType::KwInstanceof);
	EXPECT_EQ(CreateTokens("InstanceOf\n")[0].type, TokenType::KwInstanceof);
}

TEST_F(TokenizerTest, CreateTokenForCode_ExistingKeywordsStayCaseInsensitiveToo) {
	EXPECT_EQ(CreateTokens("Var\n")[0].type, TokenType::KwVar);
	EXPECT_EQ(CreateTokens("PRINT\n")[0].type, TokenType::Print);
	EXPECT_EQ(CreateTokens("Func\n")[0].type, TokenType::KwFunc);
}

// ===== Class declaration =====

TEST_F(TokenizerTest, CreateTokenForCode_EmptyClassDeclaration) {
	TokenList tokens = CreateTokens("Class Robot { }\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwClass, TokenType::Identifier, TokenType::LBrace, TokenType::RBrace,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassInstantiationWithNoArguments) {
	TokenList tokens = CreateTokens("var robot = Robot();\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwVar, TokenType::Identifier, TokenType::Assign,
		TokenType::Identifier, TokenType::LParen, TokenType::RParen, TokenType::Semicolon,
		TokenType::EndOfFile));
}

// ===== Member access (.) =====

TEST_F(TokenizerTest, CreateTokenForCode_MemberAccessAssignmentAndPrint) {
	TokenList tokens = CreateTokens(
		"r.name = \"SpeedRobot\";\n"
		"r.speed = 10;\n"
		"print r.name;\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Identifier, TokenType::Dot, TokenType::Identifier, TokenType::Assign, TokenType::String, TokenType::Semicolon,
		TokenType::Identifier, TokenType::Dot, TokenType::Identifier, TokenType::Assign, TokenType::Number, TokenType::Semicolon,
		TokenType::Print, TokenType::Identifier, TokenType::Dot, TokenType::Identifier, TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_MemberAccessDoesNotDisruptDecimalNumbers) {
	TokenList tokens = CreateTokens("r.speed = 3.14;\n");

	ASSERT_EQ(tokens.size(), 7u);
	EXPECT_EQ(tokens[0].type, TokenType::Identifier);
	EXPECT_EQ(tokens[1].type, TokenType::Dot);
	EXPECT_EQ(tokens[2].type, TokenType::Identifier);
	EXPECT_EQ(tokens[3].type, TokenType::Assign);
	EXPECT_EQ(tokens[4].type, TokenType::Number);
	EXPECT_EQ(tokens[4].lexeme, "3.14");
	EXPECT_DOUBLE_EQ(tokens[4].realValue, 3.14);
	EXPECT_EQ(tokens[5].type, TokenType::Semicolon);
	EXPECT_EQ(tokens[6].type, TokenType::EndOfFile);
}

// ===== this / method declarations =====

TEST_F(TokenizerTest, CreateTokenForCode_MethodDeclarationUsingThisAndMemberAccess) {
	TokenList tokens = CreateTokens(
		"move(dist) {\n"
		"this.position = this.position + dist;\n"
		"}\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Identifier, TokenType::LParen, TokenType::Identifier, TokenType::RParen, TokenType::LBrace,
		TokenType::KwThis, TokenType::Dot, TokenType::Identifier, TokenType::Assign,
		TokenType::KwThis, TokenType::Dot, TokenType::Identifier, TokenType::Plus, TokenType::Identifier, TokenType::Semicolon,
		TokenType::RBrace,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_ConstructorStyleInitMethodWithMultipleParameters) {
	TokenList tokens = CreateTokens(
		"init(name, speed) {\n"
		"this.name = name;\n"
		"this.speed = speed;\n"
		"}\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Identifier, TokenType::LParen, TokenType::Identifier, TokenType::Comma, TokenType::Identifier, TokenType::RParen,
		TokenType::LBrace,
		TokenType::KwThis, TokenType::Dot, TokenType::Identifier, TokenType::Assign, TokenType::Identifier, TokenType::Semicolon,
		TokenType::KwThis, TokenType::Dot, TokenType::Identifier, TokenType::Assign, TokenType::Identifier, TokenType::Semicolon,
		TokenType::RBrace,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassInstantiationWithConstructorArguments) {
	TokenList tokens = CreateTokens("var r = Robot(\"AndOr\", 10);\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwVar, TokenType::Identifier, TokenType::Assign,
		TokenType::Identifier, TokenType::LParen, TokenType::String, TokenType::Comma, TokenType::Number, TokenType::RParen,
		TokenType::Semicolon,
		TokenType::EndOfFile));
}

// ===== Inheritance (:) and super =====

TEST_F(TokenizerTest, CreateTokenForCode_InheritanceDeclarationUsesColon) {
	TokenList tokens = CreateTokens("Class SpeedRobot : Robot { }\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwClass, TokenType::Identifier, TokenType::Colon, TokenType::Identifier,
		TokenType::LBrace, TokenType::RBrace,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_SuperMethodCallInsideOverride) {
	TokenList tokens = CreateTokens("super.move(dist);\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwSuper, TokenType::Dot, TokenType::Identifier, TokenType::LParen, TokenType::Identifier, TokenType::RParen, TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_ChainedCallOnInlineInstantiation) {
	TokenList tokens = CreateTokens("SpeedRobot().move(3);\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Identifier, TokenType::LParen, TokenType::RParen,
		TokenType::Dot, TokenType::Identifier, TokenType::LParen, TokenType::Number, TokenType::RParen, TokenType::Semicolon,
		TokenType::EndOfFile));
}

// ===== instanceof =====

TEST_F(TokenizerTest, CreateTokenForCode_InstanceofExpression) {
	TokenList tokens = CreateTokens("print (w instanceof SpeedRobot);\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Print, TokenType::LParen, TokenType::Identifier, TokenType::KwInstanceof, TokenType::Identifier, TokenType::RParen,
		TokenType::Semicolon,
		TokenType::EndOfFile));
}

// ===== Static array: creation, index write, index read =====

TEST_F(TokenizerTest, SplitIntoWords_ArrayCreationAndIndexAccess) {
	EXPECT_THAT(SplitWords(
		"var arr = Array(3);\n"
		"arr[0] = 10;\n"
		"print arr[0];\n"),
		ElementsAre(
			"var", "arr", "=", "Array", "(", "3", ")", ";",
			"arr", "[", "0", "]", "=", "10", ";",
			"print", "arr", "[", "0", "]", ";"));
}

TEST_F(TokenizerTest, CreateTokenForCode_ArrayCreationProducesExpectedTypeSequence) {
	TokenList tokens = CreateTokens("var arr = Array(3);\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwVar, TokenType::Identifier, TokenType::Assign,
		TokenType::Identifier, TokenType::LParen, TokenType::Number, TokenType::RParen, TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_ArrayIndexWriteProducesExpectedTypeSequence) {
	TokenList tokens = CreateTokens("arr[0] = 10;\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Identifier, TokenType::LBracket, TokenType::Number, TokenType::RBracket,
		TokenType::Assign, TokenType::Number, TokenType::Semicolon,
		TokenType::EndOfFile));

	EXPECT_EQ(tokens[0].lexeme, "arr");
	EXPECT_DOUBLE_EQ(tokens[2].realValue, 0.0);
	EXPECT_DOUBLE_EQ(tokens[5].realValue, 10.0);
}

TEST_F(TokenizerTest, CreateTokenForCode_ArrayIndexReadInsidePrint) {
	TokenList tokens = CreateTokens("print arr[0];\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Print, TokenType::Identifier, TokenType::LBracket, TokenType::Number, TokenType::RBracket,
		TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_ArrayIndexWriteWithExpressionIndex) {
	TokenList tokens = CreateTokens(
		"var i = 2;\n"
		"arr[i - 1] = 7;\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwVar, TokenType::Identifier, TokenType::Assign, TokenType::Number, TokenType::Semicolon,
		TokenType::Identifier, TokenType::LBracket, TokenType::Identifier, TokenType::Minus, TokenType::Number, TokenType::RBracket,
		TokenType::Assign, TokenType::Number, TokenType::Semicolon,
		TokenType::EndOfFile));
}

// ===== Static array: lexically-valid-but-semantically-invalid inputs =====
// (범위 초과/타입 오류 같은 의미 검증은 CheckerUnit/ExecutorUnit 몫이며, 토크나이저는
//  아래 입력들도 그대로 토큰화만 하면 된다.)

TEST_F(TokenizerTest, CreateTokenForCode_ArrayIndexWithOutOfRangeNumberStillTokenizes) {
	TokenList tokens = CreateTokens("print arr[5];\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Print, TokenType::Identifier, TokenType::LBracket, TokenType::Number, TokenType::RBracket,
		TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_ArrayIndexWithStringStillTokenizes) {
	TokenList tokens = CreateTokens("print arr[\"hello\"];\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Print, TokenType::Identifier, TokenType::LBracket, TokenType::String, TokenType::RBracket,
		TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_IndexingNonArrayVariableStillTokenizes) {
	TokenList tokens = CreateTokens("print x[0];\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Print, TokenType::Identifier, TokenType::LBracket, TokenType::Number, TokenType::RBracket,
		TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_ArrayCreationWithStringSizeStillTokenizes) {
	TokenList tokens = CreateTokens("var brr = Array(\"hi\");\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwVar, TokenType::Identifier, TokenType::Assign,
		TokenType::Identifier, TokenType::LParen, TokenType::String, TokenType::RParen, TokenType::Semicolon,
		TokenType::EndOfFile));
}

// ===== 논리 연산자: !, &&, || =====

TEST_F(TokenizerTest, SplitIntoWords_LogicalNotAndAndOr) {
	EXPECT_THAT(SplitWords("if (!a && b || !c) print a;\n"),
		ElementsAre(
			"if", "(", "!", "a", "&&", "b", "||", "!", "c", ")", "print", "a", ";"));
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesLogicalNot) {
	TokenList tokens = CreateTokens("if(!a)\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwIf, TokenType::LParen, TokenType::Not, TokenType::Identifier, TokenType::RParen,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesLogicalAndOr) {
	TokenList tokens = CreateTokens("print a && b || c;\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Print, TokenType::Identifier, TokenType::And, TokenType::Identifier,
		TokenType::Or, TokenType::Identifier, TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_LogicalOperatorsWithoutSurroundingSpaces) {
	TokenList tokens = CreateTokens("print a&&b||!c;\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Print, TokenType::Identifier, TokenType::And, TokenType::Identifier,
		TokenType::Or, TokenType::Not, TokenType::Identifier, TokenType::Semicolon,
		TokenType::EndOfFile));
}

// ===== import 구문 =====

TEST_F(TokenizerTest, SplitIntoWords_ImportStatement) {
	EXPECT_THAT(SplitWords("import \"math.cf\" alias math;\n"),
		ElementsAre("import", "\"math.cf\"", "alias", "math", ";"));
}

TEST_F(TokenizerTest, CreateTokenForCode_ImportStatementProducesExpectedTypeSequence) {
	std::string path = CreateTempFile("codefab_test_import_seq.cf", "");

	TokenList tokens = CreateTokens("import \"" + path + "\" alias math;\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwImport, TokenType::String, TokenType::KwAlias, TokenType::Identifier, TokenType::Semicolon,
		TokenType::EndOfFile));

	EXPECT_EQ(tokens[1].lexeme, path);
	EXPECT_EQ(tokens[3].lexeme, "math");
}

TEST_F(TokenizerTest, CreateTokenForCode_ClassifiesImportAndAliasKeywordsCaseInsensitive) {
	EXPECT_EQ(CreateTokens("Import\n")[0].type, TokenType::KwImport);
	EXPECT_EQ(CreateTokens("import\n")[0].type, TokenType::KwImport);
	EXPECT_EQ(CreateTokens("Alias\n")[0].type, TokenType::KwAlias);
	EXPECT_EQ(CreateTokens("alias\n")[0].type, TokenType::KwAlias);
}

// ===== import 대상 파일 토큰화 (import 문 바로 뒤에 삽입) =====

TEST_F(TokenizerTest, CreateTokenForCode_ImportSplicesImportedFileTokensRightAfterStatement) {
	std::string path = CreateTempFile("codefab_test_math.cf", "func add(a, b) {\nreturn a + b;\n}\n");

	TokenList tokens = CreateTokens("import \"" + path + "\" alias math;\nprint 1;\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwImport, TokenType::String, TokenType::KwAlias, TokenType::Identifier, TokenType::Semicolon,
		TokenType::KwFunc, TokenType::Identifier, TokenType::LParen,
		TokenType::Identifier, TokenType::Comma, TokenType::Identifier, TokenType::RParen,
		TokenType::LBrace,
		TokenType::KwReturn, TokenType::Identifier, TokenType::Plus, TokenType::Identifier, TokenType::Semicolon,
		TokenType::RBrace,
		TokenType::Print, TokenType::Number, TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_ImportRecursivelySplicesNestedImports) {
	std::string innerPath = CreateTempFile("codefab_test_inner.cf", "var b = 2;\n");
	std::string outerPath = CreateTempFile("codefab_test_outer.cf", "import \"" + innerPath + "\" alias inner;\nvar a = 1;\n");

	TokenList tokens = CreateTokens("import \"" + outerPath + "\" alias outer;\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::KwImport, TokenType::String, TokenType::KwAlias, TokenType::Identifier, TokenType::Semicolon,
		TokenType::KwImport, TokenType::String, TokenType::KwAlias, TokenType::Identifier, TokenType::Semicolon,
		TokenType::KwVar, TokenType::Identifier, TokenType::Assign, TokenType::Number, TokenType::Semicolon,
		TokenType::KwVar, TokenType::Identifier, TokenType::Assign, TokenType::Number, TokenType::Semicolon,
		TokenType::EndOfFile));
}

TEST_F(TokenizerTest, CreateTokenForCode_ImportOfMissingFileThrows) {
	std::filesystem::path missing = std::filesystem::temp_directory_path() / "codefab_test_does_not_exist.cf";
	std::filesystem::remove(missing);

	try {
		CreateTokens("import \"" + missing.string() + "\" alias missing;\n");
		FAIL();
	}
	catch (const std::runtime_error& e) {
		EXPECT_STREQ(e.what(), ("Cannot open import file: " + missing.string() + " at line 0").c_str());
	}
}

TEST_F(TokenizerTest, CreateTokenForCode_CircularImportThrows) {
	std::filesystem::path pathA = std::filesystem::temp_directory_path() / "codefab_test_cycle_a.cf";
	std::filesystem::path pathB = std::filesystem::temp_directory_path() / "codefab_test_cycle_b.cf";
	tempFiles.push_back(pathA);
	tempFiles.push_back(pathB);

	std::ofstream(pathA) << "import \"" + pathB.string() + "\" alias b;\n";
	std::ofstream(pathB) << "import \"" + pathA.string() + "\" alias a;\n";

	try {
		CreateTokens("import \"" + pathA.string() + "\" alias a;\n");
		FAIL();
	}
	catch (const std::runtime_error& e) {
		EXPECT_STREQ(e.what(), ("Circular import detected: " + pathA.string() + " at line 0").c_str());
	}
}

TEST_F(TokenizerTest, CreateTokenForCode_NotEqualStillTokenizesAsSingleOperator) {
	TokenList tokens = CreateTokens("print a != b;\n");

	std::vector<TokenType> types;
	for (const Token& token : tokens) {
		types.push_back(token.type);
	}

	EXPECT_THAT(types, ElementsAre(
		TokenType::Print, TokenType::Identifier, TokenType::NotEq, TokenType::Identifier, TokenType::Semicolon,
		TokenType::EndOfFile));
}
#endif
