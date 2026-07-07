#ifdef _DEBUG
#include "gmock/gmock.h"
#include "IfStatementParser.h"
#include "MockExpressionParser.h"
#include "TestTokenHelpers.h"

using namespace testing;

class IfStatementParserTest : public Test {
protected:
	MockExpressionParser exprParser;
	IfStatementParser parser{ exprParser };

	// "if (a > 3)"
	TokenList MakeIfConditionTokens() {
		return TokenList{
			MakeToken(TokenType::KwIf, "if", 1, 0),
			MakeToken(TokenType::LParen, "(", 1, 1),
			MakeToken(TokenType::Identifier, "a", 1, 2),
			MakeToken(TokenType::Gt, ">", 1, 3),
			MakeToken(TokenType::Number, "3", 1, 4),
			MakeToken(TokenType::RParen, ")", 1, 5),
			MakeToken(TokenType::EndOfFile, "", 1, 6),
		};
	}
};

TEST_F(IfStatementParserTest, Parse_Condition_AttachesExpressionParserResultAsCondition) {
	TokenList tokenList = MakeIfConditionTokens();

	EXPECT_CALL(exprParser, Parse(_, _))
		.Times(1)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<SyntaxNode>();
			node->type = NodeType::BinaryExpr;
			node->token = tokens[pos];
			pos += 3; // consume "a > 3"
			return node;
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::IfStmt));
	EXPECT_THAT(root->token.lexeme, Eq("if"));
	ASSERT_THAT(root->children, SizeIs(1));

	const std::unique_ptr<SyntaxNode>& conditionNode = root->children[0];
	EXPECT_THAT(conditionNode->type, Eq(NodeType::BinaryExpr));
}
#endif
