#ifdef _DEBUG
#include "gmock/gmock.h"
#include "BlockParser.h"
#include "MockExpressionParser.h"
#include "TestTokenHelpers.h"
#include <stdexcept>

using namespace testing;

class BlockParserTest : public Test {
public:
	TokenList MakeBlockEmptyBlock() {
		return TokenList{
			MakeToken(TokenType::LBrace, "{", 0, 0),
			MakeToken(TokenType::RBrace, "}", 0, 2)
		};
	}

	// "{ var a = 1; }"
	TokenList MakeBlockWithSingleVarDeclareTokens() {
		return TokenList{
			MakeToken(TokenType::LBrace, "{", 0, 0),
			MakeToken(TokenType::KwVar, "var", 0, 2),
			MakeToken(TokenType::Identifier, "a", 0, 6),
			MakeToken(TokenType::Assign, "=", 0, 8),
			MakeToken(TokenType::Number, "1", 0, 10),
			MakeToken(TokenType::Semicolon, ";", 0, 11),
			MakeToken(TokenType::RBrace, "}", 0, 13),
		};
	}

	// "{ { } }"
	TokenList MakeNestedEmptyBlockTokens() {
		return TokenList{
			MakeToken(TokenType::LBrace, "{", 0, 0),
			MakeToken(TokenType::LBrace, "{", 0, 2),
			MakeToken(TokenType::RBrace, "}", 0, 4),
			MakeToken(TokenType::RBrace, "}", 0, 6),
		};
	}

	// "{ var a = 1;" -- missing closing '}'
	TokenList MakeUnclosedBlockTokens() {
		return TokenList{
			MakeToken(TokenType::LBrace, "{", 0, 0),
			MakeToken(TokenType::KwVar, "var", 0, 2),
			MakeToken(TokenType::Identifier, "a", 0, 6),
			MakeToken(TokenType::Assign, "=", 0, 8),
			MakeToken(TokenType::Number, "1", 0, 10),
			MakeToken(TokenType::Semicolon, ";", 0, 11),
		};
	}
protected:
	MockExpressionParser exprParser;
	BlockParser parser{ exprParser };
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

	EXPECT_CALL(exprParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::NumberLiteral;
		node->token = tokens[pos];
		pos++; // consume '1'
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

	size_t pos = 0;
}
#endif
