#ifdef _DEBUG
#include <utility>
#include "gmock/gmock.h"
#include "FunctionStatementParser.h"
#include "MockStatementParser.h"
#include "StatementParserRegistry.h"
#include "TestTokenHelpers.h"

using namespace testing;

class FunctionStatementParserTest : public Test {
protected:
	FunctionStatementParser parser;

	// FunctionStatementParser resolves whatever is registered for the token
	// at the function body's position ('{' here) via StatementParserRegistry
	// -- in production that's the real BlockParser. We stand in for it with
	// a mock so this TC doesn't depend on BlockParser's real behavior.
	std::shared_ptr<MockStatementParser> mockBodyParser = std::make_shared<MockStatementParser>();

	void SetUp() override {
		// Capture mockBodyParser by value (not `this`): the factory lambda
		// is stored in the global StatementParserRegistry and can outlive
		// this fixture.
		StatementParserRegistry::Instance().Register(TokenType::LBrace, [bodyParser = mockBodyParser]() {
			return bodyParser;
		});
	}

	void TearDown() override {
		// Release our captured mockBodyParser from the global registry so
		// it doesn't outlive this fixture (which would otherwise be
		// reported as a leaked mock, or get called again -- with
		// already-satisfied expectations -- by a later test).
		StatementParserRegistry::Instance().Register(TokenType::LBrace, []() {
			return nullptr;
		});
	}
};

// func add(a, b) { ... }
TEST_F(FunctionStatementParserTest, Parse_WithParams_AttachesParamsThenBody) {
	TokenList tokenList = {
		MakeToken(TokenType::KwFunc, "func", 1, 0),
		MakeToken(TokenType::Identifier, "add", 1, 1),
		MakeToken(TokenType::LParen, "(", 1, 2),
		MakeToken(TokenType::Identifier, "a", 1, 3),
		MakeToken(TokenType::Comma, ",", 1, 4),
		MakeToken(TokenType::Identifier, "b", 1, 5),
		MakeToken(TokenType::RParen, ")", 1, 6),
		MakeToken(TokenType::LBrace, "{", 1, 7),
		MakeToken(TokenType::RBrace, "}", 1, 8),
		MakeToken(TokenType::EndOfFile, "", 1, 9),
	};

	EXPECT_CALL(*mockBodyParser, Parse(_, _))
		.Times(1)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<BlockStmtNode>();
			node->token = tokens[pos];
			pos += 2; // consume '{' and '}'
			return node;
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::FuncDeclStmt));
	EXPECT_THAT(root->token.lexeme, Eq("add"));
	ASSERT_THAT(root->children, SizeIs(3));

	EXPECT_THAT(root->children[0]->type, Eq(NodeType::Identifier));
	EXPECT_THAT(root->children[0]->token.lexeme, Eq("a"));
	EXPECT_THAT(root->children[1]->type, Eq(NodeType::Identifier));
	EXPECT_THAT(root->children[1]->token.lexeme, Eq("b"));
	EXPECT_THAT(root->children[2]->type, Eq(NodeType::BlockStmt));
}
#endif
