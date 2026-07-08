#ifdef _DEBUG
#include <stdexcept>
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

	const std::string RETURN = "return";

	// "a + b" 3개 토큰을 소비하는 BinaryExprNode를 돌려주도록 mockExprParser를 stub한다.
	void StubExprParserToConsumeBinaryExpr() {
		EXPECT_CALL(*mockExprParser, Parse(_, _))
			.Times(1)
			.WillOnce([](const TokenList& tokens, size_t& pos) {
				auto node = std::make_unique<BinaryExprNode>();
				node->token = tokens[pos];
				pos += 3;
				return node;
			});
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
};

// return a + b;
TEST_F(ReturnStatementParserTest, Parse_ReturnBinaryExpression_AttachesExpressionAsChild) {
	TokenList tokenList = {
		MakeToken(TokenType::KwReturn, RETURN, 1, 0),
		MakeToken(TokenType::Identifier, "a", 1, 1),
		MakeToken(TokenType::Plus, "+", 1, 2),
		MakeToken(TokenType::Identifier, "b", 1, 3),
		MakeToken(TokenType::Semicolon, ";", 1, 4),
		MakeToken(TokenType::EndOfFile, "", 1, 5),
	};
	StubExprParserToConsumeBinaryExpr();

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::ReturnStmt));
	EXPECT_THAT(root->token.lexeme, Eq(RETURN));
	ASSERT_THAT(root->children, SizeIs(1));
	EXPECT_THAT(root->children[0]->type, Eq(NodeType::BinaryExpr));
}

// return;  -- 값 없는 bare return
TEST_F(ReturnStatementParserTest, Parse_BareReturn_HasNoExpressionChild) {
	TokenList tokenList = {
		MakeToken(TokenType::KwReturn, RETURN, 1, 0),
		MakeToken(TokenType::Semicolon, ";", 1, 1),
		MakeToken(TokenType::EndOfFile, "", 1, 2),
	};

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::ReturnStmt));
	EXPECT_THAT(root->token.lexeme, Eq(RETURN));
	EXPECT_THAT(root->children, SizeIs(0));
}

// return a + b  -- 표현식 뒤 ';' 누락
TEST_F(ReturnStatementParserTest, Parse_MissingSemicolonAfterExpression_ThrowsOnMalformedSyntax) {
	TokenList tokenList = {
		MakeToken(TokenType::KwReturn, RETURN, 1, 0),
		MakeToken(TokenType::Identifier, "a", 1, 1),
		MakeToken(TokenType::Plus, "+", 1, 2),
		MakeToken(TokenType::Identifier, "b", 1, 3),
		MakeToken(TokenType::EndOfFile, "", 1, 4),
	};
	StubExprParserToConsumeBinaryExpr();

	ExpectParseThrows(tokenList, "Expected ';' after return statement at line 1");
}

// return  -- 'return' 뒤에 표현식도 ';'도 없이 바로 끝남
TEST_F(ReturnStatementParserTest, Parse_NothingAfterReturn_ThrowsOnMalformedSyntax) {
	TokenList tokenList = {
		MakeToken(TokenType::KwReturn, RETURN, 1, 0),
		MakeToken(TokenType::EndOfFile, "", 1, 1),
	};

	ExpectParseThrows(tokenList, "Expected ';' after return statement at line 1");
}
#endif
