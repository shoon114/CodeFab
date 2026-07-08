#ifdef _DEBUG
#include "gmock/gmock.h"
#include "ReturnStatementParser.h"
#include "MockStatementParser.h"
#include "StatementParserRegistry.h"
#include "TestTokenHelpers.h"

using namespace testing;

class ReturnStatementParserTest : public Test {
protected:
	ReturnStatementParser parser;

	// ReturnStatementParser resolves whatever is registered for the token
	// right after 'return' via StatementParserRegistry -- in production
	// that's the real ExpressionParser. We stand in for it with a mock so
	// this TC doesn't depend on ExpressionParser's real behavior.
	std::shared_ptr<MockStatementParser> mockExprParser = std::make_shared<MockStatementParser>();

	void SetUp() override {
		// Capture mockExprParser by value (not `this`): the factory lambda
		// is stored in the global StatementParserRegistry and can outlive
		// this fixture.
		StatementParserRegistry::Instance().Register(TokenType::Identifier, [exprParser = mockExprParser]() {
			return exprParser;
		});
	}

	void TearDown() override {
		// Release our captured mockExprParser from the global registry so
		// it doesn't outlive this fixture (which would otherwise be
		// reported as a leaked mock, or get called again -- with
		// already-satisfied expectations -- by a later test).
		StatementParserRegistry::Instance().Register(TokenType::Identifier, []() {
			return nullptr;
		});
	}
};

// return a + b;
TEST_F(ReturnStatementParserTest, Parse_ReturnBinaryExpression_AttachesExpressionAsChild) {
	TokenList tokenList = {
		MakeToken(TokenType::KwReturn, "return", 1, 0),
		MakeToken(TokenType::Identifier, "a", 1, 1),
		MakeToken(TokenType::Plus, "+", 1, 2),
		MakeToken(TokenType::Identifier, "b", 1, 3),
		MakeToken(TokenType::Semicolon, ";", 1, 4),
		MakeToken(TokenType::EndOfFile, "", 1, 5),
	};

	EXPECT_CALL(*mockExprParser, Parse(_, _))
		.Times(1)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<BinaryExprNode>();
			node->token = tokens[pos];
			pos += 3; // consume 'a', '+', 'b'
			return node;
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::ReturnStmt));
	EXPECT_THAT(root->token.lexeme, Eq("return"));
	ASSERT_THAT(root->children, SizeIs(1));
	EXPECT_THAT(root->children[0]->type, Eq(NodeType::BinaryExpr));
}
#endif
