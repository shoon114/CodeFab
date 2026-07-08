#ifdef _DEBUG
#include <stdexcept>
#include <utility>
#include "gmock/gmock.h"
#include "BlockParser.h"
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
		//
		// LBrace's only production owner is the real BlockParser,
		// self-registered once at static-init time. Resetting it to a
		// nullptr-returning factory here would permanently destroy that
		// registration for the rest of the test binary's lifetime -- any
		// later test (depending on link order) that relies on LBrace
		// resolving to the real BlockParser would then fail. Restore the
		// real factory instead of nulling it out.
		StatementParserRegistry::Instance().Register(TokenType::LBrace, []() {
			return std::make_shared<BlockParser>();
		});
	}
	const std::string FUNC = "func";
	const std::string FUNC_NAME = "add";
};

// func add(a, b) {
// return a + b;
// }
TEST_F(FunctionStatementParserTest, Parse_WithParams_AttachesParamsThenBody) {
	TokenList tokenList = {
		MakeToken(TokenType::KwFunc, FUNC, 1, 0),
		MakeToken(TokenType::Identifier, FUNC_NAME, 1, 1),
		MakeToken(TokenType::LParen, "(", 1, 2),
		MakeToken(TokenType::Identifier, "a", 1, 3),
		MakeToken(TokenType::Comma, ",", 1, 4),
		MakeToken(TokenType::Identifier, "b", 1, 5),
		MakeToken(TokenType::RParen, ")", 1, 6),
		MakeToken(TokenType::LBrace, "{", 1, 7),
		MakeToken(TokenType::KwReturn, "return", 2, 0),
		MakeToken(TokenType::Identifier, "a", 2, 1),
		MakeToken(TokenType::Plus, "+", 2, 2),
		MakeToken(TokenType::Identifier, "b", 2, 3),
		MakeToken(TokenType::Semicolon, ";", 2, 4),
		MakeToken(TokenType::RBrace, "}", 3, 0),
		MakeToken(TokenType::EndOfFile, "", 3, 1),
	};

	EXPECT_CALL(*mockBodyParser, Parse(_, _))
		.Times(1)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<BlockStmtNode>();
			node->token = tokens[pos];
			pos += 7; // '{' return a + b ; '}' 7개 토큰 소비
			return node;
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::FuncDeclStmt));
	EXPECT_THAT(root->token.lexeme, Eq(FUNC_NAME));
	ASSERT_THAT(root->children, SizeIs(3));

	EXPECT_THAT(root->children[0]->type, Eq(NodeType::Identifier));
	EXPECT_THAT(root->children[0]->token.lexeme, Eq("a"));
	EXPECT_THAT(root->children[1]->type, Eq(NodeType::Identifier));
	EXPECT_THAT(root->children[1]->token.lexeme, Eq("b"));
	EXPECT_THAT(root->children[2]->type, Eq(NodeType::BlockStmt));
}

// func add() {
// return 1;
// }
TEST_F(FunctionStatementParserTest, Parse_NoParams_AttachesOnlyBodyAsChild) {
	TokenList tokenList = {
		MakeToken(TokenType::KwFunc, FUNC, 1, 0),
		MakeToken(TokenType::Identifier, FUNC_NAME, 1, 1),
		MakeToken(TokenType::LParen, "(", 1, 2),
		MakeToken(TokenType::RParen, ")", 1, 3),
		MakeToken(TokenType::LBrace, "{", 1, 4),
		MakeToken(TokenType::KwReturn, "return", 2, 0),
		MakeToken(TokenType::Number, "1", 2, 1),
		MakeToken(TokenType::Semicolon, ";", 2, 2),
		MakeToken(TokenType::RBrace, "}", 3, 0),
		MakeToken(TokenType::EndOfFile, "", 3, 1),
	};

	EXPECT_CALL(*mockBodyParser, Parse(_, _))
		.Times(1)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<BlockStmtNode>();
			node->token = tokens[pos];
			pos += 5; // '{' return 1 ; '}' 5개 토큰 소비
			return node;
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::FuncDeclStmt));
	EXPECT_THAT(root->token.lexeme, Eq(FUNC_NAME));
	ASSERT_THAT(root->children, SizeIs(1));
	EXPECT_THAT(root->children[0]->type, Eq(NodeType::BlockStmt));
}

// func (a, b) {
// return a + b;
// }  -- 함수 이름 누락
TEST_F(FunctionStatementParserTest, Parse_MissingFunctionName_ThrowsOnMalformedSyntax) {
	TokenList tokenList = {
		MakeToken(TokenType::KwFunc, FUNC, 1, 0),
		MakeToken(TokenType::LParen, "(", 1, 1),
		MakeToken(TokenType::Identifier, "a", 1, 2),
		MakeToken(TokenType::Comma, ",", 1, 3),
		MakeToken(TokenType::Identifier, "b", 1, 4),
		MakeToken(TokenType::RParen, ")", 1, 5),
		MakeToken(TokenType::LBrace, "{", 1, 6),
		MakeToken(TokenType::KwReturn, "return", 2, 0),
		MakeToken(TokenType::Identifier, "a", 2, 1),
		MakeToken(TokenType::Plus, "+", 2, 2),
		MakeToken(TokenType::Identifier, "b", 2, 3),
		MakeToken(TokenType::Semicolon, ";", 2, 4),
		MakeToken(TokenType::RBrace, "}", 3, 0),
		MakeToken(TokenType::EndOfFile, "", 3, 1),
	};

	size_t pos = 0;
	try {
		parser.Parse(tokenList, pos);
		FAIL();
	}
	catch (const std::runtime_error& e) {
		EXPECT_STREQ(e.what(), "Expected a function name after 'func' at line 1");
	}
}

// func add a, b) {
// return a + b;
// }  -- '(' 누락
TEST_F(FunctionStatementParserTest, Parse_MissingOpenParen_ThrowsOnMalformedSyntax) {
	TokenList tokenList = {
		MakeToken(TokenType::KwFunc, FUNC, 1, 0),
		MakeToken(TokenType::Identifier, FUNC_NAME, 1, 1),
		MakeToken(TokenType::Identifier, "a", 1, 2),
		MakeToken(TokenType::Comma, ",", 1, 3),
		MakeToken(TokenType::Identifier, "b", 1, 4),
		MakeToken(TokenType::RParen, ")", 1, 5),
		MakeToken(TokenType::LBrace, "{", 1, 6),
		MakeToken(TokenType::KwReturn, "return", 2, 0),
		MakeToken(TokenType::Identifier, "a", 2, 1),
		MakeToken(TokenType::Plus, "+", 2, 2),
		MakeToken(TokenType::Identifier, "b", 2, 3),
		MakeToken(TokenType::Semicolon, ";", 2, 4),
		MakeToken(TokenType::RBrace, "}", 3, 0),
		MakeToken(TokenType::EndOfFile, "", 3, 1),
	};

	size_t pos = 0;
	try {
		parser.Parse(tokenList, pos);
		FAIL();
	}
	catch (const std::runtime_error& e) {
		EXPECT_STREQ(e.what(), "Invalid Syntax. '(' is Missing");
	}
}

// func add(1, b) {
// return a + b;
// }  -- 파라미터 자리에 식별자가 아닌 토큰
TEST_F(FunctionStatementParserTest, Parse_NonIdentifierParameter_ThrowsOnMalformedSyntax) {
	TokenList tokenList = {
		MakeToken(TokenType::KwFunc, FUNC, 1, 0),
		MakeToken(TokenType::Identifier, FUNC_NAME, 1, 1),
		MakeToken(TokenType::LParen, "(", 1, 2),
		MakeToken(TokenType::Number, "1", 1, 3),
		MakeToken(TokenType::Comma, ",", 1, 4),
		MakeToken(TokenType::Identifier, "b", 1, 5),
		MakeToken(TokenType::RParen, ")", 1, 6),
		MakeToken(TokenType::LBrace, "{", 1, 7),
		MakeToken(TokenType::KwReturn, "return", 2, 0),
		MakeToken(TokenType::Identifier, "a", 2, 1),
		MakeToken(TokenType::Plus, "+", 2, 2),
		MakeToken(TokenType::Identifier, "b", 2, 3),
		MakeToken(TokenType::Semicolon, ";", 2, 4),
		MakeToken(TokenType::RBrace, "}", 3, 0),
		MakeToken(TokenType::EndOfFile, "", 3, 1),
	};

	size_t pos = 0;
	try {
		parser.Parse(tokenList, pos);
		FAIL();
	}
	catch (const std::runtime_error& e) {
		EXPECT_STREQ(e.what(), "Expected a parameter name at line 1");
	}
}

// func add(a, b {
// return a + b;
// }  -- ')' 누락
TEST_F(FunctionStatementParserTest, Parse_MissingCloseParen_ThrowsOnMalformedSyntax) {
	TokenList tokenList = {
		MakeToken(TokenType::KwFunc, FUNC, 1, 0),
		MakeToken(TokenType::Identifier, FUNC_NAME, 1, 1),
		MakeToken(TokenType::LParen, "(", 1, 2),
		MakeToken(TokenType::Identifier, "a", 1, 3),
		MakeToken(TokenType::Comma, ",", 1, 4),
		MakeToken(TokenType::Identifier, "b", 1, 5),
		MakeToken(TokenType::LBrace, "{", 1, 6),
		MakeToken(TokenType::KwReturn, "return", 2, 0),
		MakeToken(TokenType::Identifier, "a", 2, 1),
		MakeToken(TokenType::Plus, "+", 2, 2),
		MakeToken(TokenType::Identifier, "b", 2, 3),
		MakeToken(TokenType::Semicolon, ";", 2, 4),
		MakeToken(TokenType::RBrace, "}", 3, 0),
		MakeToken(TokenType::EndOfFile, "", 3, 1),
	};

	size_t pos = 0;
	try {
		parser.Parse(tokenList, pos);
		FAIL();
	}
	catch (const std::runtime_error& e) {
		EXPECT_STREQ(e.what(), "Invalid Syntax. ')' is Missing");
	}
}

// func add(a, b)  -- 본문 '{' 누락
TEST_F(FunctionStatementParserTest, Parse_MissingBody_ThrowsOnMalformedSyntax) {
	TokenList tokenList = {
		MakeToken(TokenType::KwFunc, FUNC, 1, 0),
		MakeToken(TokenType::Identifier, FUNC_NAME, 1, 1),
		MakeToken(TokenType::LParen, "(", 1, 2),
		MakeToken(TokenType::Identifier, "a", 1, 3),
		MakeToken(TokenType::Comma, ",", 1, 4),
		MakeToken(TokenType::Identifier, "b", 1, 5),
		MakeToken(TokenType::RParen, ")", 1, 6),
		MakeToken(TokenType::EndOfFile, "", 1, 7),
	};

	size_t pos = 0;
	try {
		parser.Parse(tokenList, pos);
		FAIL();
	}
	catch (const std::runtime_error& e) {
		EXPECT_STREQ(e.what(), "Expected a function body at line 1");
	}
}

// func  -- 'func' 뒤에 아무것도 없음
TEST_F(FunctionStatementParserTest, Parse_NothingAfterFunc_ThrowsOnMalformedSyntax) {
	TokenList tokenList = {
		MakeToken(TokenType::KwFunc, FUNC, 1, 0),
		MakeToken(TokenType::EndOfFile, "", 1, 1),
	};

	size_t pos = 0;
	try {
		parser.Parse(tokenList, pos);
		FAIL();
	}
	catch (const std::runtime_error& e) {
		EXPECT_STREQ(e.what(), "Expected a function name after 'func' at line 1");
	}
}
#endif
