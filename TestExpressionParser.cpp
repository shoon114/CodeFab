#ifdef _DEBUG
#include "gmock/gmock.h"
#include "ExpressionParser.h"
#include "Tokenizer.h"
#include "TestTokenHelpers.h"
#include <stdexcept>

using namespace testing;

class TestableTokenizer : public Tokenizer {
public:
	MOCK_METHOD(TokenList, CreateTokenForCode, (), (override));
};

class ExpressionParserTest : public Test {
protected:
	ExpressionParser parser;
	NiceMock<TestableTokenizer> tokenizer;

	TokenList MakeTokens(const TokenList& tokenList) {
		EXPECT_CALL(tokenizer, CreateTokenForCode()).WillOnce(Return(tokenList));
		return tokenizer.CreateTokenForCode();
	}
};

TEST_F(ExpressionParserTest, Parse_NumberLiteral) {
	TokenList tokenList = MakeTokens({ MakeToken(TokenType::Number, "1", 0, 0) });
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::NumberLiteral));
	EXPECT_THAT(pos, Eq(1u));
}

TEST_F(ExpressionParserTest, Parse_BinaryAddition) {
	// "a + 1"
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "a", 0, 0),
		MakeToken(TokenType::Plus, "+", 0, 2),
		MakeToken(TokenType::Number, "1", 0, 4),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::BinaryExpr));
	ASSERT_THAT(node->children.size(), Eq(2u));
	EXPECT_THAT(node->children[0]->type, Eq(NodeType::Identifier));
	EXPECT_THAT(node->children[1]->type, Eq(NodeType::NumberLiteral));
	EXPECT_THAT(pos, Eq(3u));
}

TEST_F(ExpressionParserTest, Parse_RespectsPrecedence_MultiplicationBeforeAddition) {
	// "1 + 2 * 3" -> 1 + (2 * 3)
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Number, "1", 0, 0),
		MakeToken(TokenType::Plus, "+", 0, 2),
		MakeToken(TokenType::Number, "2", 0, 4),
		MakeToken(TokenType::Star, "*", 0, 6),
		MakeToken(TokenType::Number, "3", 0, 8),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(node->token.type, Eq(TokenType::Plus));
	EXPECT_THAT(node->children[0]->type, Eq(NodeType::NumberLiteral));

	SyntaxNode* rightNode = node->children[1].get();
	ASSERT_THAT(rightNode->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(rightNode->token.type, Eq(TokenType::Star));
}

TEST_F(ExpressionParserTest, Parse_Parenthesized_OverridesPrecedence) {
	// "(1 + 2) * 3" -> (1 + 2) * 3
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::LParen, "(", 0, 0),
		MakeToken(TokenType::Number, "1", 0, 1),
		MakeToken(TokenType::Plus, "+", 0, 3),
		MakeToken(TokenType::Number, "2", 0, 5),
		MakeToken(TokenType::RParen, ")", 0, 6),
		MakeToken(TokenType::Star, "*", 0, 8),
		MakeToken(TokenType::Number, "3", 0, 10),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(node->token.type, Eq(TokenType::Star));

	SyntaxNode* leftNode = node->children[0].get();
	ASSERT_THAT(leftNode->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(leftNode->token.type, Eq(TokenType::Plus));
}

TEST_F(ExpressionParserTest, Parse_UnaryMinus) {
	// "-1"
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Minus, "-", 0, 0),
		MakeToken(TokenType::Number, "1", 0, 1),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::UnaryExpr));
	ASSERT_THAT(node->children.size(), Eq(1u));
	EXPECT_THAT(node->children[0]->type, Eq(NodeType::NumberLiteral));
}

TEST_F(ExpressionParserTest, Parse_StringLiteral) {
	TokenList tokenList = MakeTokens({ MakeToken(TokenType::String, "hello", 0, 0) });
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::StringLiteral));
}

TEST_F(ExpressionParserTest, Parse_BoolLiteral_True) {
	TokenList tokenList = MakeTokens({ MakeToken(TokenType::KwTrue, "true", 0, 0) });
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::BoolLiteral));
}

TEST_F(ExpressionParserTest, Parse_Subtraction_IsLeftAssociative) {
	// "1 - 2 - 3" -> (1 - 2) - 3
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Number, "1", 0, 0),
		MakeToken(TokenType::Minus, "-", 0, 2),
		MakeToken(TokenType::Number, "2", 0, 4),
		MakeToken(TokenType::Minus, "-", 0, 6),
		MakeToken(TokenType::Number, "3", 0, 8),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(node->token.type, Eq(TokenType::Minus));

	SyntaxNode* leftNode = node->children[0].get();
	ASSERT_THAT(leftNode->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(leftNode->token.type, Eq(TokenType::Minus));
	EXPECT_THAT(node->children[1]->type, Eq(NodeType::NumberLiteral));
}
#endif
