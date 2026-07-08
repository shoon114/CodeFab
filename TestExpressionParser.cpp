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

TEST_F(ExpressionParserTest, Parse_Identifier) {
	TokenList tokenList = MakeTokens({ MakeToken(TokenType::Identifier, "a", 0, 0) });
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::Identifier));
	EXPECT_THAT(pos, Eq(1u));
}

TEST_F(ExpressionParserTest, Parse_BinaryAddition) {
	// "a + 1"
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "a", 0, 0),
		MakeToken(TokenType::Plus, "+", 0, 1),
		MakeToken(TokenType::Number, "1", 0, 2),
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
		MakeToken(TokenType::Plus, "+", 0, 1),
		MakeToken(TokenType::Number, "2", 0, 2),
		MakeToken(TokenType::Star, "*", 0, 3),
		MakeToken(TokenType::Number, "3", 0, 4),
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
		MakeToken(TokenType::Plus, "+", 0, 2),
		MakeToken(TokenType::Number, "2", 0, 3),
		MakeToken(TokenType::RParen, ")", 0, 4),
		MakeToken(TokenType::Star, "*", 0, 5),
		MakeToken(TokenType::Number, "3", 0, 6),
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
		MakeToken(TokenType::Minus, "-", 0, 1),
		MakeToken(TokenType::Number, "2", 0, 2),
		MakeToken(TokenType::Minus, "-", 0, 3),
		MakeToken(TokenType::Number, "3", 0, 4),
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

TEST_F(ExpressionParserTest, Parse_RespectsPrecedence_ComparisonBeforeEquality) {
	// "a < 1 == b < 2" -> (a < 1) == (b < 2)
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "a", 0, 0),
		MakeToken(TokenType::Lt, "<", 0, 1),
		MakeToken(TokenType::Number, "1", 0, 2),
		MakeToken(TokenType::Eq, "==", 0, 3),
		MakeToken(TokenType::Identifier, "b", 0, 4),
		MakeToken(TokenType::Lt, "<", 0, 5),
		MakeToken(TokenType::Number, "2", 0, 6),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(node->token.type, Eq(TokenType::Eq));

	SyntaxNode* leftNode = node->children[0].get();
	SyntaxNode* rightNode = node->children[1].get();
	ASSERT_THAT(leftNode->type, Eq(NodeType::BinaryExpr));
	ASSERT_THAT(rightNode->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(leftNode->token.type, Eq(TokenType::Lt));
	EXPECT_THAT(rightNode->token.type, Eq(TokenType::Lt));
}

TEST_F(ExpressionParserTest, Parse_RespectsPrecedence_AndBeforeOr) {
	// "a or b and c" -> a or (b and c)
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "a", 0, 0),
		MakeToken(TokenType::Or, "or", 0, 1),
		MakeToken(TokenType::Identifier, "b", 0, 2),
		MakeToken(TokenType::And, "and", 0, 3),
		MakeToken(TokenType::Identifier, "c", 0, 4),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(node->token.type, Eq(TokenType::Or));
	EXPECT_THAT(node->children[0]->type, Eq(NodeType::Identifier));

	SyntaxNode* rightNode = node->children[1].get();
	ASSERT_THAT(rightNode->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(rightNode->token.type, Eq(TokenType::And));
}

TEST_F(ExpressionParserTest, Parse_NestedParentheses) {
	// "((1))"
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::LParen, "(", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::Number, "1", 0, 2),
		MakeToken(TokenType::RParen, ")", 0, 3),
		MakeToken(TokenType::RParen, ")", 0, 4),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::NumberLiteral));
	EXPECT_THAT(pos, Eq(5u));
}

TEST_F(ExpressionParserTest, Parse_Assignment) {
	// "a = 1"
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "a", 0, 0),
		MakeToken(TokenType::Assign, "=", 0, 1),
		MakeToken(TokenType::Number, "1", 0, 2),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::AssignExpr));
	EXPECT_THAT(node->children[0]->type, Eq(NodeType::Identifier));
	EXPECT_THAT(node->children[1]->type, Eq(NodeType::NumberLiteral));
}

TEST_F(ExpressionParserTest, Parse_EmptyTokenList_Throws) {
	TokenList tokenList = MakeTokens({});
	size_t pos = 0;

	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(ExpressionParserTest, Parse_UnexpectedToken_Throws) {
	// a bare ')' can never start an expression
	TokenList tokenList = MakeTokens({ MakeToken(TokenType::RParen, ")", 0, 0) });
	size_t pos = 0;

	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(ExpressionParserTest, Parse_MissingRightOperand_Throws) {
	// "1 +" -- operator with nothing after it
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Number, "1", 0, 0),
		MakeToken(TokenType::Plus, "+", 0, 1),
		});
	size_t pos = 0;

	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(ExpressionParserTest, Parse_UnclosedParenthesis_Throws) {
	// "(1" -- missing ')'
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::LParen, "(", 0, 0),
		MakeToken(TokenType::Number, "1", 0, 1),
		});
	size_t pos = 0;

	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(ExpressionParserTest, Parse_CallExpr_NoArgs) {
	// "func()"
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "func", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::RParen, ")", 0, 2),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::CallExpr));
	EXPECT_THAT(node->token.lexeme, Eq("func"));
	EXPECT_THAT(node->children.size(), Eq(0u));
	EXPECT_THAT(pos, Eq(3u));
}

TEST_F(ExpressionParserTest, Parse_CallExpr_OneArg) {
	// "func(3)"
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "func", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::Number, "3", 0, 2),
		MakeToken(TokenType::RParen, ")", 0, 3),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::CallExpr));
	EXPECT_THAT(node->token.lexeme, Eq("func"));
	ASSERT_THAT(node->children.size(), Eq(1u));
	EXPECT_THAT(node->children[0]->type, Eq(NodeType::NumberLiteral));
}

TEST_F(ExpressionParserTest, Parse_ArrExpr_Array3) {
	// "Array(3)"
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "Array", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::Number, "3", 0, 2),
		MakeToken(TokenType::RParen, ")", 0, 3),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::ArrExpr));
	EXPECT_THAT(node->token.lexeme, Eq("Array"));
	ASSERT_THAT(node->children.size(), Eq(1u));
	EXPECT_THAT(node->children[0]->type, Eq(NodeType::NumberLiteral));
	EXPECT_THAT(node->children[0]->token.lexeme, Eq("3"));
}

TEST_F(ExpressionParserTest, Parse_ArrExpr_SizeIsExpression) {
	// "Array(n + 1)"
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "Array", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::Identifier, "n", 0, 2),
		MakeToken(TokenType::Plus, "+", 0, 3),
		MakeToken(TokenType::Number, "1", 0, 4),
		MakeToken(TokenType::RParen, ")", 0, 5),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::ArrExpr));
	ASSERT_THAT(node->children.size(), Eq(1u));
	EXPECT_THAT(node->children[0]->type, Eq(NodeType::BinaryExpr));
}

TEST_F(ExpressionParserTest, Parse_ArrExpr_MissingSizeArgument_Throws) {
	// "Array()" -- Array는 크기 인자가 반드시 있어야 한다
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "Array", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::RParen, ")", 0, 2),
		});
	size_t pos = 0;

	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(ExpressionParserTest, Parse_ArrExpr_TooManyArguments_Throws) {
	// "Array(3, 4)" -- Array는 인자를 하나만 받는다
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "Array", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::Number, "3", 0, 2),
		MakeToken(TokenType::Comma, ",", 0, 3),
		MakeToken(TokenType::Number, "4", 0, 4),
		MakeToken(TokenType::RParen, ")", 0, 5),
		});
	size_t pos = 0;

	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(ExpressionParserTest, Parse_ArrExpr_UnclosedParenthesis_Throws) {
	// "Array(3" -- missing ')'
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "Array", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::Number, "3", 0, 2),
		});
	size_t pos = 0;

	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(ExpressionParserTest, Parse_CallExpr_MultipleArgs) {
	// "func(1, 2)"
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "func", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::Number, "1", 0, 2),
		MakeToken(TokenType::Comma, ",", 0, 3),
		MakeToken(TokenType::Number, "2", 0, 4),
		MakeToken(TokenType::RParen, ")", 0, 5),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::CallExpr));
	ASSERT_THAT(node->children.size(), Eq(2u));
	EXPECT_THAT(node->children[0]->token.lexeme, Eq("1"));
	EXPECT_THAT(node->children[1]->token.lexeme, Eq("2"));
}

TEST_F(ExpressionParserTest, Parse_CallExpr_NonIdentifierCallee_Throws) {
	// "1(2)" -- only identifiers can be called
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Number, "1", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::Number, "2", 0, 2),
		MakeToken(TokenType::RParen, ")", 0, 3),
		});
	size_t pos = 0;

	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(ExpressionParserTest, Parse_CallExpr_UnclosedArgs_Throws) {
	// "func(1" -- missing ')'
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "func", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::Number, "1", 0, 2),
		});
	size_t pos = 0;

	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(ExpressionParserTest, Parse_IndexExpr_NonIdentifierArray_Throws) {
	// "3[0]" -- only identifiers can be indexed
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Number, "3", 0, 0),
		MakeToken(TokenType::LBracket, "[", 0, 1),
		MakeToken(TokenType::Number, "0", 0, 2),
		MakeToken(TokenType::RBracket, "]", 0, 3),
		});
	size_t pos = 0;

	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(ExpressionParserTest, Parse_IndexExpr_NumberIndex) {
	// "arr[0]"
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "arr", 0, 0),
		MakeToken(TokenType::LBracket, "[", 0, 1),
		MakeToken(TokenType::Number, "0", 0, 2),
		MakeToken(TokenType::RBracket, "]", 0, 3),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::IndexExpr));
	ASSERT_THAT(node->children.size(), Eq(2u));
	EXPECT_THAT(node->children[0]->type, Eq(NodeType::Identifier));
	EXPECT_THAT(node->children[0]->token.lexeme, Eq("arr"));
	EXPECT_THAT(node->children[1]->type, Eq(NodeType::NumberLiteral));
	EXPECT_THAT(pos, Eq(4u));
}

TEST_F(ExpressionParserTest, Parse_IndexExpr_ExpressionIndex) {
	// "arr[i - 1]"
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "arr", 0, 0),
		MakeToken(TokenType::LBracket, "[", 0, 1),
		MakeToken(TokenType::Identifier, "i", 0, 2),
		MakeToken(TokenType::Minus, "-", 0, 3),
		MakeToken(TokenType::Number, "1", 0, 4),
		MakeToken(TokenType::RBracket, "]", 0, 5),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::IndexExpr));

	SyntaxNode* indexNode = node->children[1].get();
	ASSERT_THAT(indexNode->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(indexNode->token.type, Eq(TokenType::Minus));
}

TEST_F(ExpressionParserTest, Parse_IndexExpr_Unclosed_Throws) {
	// "arr[0" -- missing ']'
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "arr", 0, 0),
		MakeToken(TokenType::LBracket, "[", 0, 1),
		MakeToken(TokenType::Number, "0", 0, 2),
		});
	size_t pos = 0;

	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(ExpressionParserTest, Parse_AssignToIndexExpr) {
	// "arr[0] = 10"
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "arr", 0, 0),
		MakeToken(TokenType::LBracket, "[", 0, 1),
		MakeToken(TokenType::Number, "0", 0, 2),
		MakeToken(TokenType::RBracket, "]", 0, 3),
		MakeToken(TokenType::Assign, "=", 0, 4),
		MakeToken(TokenType::Number, "10", 0, 5),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::AssignExpr));
	EXPECT_THAT(node->children[0]->type, Eq(NodeType::IndexExpr));
	EXPECT_THAT(node->children[1]->type, Eq(NodeType::NumberLiteral));
}

TEST_F(ExpressionParserTest, Parse_VarDeclareInitializer_ArrExpr) {
	// "Array(3)" used as a var-declare initializer -- exercised here at the
	// expression level since VarDeclareParser just delegates to whatever
	// parser is registered for the initializer's leading token.
	TokenList tokenList = MakeTokens({
		MakeToken(TokenType::Identifier, "Array", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::Number, "3", 0, 2),
		MakeToken(TokenType::RParen, ")", 0, 3),
		});
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::ArrExpr));
	EXPECT_THAT(node->token.lexeme, Eq("Array"));
}

#endif
