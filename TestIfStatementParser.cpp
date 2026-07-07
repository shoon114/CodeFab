#ifdef _DEBUG
#include "gmock/gmock.h"
#include "IfStatementParser.h"
#include "MockExpressionParser.h"
#include "TestTokenHelpers.h"

using namespace testing;

class IfStatementParserTest : public Test {
protected:
	NiceMock<MockExpressionParser> exprParser;
	IfStatementParser parser{ exprParser };

	// "if (a <op> 3)"
	TokenList MakeIfConditionTokens(TokenType opType, const std::string& opLexeme) {
		return TokenList{
			MakeToken(TokenType::KwIf, "if", 1, 0),
			MakeToken(TokenType::LParen, "(", 1, 1),
			MakeToken(TokenType::Identifier, "a", 1, 2),
			MakeToken(opType, opLexeme, 1, 3),
			MakeToken(TokenType::Number, "3", 1, 4),
			MakeToken(TokenType::RParen, ")", 1, 5),
			MakeToken(TokenType::EndOfFile, "", 1, 6),
		};
	}

	// exprParser가 조건식(3개 토큰: identifier, operator, number)을 소비하고
	// BinaryExpr 노드 하나를 돌려주는 상황을 흉내낸다.
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
	TokenList tokenList = {
		MakeToken(TokenType::KwIf, "if", 1, 0),
		MakeToken(TokenType::Identifier, "a", 1, 1),
		MakeToken(TokenType::Gt, ">", 1, 2),
		MakeToken(TokenType::Number, "3", 1, 3),
		MakeToken(TokenType::RParen, ")", 1, 4),
		MakeToken(TokenType::EndOfFile, "", 1, 5),
	};

	ExpectParseThrows(tokenList, "Invalid Syntax. '(' is Missing");
}

TEST_F(IfStatementParserTest, Parse_MissingCloseParen_ThrowsOnMalformedSyntax) {
	// if(a>3   -- ')' 누락
	TokenList tokenList = {
		MakeToken(TokenType::KwIf, "if", 1, 0),
		MakeToken(TokenType::LParen, "(", 1, 1),
		MakeToken(TokenType::Identifier, "a", 1, 2),
		MakeToken(TokenType::Gt, ">", 1, 3),
		MakeToken(TokenType::Number, "3", 1, 4),
		MakeToken(TokenType::EndOfFile, "", 1, 5),
	};

	ExpectParseThrows(tokenList, "Invalid Syntax. ')' is Missing");
}
#endif
