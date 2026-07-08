#ifdef _DEBUG
#include <utility>
#include "gmock/gmock.h"
#include "IfStatementParser.h"
#include "MockStatementParser.h"
#include "StatementParserRegistry.h"
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
	IfStatementParser parser;

	// IfStatementParser resolves whatever is registered for the token right
	// after '(' (Identifier here) via StatementParserRegistry -- in
	// production that's the real ExpressionParser. We stand in for it with
	// a mock so this TC doesn't depend on ExpressionParser's real behavior.
	std::shared_ptr<MockStatementParser> mockConditionParser = std::make_shared<MockStatementParser>();

	void SetUp() override {
		// Capture mockConditionParser by value (not `this`): the factory
		// lambda is stored in the global StatementParserRegistry and can
		// outlive this fixture.
		StatementParserRegistry::Instance().Register(TokenType::Identifier, [conditionParser = mockConditionParser]() {
			return conditionParser;
		});
	}

	void TearDown() override {
		// Release our captured mockConditionParser from the global registry
		// so it doesn't outlive this fixture (which would otherwise be
		// reported as a leaked mock, or get called again -- with
		// already-satisfied expectations -- by a later test).
		StatementParserRegistry::Instance().Register(TokenType::Identifier, []() {
			return nullptr;
		});
	}

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

	void StubConditionParserToConsumeCondition() {
		EXPECT_CALL(*mockConditionParser, Parse(_, _))
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
	StubConditionParserToConsumeCondition();

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
	StubConditionParserToConsumeCondition();

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

	EXPECT_CALL(*mockConditionParser, Parse(_, _))
		.Times(1)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<SyntaxNode>();
			node->type = NodeType::BinaryExpr;
			node->token = tokens[pos];
			pos += 3; // consume 'a', '>', '3'
			return node;
		});

	ExpectParseThrows(tokenList, "Invalid Syntax. ')' is Missing");
}
#endif
