#ifdef _DEBUG
#include "gmock/gmock.h"
#include "BlockParser.h"
#include "MockStatementParser.h"
#include "StatementParserRegistry.h"
#include "TestTokenHelpers.h"
#include <stdexcept>

using namespace testing;

class BlockParserTest : public Test {
public:
	TokenList MakeBlockEmptyBlock() {
		return TokenList{
			MakeToken(TokenType::LBrace, "{", 0, 0),
			MakeToken(TokenType::RBrace, "}", 0, 1)
		};
	}

	// "{ var a = 1; }"
	TokenList MakeBlockWithSingleVarDeclareTokens() {
		return TokenList{
			MakeToken(TokenType::LBrace, "{", 0, 0),
			MakeToken(TokenType::KwVar, "var", 0, 1),
			MakeToken(TokenType::Identifier, "a", 0, 2),
			MakeToken(TokenType::Assign, "=", 0, 3),
			MakeToken(TokenType::Number, "1", 0, 4),
			MakeToken(TokenType::Semicolon, ";", 0, 5),
			MakeToken(TokenType::RBrace, "}", 0, 6),
		};
	}

	// "{ { } }"
	TokenList MakeNestedEmptyBlockTokens() {
		return TokenList{
			MakeToken(TokenType::LBrace, "{", 0, 0),
			MakeToken(TokenType::LBrace, "{", 0, 1),
			MakeToken(TokenType::RBrace, "}", 0, 2),
			MakeToken(TokenType::RBrace, "}", 0, 3),
		};
	}

	// "{ var a = 1;" -- missing closing '}'
	TokenList MakeUnclosedBlockTokens() {
		return TokenList{
			MakeToken(TokenType::LBrace, "{", 0, 0),
			MakeToken(TokenType::KwVar, "var", 0, 1),
			MakeToken(TokenType::Identifier, "a", 0, 2),
			MakeToken(TokenType::Assign, "=", 0, 3),
			MakeToken(TokenType::Number, "1", 0, 4),
			MakeToken(TokenType::Semicolon, ";", 0, 5),
		};
	}

	// "{ a = 1; b = 2; }" -- two consecutive bare expression-statements
	TokenList MakeConsecutiveBareAssignmentTokens() {
		return TokenList{
			MakeToken(TokenType::LBrace, "{", 0, 0),
			MakeToken(TokenType::Identifier, "a", 0, 1),
			MakeToken(TokenType::Assign, "=", 0, 2),
			MakeToken(TokenType::Number, "1", 0, 3),
			MakeToken(TokenType::Semicolon, ";", 0, 4),
			MakeToken(TokenType::Identifier, "b", 0, 5),
			MakeToken(TokenType::Assign, "=", 0, 6),
			MakeToken(TokenType::Number, "2", 0, 7),
			MakeToken(TokenType::Semicolon, ";", 0, 8),
			MakeToken(TokenType::RBrace, "}", 0, 9),
		};
	}
protected:
	BlockParser parser;

	// Nested "var ...;" statements are handled by the real VarDeclareParser
	// (self-registered for KwVar), which itself resolves the token after
	// 'var' via StatementParserRegistry. Register a mock for Identifier so
	// this test doesn't depend on whatever another test file happens to
	// leave registered there.
	std::shared_ptr<MockStatementParser> mockIdentifierParser = std::make_shared<MockStatementParser>();

	void SetUp() override {
		// Capture mockIdentifierParser by value (not `this`): the factory
		// lambda is stored in the global StatementParserRegistry and can
		// outlive this fixture.
		StatementParserRegistry::Instance().Register(TokenType::Identifier, [tailParser = mockIdentifierParser]() {
			return tailParser;
		});
	}

	void TearDown() override {
		// Release our captured mockIdentifierParser from the global registry
		// so it doesn't outlive this fixture (which would otherwise be
		// reported as a leaked mock, or get called again -- with
		// already-satisfied expectations -- by a later test).
		StatementParserRegistry::Instance().Register(TokenType::Identifier, []() {
			return nullptr;
		});
	}
};

TEST_F(BlockParserTest, Parse_EmptyBlock) {
	// "{ }"
	TokenList tokenList = MakeBlockEmptyBlock();
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::BlockStmt));
	ASSERT_THAT(node->children, SizeIs(1));

	// last child is a marker node for the closing '}'
	const std::unique_ptr<SyntaxNode>& endNode = node->children[0];
	EXPECT_THAT(endNode->type, Eq(NodeType::BlockStmt));
	EXPECT_THAT(endNode->token.type, Eq(TokenType::RBrace));
	EXPECT_THAT(pos, Eq(2u));
}

TEST_F(BlockParserTest, Parse_SingleStatement_DelegatesToRegisteredStatementParser) {
	// "{ var a = 1; }" -- BlockParser must resolve nested statements via StatementParserRegistry
	TokenList tokenList = MakeBlockWithSingleVarDeclareTokens();

	EXPECT_CALL(*mockIdentifierParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
		auto node = std::make_unique<AssignExprNode>();
		node->token = tokens[pos + 1]; // '='
		pos += 3; // consume 'a', '=', '1'
		return node;
			});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::BlockStmt));
	ASSERT_THAT(node->children, SizeIs(2));
	EXPECT_THAT(node->children[0]->type, Eq(NodeType::VarDeclareStatement));

	// last child is a marker node for the closing '}'
	const std::unique_ptr<SyntaxNode>& endNode = node->children[1];
	EXPECT_THAT(endNode->type, Eq(NodeType::BlockStmt));
	EXPECT_THAT(endNode->token.type, Eq(TokenType::RBrace));
	EXPECT_THAT(pos, Eq(tokenList.size()));
}

TEST_F(BlockParserTest, Parse_NestedEmptyBlock) {
	// "{ { } }" -- LBrace resolves back to BlockParser itself via the registry
	TokenList tokenList = MakeNestedEmptyBlockTokens();
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::BlockStmt));
	ASSERT_THAT(node->children, SizeIs(2));

	// first child is the nested block, itself ending in its own '}' marker
	const std::unique_ptr<SyntaxNode>& innerNode = node->children[0];
	EXPECT_THAT(innerNode->type, Eq(NodeType::BlockStmt));
	ASSERT_THAT(innerNode->children, SizeIs(1));
	EXPECT_THAT(innerNode->children[0]->type, Eq(NodeType::BlockStmt));
	EXPECT_THAT(innerNode->children[0]->token.type, Eq(TokenType::RBrace));

	// second child is the outer block's own closing '}' marker
	const std::unique_ptr<SyntaxNode>& endNode = node->children[1];
	EXPECT_THAT(endNode->type, Eq(NodeType::BlockStmt));
	EXPECT_THAT(endNode->token.type, Eq(TokenType::RBrace));
	EXPECT_THAT(pos, Eq(tokenList.size()));
}

TEST_F(BlockParserTest, Parse_UnclosedBlock_Throws) {
	// "{ var a = 1;" -- reaches end of input without a closing '}'
	TokenList tokenList = MakeUnclosedBlockTokens();

	EXPECT_CALL(*mockIdentifierParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
		auto node = std::make_unique<AssignExprNode>();
		node->token = tokens[pos + 1]; // '='
		pos += 3; // consume 'a', '=', '1'
		return node;
			});

	size_t pos = 0;
	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(BlockParserTest, Parse_ConsecutiveBareExpressionStatements_ConsumesEachTrailingSemicolon) {
	// "{ a = 1; b = 2; }" -- regression test: a bare expression statement's
	// own leading-token resolution (ExpressionParser) never consumes its
	// trailing ';', so BlockParser itself must swallow it before resolving
	// the next statement. Without that, the leftover ';' would be mistaken
	// for the start of a new statement and throw "Unexpected token inside
	// block", silently dropping the rest of the block.
	TokenList tokenList = MakeConsecutiveBareAssignmentTokens();

	EXPECT_CALL(*mockIdentifierParser, Parse(_, _))
		.Times(2)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 3; // consume 'a', '=', '1' -- NOT the ';'
			return std::make_unique<AssignExprNode>();
		})
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 3; // consume 'b', '=', '2' -- NOT the ';'
			return std::make_unique<AssignExprNode>();
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	// 2 statements + the closing '}' marker
	ASSERT_THAT(node->children, SizeIs(3));
	EXPECT_THAT(pos, Eq(tokenList.size()));
}
#endif
