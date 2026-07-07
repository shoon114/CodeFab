#ifdef _DEBUG
#include "gmock/gmock.h"
#include "ForStmtParser.h"
#include "MockStatementParser.h"
#include "StatementParserRegistry.h"
#include "TestTokenHelpers.h"

using namespace testing;

class ForStmtParserTest : public Test {
protected:
	ForStmtParser parser;

	// ForStmtParser resolves the init/condition/update clauses via
	// StatementParserRegistry -- in production init is VarDeclareParser
	// (KwVar) and condition/update are ExpressionParser (Identifier). We
	// stand in for both with mocks so this TC doesn't depend on their real
	// behavior.
	std::shared_ptr<MockStatementParser> mockInitParser = std::make_shared<MockStatementParser>();
	std::shared_ptr<MockStatementParser> mockIdentifierParser = std::make_shared<MockStatementParser>();

	void SetUp() override {
		// Capture mocks by value (not `this`): the factory lambdas are
		// stored in the global StatementParserRegistry and can outlive this
		// fixture.
		StatementParserRegistry::Instance().Register(TokenType::KwVar, [initParser = mockInitParser]() {
			return initParser;
		});
		StatementParserRegistry::Instance().Register(TokenType::Identifier, [identifierParser = mockIdentifierParser]() {
			return identifierParser;
		});
	}

	void TearDown() override {
		// Release our captured mocks from the global registry so they don't
		// outlive this fixture (which would otherwise be reported as leaked
		// mocks, or get called again -- with already-satisfied expectations
		// -- by a later test).
		StatementParserRegistry::Instance().Register(TokenType::KwVar, []() { return nullptr; });
		StatementParserRegistry::Instance().Register(TokenType::Identifier, []() { return nullptr; });
	}

	// "for(var i = 0; i < 3; i = i + 1) {}"
	TokenList MakeForLoopWithEmptyBodyTokens() {
		return TokenList{
			MakeToken(TokenType::KwFor, "for", 0, 0),
			MakeToken(TokenType::LParen, "(", 0, 1),
			MakeToken(TokenType::KwVar, "var", 0, 2),
			MakeToken(TokenType::Identifier, "i", 0, 3),
			MakeToken(TokenType::Assign, "=", 0, 4),
			MakeToken(TokenType::Number, "0", 0, 5),
			MakeToken(TokenType::Semicolon, ";", 0, 6),
			MakeToken(TokenType::Identifier, "i", 0, 7),
			MakeToken(TokenType::Lt, "<", 0, 8),
			MakeToken(TokenType::Number, "3", 0, 9),
			MakeToken(TokenType::Semicolon, ";", 0, 10),
			MakeToken(TokenType::Identifier, "i", 0, 11),
			MakeToken(TokenType::Assign, "=", 0, 12),
			MakeToken(TokenType::Identifier, "i", 0, 13),
			MakeToken(TokenType::Plus, "+", 0, 14),
			MakeToken(TokenType::Number, "1", 0, 15),
			MakeToken(TokenType::RParen, ")", 0, 16),
			MakeToken(TokenType::LBrace, "{", 0, 17),
			MakeToken(TokenType::RBrace, "}", 0, 18),
			MakeToken(TokenType::EndOfFile, "", 0, 19),
		};
	}
};

TEST_F(ForStmtParserTest, Parse_WithEmptyBody_BuildsForStmtTree) {
	TokenList tokenList = MakeForLoopWithEmptyBodyTokens();

	EXPECT_CALL(*mockInitParser, Parse(_, _))
		.Times(1)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			// "var i = 0;" -- consumes through its own trailing ';'
			auto node = std::make_unique<VarDeclareStatementNode>();
			node->token = tokens[pos];
			pos += 5; // 'var', 'i', '=', '0', ';'
			return node;
		});

	EXPECT_CALL(*mockIdentifierParser, Parse(_, _))
		.Times(2)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			// cond: "i < 3"
			auto node = std::make_unique<BinaryExprNode>();
			node->token = tokens[pos + 1]; // '<'
			pos += 3;
			return node;
		})
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			// update: "i = i + 1"
			auto node = std::make_unique<AssignExprNode>();
			node->token = tokens[pos + 1]; // '='
			pos += 5;
			return node;
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::ForStmt));
	EXPECT_THAT(root->token.lexeme, Eq("for"));
	ASSERT_THAT(root->children, SizeIs(4));

	const std::unique_ptr<SyntaxNode>& initNode = root->children[0];
	EXPECT_THAT(initNode->type, Eq(NodeType::VarDeclareStatement));

	const std::unique_ptr<SyntaxNode>& condNode = root->children[1];
	EXPECT_THAT(condNode->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(condNode->token.lexeme, Eq("<"));

	const std::unique_ptr<SyntaxNode>& updateNode = root->children[2];
	EXPECT_THAT(updateNode->type, Eq(NodeType::AssignExpr));
	EXPECT_THAT(updateNode->token.lexeme, Eq("="));

	const std::unique_ptr<SyntaxNode>& bodyNode = root->children[3];
	EXPECT_THAT(bodyNode->type, Eq(NodeType::BlockStmt));
	EXPECT_THAT(bodyNode->children, IsEmpty());
}
#endif
