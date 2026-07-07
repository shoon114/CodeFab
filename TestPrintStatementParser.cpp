#ifdef _DEBUG
#include "gmock/gmock.h"
#include "PrintStatementParser.h"
#include "MockExpressionParser.h"
#include "TestTokenHelpers.h"

using namespace testing;

TEST(PrintStatementParserTest, Parse_PrintNumberLiteral_AttachesExpressionAsChild) {
	// "print 3;"
	TokenList tokenList = {
		MakeToken(TokenType::Print, "print", 1, 0),
		MakeToken(TokenType::Number, "3", 1, 1),
		MakeToken(TokenType::Semicolon, ";", 1, 2),
		MakeToken(TokenType::EndOfFile, "", 1, 3),
	};

	NiceMock<MockExpressionParser> exprParser;
	EXPECT_CALL(exprParser, Parse(_, _))
		.Times(1)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<SyntaxNode>();
			node->type = NodeType::NumberLiteral;
			node->token = tokens[pos];
			pos += 1; // "3" 1개 토큰 소비
			return node;
		});

	PrintStatementParser parser{ exprParser };
	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::PrintStmt));
	EXPECT_THAT(root->token.lexeme, Eq("print"));
	ASSERT_THAT(root->children, SizeIs(1));

	const std::unique_ptr<SyntaxNode>& exprNode = root->children[0];
	EXPECT_THAT(exprNode->type, Eq(NodeType::NumberLiteral));
}

TEST(PrintStatementParserTest, Parse_PrintBinaryExpression_AttachesExpressionAsChild) {
	// "print 2+3;"
	TokenList tokenList = {
		MakeToken(TokenType::Print, "print", 1, 0),
		MakeToken(TokenType::Number, "2", 1, 1),
		MakeToken(TokenType::Plus, "+", 1, 2),
		MakeToken(TokenType::Number, "3", 1, 3),
		MakeToken(TokenType::Semicolon, ";", 1, 4),
		MakeToken(TokenType::EndOfFile, "", 1, 5),
	};

	NiceMock<MockExpressionParser> exprParser;
	EXPECT_CALL(exprParser, Parse(_, _))
		.Times(1)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<SyntaxNode>();
			node->type = NodeType::BinaryExpr;
			node->token = tokens[pos];
			pos += 3; // "2+3" 3개 토큰 소비
			return node;
		});

	PrintStatementParser parser{ exprParser };
	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::PrintStmt));
	EXPECT_THAT(root->token.lexeme, Eq("print"));
	ASSERT_THAT(root->children, SizeIs(1));

	const std::unique_ptr<SyntaxNode>& exprNode = root->children[0];
	EXPECT_THAT(exprNode->type, Eq(NodeType::BinaryExpr));
}
#endif
