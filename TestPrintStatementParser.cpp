#ifdef _DEBUG
#include <stdexcept>
#include "gmock/gmock.h"
#include "PrintStatementParser.h"
#include "MockExpressionParser.h"
#include "TestTokenHelpers.h"

using namespace testing;

class PrintStatementParserTest : public Test {
protected:
	NiceMock<MockExpressionParser> exprParser;
	PrintStatementParser parser{ exprParser };

	void StubExprParserToConsume(int tokenCount, NodeType resultType) {
		EXPECT_CALL(exprParser, Parse(_, _))
			.Times(1)
			.WillOnce([tokenCount, resultType](const TokenList& tokens, size_t& pos) {
				auto node = std::make_unique<SyntaxNode>();
				node->type = resultType;
				node->token = tokens[pos];
				pos += tokenCount;
				return node;
			});
	}

	void StubExprParserToThrow(const std::string& message) {
		EXPECT_CALL(exprParser, Parse(_, _))
			.Times(1)
			.WillOnce([message](const TokenList&, size_t&) -> std::unique_ptr<SyntaxNode> {
				throw std::runtime_error(message);
			});
	}

	std::unique_ptr<SyntaxNode> ParseTokens(const TokenList& tokenList) {
		size_t pos = 0;
		return parser.Parse(tokenList, pos);
	}

	void ExpectParseThrows(const TokenList& tokenList, const char* expectedMessage) {
		size_t pos = 0;
		try {
			parser.Parse(tokenList, pos);
			FAIL();
		}
		catch (const std::runtime_error& e) {
			EXPECT_STREQ(e.what(), expectedMessage);
		}
	}

	void AssertPrintStmtWithExpression(const std::unique_ptr<SyntaxNode>& root, NodeType exprType) {
		ASSERT_THAT(root, NotNull());
		EXPECT_THAT(root->type, Eq(NodeType::PrintStmt));
		EXPECT_THAT(root->token.lexeme, Eq("print"));
		ASSERT_THAT(root->children, SizeIs(1));

		const std::unique_ptr<SyntaxNode>& exprNode = root->children[0];
		EXPECT_THAT(exprNode->type, Eq(exprType));
	}
};

TEST_F(PrintStatementParserTest, Parse_PrintNumberLiteral_AttachesExpressionAsChild) {
	// "print 3;"
	TokenList tokenList = {
		MakeToken(TokenType::Print, "print", 1, 0),
		MakeToken(TokenType::Number, "3", 1, 1),
		MakeToken(TokenType::Semicolon, ";", 1, 2),
		MakeToken(TokenType::EndOfFile, "", 1, 3),
	};
	StubExprParserToConsume(1, NodeType::NumberLiteral); // "3" 1개 토큰 소비

	std::unique_ptr<SyntaxNode> root = ParseTokens(tokenList);

	AssertPrintStmtWithExpression(root, NodeType::NumberLiteral);
}

TEST_F(PrintStatementParserTest, Parse_PrintBinaryExpression_AttachesExpressionAsChild) {
	// "print 2+3;"
	TokenList tokenList = {
		MakeToken(TokenType::Print, "print", 1, 0),
		MakeToken(TokenType::Number, "2", 1, 1),
		MakeToken(TokenType::Plus, "+", 1, 2),
		MakeToken(TokenType::Number, "3", 1, 3),
		MakeToken(TokenType::Semicolon, ";", 1, 4),
		MakeToken(TokenType::EndOfFile, "", 1, 5),
	};
	StubExprParserToConsume(3, NodeType::BinaryExpr); // "2+3" 3개 토큰 소비

	std::unique_ptr<SyntaxNode> root = ParseTokens(tokenList);

	AssertPrintStmtWithExpression(root, NodeType::BinaryExpr);
}

TEST_F(PrintStatementParserTest, Parse_PrintDanglingOperator_ThrowsOnMalformedSyntax) {
	// "print a+;"   -- '+' 뒤 피연산자 누락
	TokenList tokenList = {
		MakeToken(TokenType::Print, "print", 1, 0),
		MakeToken(TokenType::Identifier, "a", 1, 1),
		MakeToken(TokenType::Plus, "+", 1, 2),
		MakeToken(TokenType::Semicolon, ";", 1, 3),
		MakeToken(TokenType::EndOfFile, "", 1, 4),
	};
	StubExprParserToThrow("Unexpected token while parsing expression");

	ExpectParseThrows(tokenList, "Unexpected token while parsing expression");
}
#endif
