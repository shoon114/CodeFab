#ifdef _DEBUG
#include <utility>
#include "gmock/gmock.h"
#include "IfStatementParser.h"
#include "MockExpressionParser.h"
#include "TestTokenHelpers.h"

using namespace testing;

namespace {
	TokenList MakeTokens(std::initializer_list<std::pair<TokenType, std::string>> tokens) {
		TokenList result;
		int column = 0;
		for (const auto& [type, lexeme] : tokens) {
			result.push_back(MakeToken(type, lexeme, 1, column++));
		}
		return result;
	}
}

class IfStatementParserTest : public Test {
protected:
	NiceMock<MockExpressionParser> exprParser;
	IfStatementParser parser{ exprParser };

	// "if (a <op> 3)"
	TokenList MakeIfConditionTokens(TokenType opType, const std::string& opLexeme) {
		return MakeTokens({
			{TokenType::KwIf, "if"},
			{TokenType::LParen, "("},
			{TokenType::Identifier, "a"},
			{opType, opLexeme},
			{TokenType::Number, "3"},
			{TokenType::RParen, ")"},
			{TokenType::EndOfFile, ""},
			});
	}

	void StubExprParserToConsumeCondition() {
		EXPECT_CALL(exprParser, Parse(_, _))
			.Times(1)
			.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<SyntaxNode>();
			node->type = NodeType::BinaryExpr;
			node->token = tokens[pos];
			pos += 3;
			return node;
				});
	}

	void ExpectParseThrows(TokenList tokenList, const char* expectedMessage) {
		size_t pos = 0;
		try {
			parser.Parse(tokenList, pos);
			FAIL();
		}
		catch (const std::runtime_error& e) {
			EXPECT_STREQ(e.what(), expectedMessage);
		}
	}
};

TEST_F(IfStatementParserTest, Parse_Condition_AttachesExpressionParserResultAsCondition) {
	TokenList tokenList = MakeIfConditionTokens(TokenType::Gt, ">");
	StubExprParserToConsumeCondition();

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::IfStmt));
	EXPECT_THAT(root->token.lexeme, Eq("if"));
	ASSERT_THAT(root->children, SizeIs(1));

	const std::unique_ptr<SyntaxNode>& conditionNode = root->children[0];
	EXPECT_THAT(conditionNode->type, Eq(NodeType::BinaryExpr));
}

TEST_F(IfStatementParserTest, Parse_Condition_WithGtEqOperator_AttachesExpressionParserResultAsCondition) {
	TokenList tokenList = MakeIfConditionTokens(TokenType::GtEq, ">=");
	StubExprParserToConsumeCondition();

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::IfStmt));
	EXPECT_THAT(root->token.lexeme, Eq("if"));
	ASSERT_THAT(root->children, SizeIs(1));

	const std::unique_ptr<SyntaxNode>& conditionNode = root->children[0];
	EXPECT_THAT(conditionNode->type, Eq(NodeType::BinaryExpr));
}

TEST_F(IfStatementParserTest, Parse_MissingOpenParen_ThrowsOnMalformedSyntax) {
	// if a>3)   -- '(' 누락
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::Identifier, "a"},
		{TokenType::Gt, ">"},
		{TokenType::Number, "3"},
		{TokenType::RParen, ")"},
		{TokenType::EndOfFile, ""},
		});

	ExpectParseThrows(tokenList, "Invalid Syntax. '(' is Missing");
}

TEST_F(IfStatementParserTest, Parse_MissingCloseParen_ThrowsOnMalformedSyntax) {
	// if(a>3   -- ')' 누락
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "a"},
		{TokenType::Gt, ">"},
		{TokenType::Number, "3"},
		{TokenType::EndOfFile, ""},
		});

	ExpectParseThrows(tokenList, "Invalid Syntax. ')' is Missing");
}
#endif
