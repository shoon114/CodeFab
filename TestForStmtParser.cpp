#ifdef _DEBUG
#include "gmock/gmock.h"
#include "ForStmtParser.h"
#include "MockStatementParser.h"
#include "StatementParserRegistry.h"
#include "TestTokenHelpers.h"
#include "VarDeclareParser.h"

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
		//
		// Unlike Identifier (which every fixture that cares about re-registers
		// in its own SetUp), KwVar's *only* production owner is the real
		// VarDeclareParser, self-registered once at static-init time. Resetting
		// it to a nullptr-returning factory here would permanently destroy that
		// registration for the rest of the test binary's lifetime -- any later
		// test (in any file, depending on link order) that relies on KwVar
		// resolving to the real VarDeclareParser would then fail. Restore the
		// real factory instead of nulling it out.
		StatementParserRegistry::Instance().Register(TokenType::KwVar, []() { return std::make_shared<VarDeclareParser>(); });
		StatementParserRegistry::Instance().Register(TokenType::Identifier, []() { return nullptr; });
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

	// body delegates to the real BlockParser (registered for LBrace), whose
	// own convention appends a marker child for the closing '}'.
	const std::unique_ptr<SyntaxNode>& bodyNode = root->children[3];
	EXPECT_THAT(bodyNode->type, Eq(NodeType::BlockStmt));
	ASSERT_THAT(bodyNode->children, SizeIs(1));
	EXPECT_THAT(bodyNode->children[0]->token.type, Eq(TokenType::RBrace));
}

TEST_F(ForStmtParserTest, Parse_WithStatementInBody_DelegatesBodyToBlockParser) {
	// "for (var i = 0; i < 3; i = i + 1) { print i; }" -- 빈 블록이 아닌
	// 실제 문장을 포함한 body가 BlockParser에 위임되어 파싱되는지 검증.
	TokenList tokenList = {
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
		MakeToken(TokenType::Print, "print", 0, 18),
		MakeToken(TokenType::Identifier, "i", 0, 19),
		MakeToken(TokenType::Semicolon, ";", 0, 20),
		MakeToken(TokenType::RBrace, "}", 0, 21),
		MakeToken(TokenType::EndOfFile, "", 0, 22),
	};

	EXPECT_CALL(*mockInitParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 5; // 'var', 'i', '=', '0', ';'
			return std::make_unique<VarDeclareStatementNode>();
		});
	EXPECT_CALL(*mockIdentifierParser, Parse(_, _))
		.Times(3)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 3; // cond: "i < 3"
			return std::make_unique<BinaryExprNode>();
		})
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 5; // update: "i = i + 1"
			return std::make_unique<AssignExprNode>();
		})
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // print's operand: "i"
			return std::make_unique<IdentifierNode>();
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->children, SizeIs(4));

	// body is the real BlockParser's output: [PrintStmt, '}' marker]
	const std::unique_ptr<SyntaxNode>& bodyNode = root->children[3];
	EXPECT_THAT(bodyNode->type, Eq(NodeType::BlockStmt));
	ASSERT_THAT(bodyNode->children, SizeIs(2));
	EXPECT_THAT(bodyNode->children[0]->type, Eq(NodeType::PrintStmt));
	EXPECT_THAT(bodyNode->children[1]->token.type, Eq(TokenType::RBrace));
	EXPECT_THAT(pos, Eq(tokenList.size() - 1));
}

TEST_F(ForStmtParserTest, Parse_MissingOpenParen_ThrowsOnMalformedSyntax) {
	// "for var i = 0; i < 3; i = i + 1) {}" -- '(' 누락
	TokenList tokenList = {
		MakeToken(TokenType::KwFor, "for", 0, 0),
		MakeToken(TokenType::KwVar, "var", 0, 1),
		MakeToken(TokenType::EndOfFile, "", 0, 2),
	};

	ExpectParseThrows(tokenList, "Expected '(' after 'for' at line 0");
}

TEST_F(ForStmtParserTest, Parse_MissingInitializer_ThrowsOnMalformedSyntax) {
	// "for (; i < 3; i = i + 1) {}" -- 초기화 문 누락 (';'는 등록된 파서가 없음)
	TokenList tokenList = {
		MakeToken(TokenType::KwFor, "for", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::Semicolon, ";", 0, 2),
		MakeToken(TokenType::EndOfFile, "", 0, 3),
	};

	ExpectParseThrows(tokenList, "Expected an initializer statement in 'for' at line 0");
}

TEST_F(ForStmtParserTest, Parse_MissingSemicolonAfterCondition_ThrowsOnMalformedSyntax) {
	// "for (var i = 0; i < 3 i = i + 1) {}" -- 조건식 뒤 ';' 누락
	TokenList tokenList = {
		MakeToken(TokenType::KwFor, "for", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::KwVar, "var", 0, 2),
		MakeToken(TokenType::Identifier, "i", 0, 3),
		MakeToken(TokenType::Identifier, "i", 0, 4),
		MakeToken(TokenType::EndOfFile, "", 0, 5),
	};

	EXPECT_CALL(*mockInitParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume 'var' only (opaque init clause)
			return std::make_unique<VarDeclareStatementNode>();
		});
	EXPECT_CALL(*mockIdentifierParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume 'i' only (opaque condition clause)
			return std::make_unique<BinaryExprNode>();
		});

	ExpectParseThrows(tokenList, "Expected ';' after for-loop condition at line 0");
}

TEST_F(ForStmtParserTest, Parse_MissingCondition_ThrowsOnMalformedSyntax) {
	// "for (var i = 0; ; i = i + 1) {}" -- 조건식 누락 (';'는 등록된 파서가 없음)
	TokenList tokenList = {
		MakeToken(TokenType::KwFor, "for", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::KwVar, "var", 0, 2),
		MakeToken(TokenType::Semicolon, ";", 0, 3),
		MakeToken(TokenType::EndOfFile, "", 0, 4),
	};

	EXPECT_CALL(*mockInitParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume 'var' only (opaque init clause)
			return std::make_unique<VarDeclareStatementNode>();
		});

	ExpectParseThrows(tokenList, "Expected a condition expression in 'for' at line 0");
}

TEST_F(ForStmtParserTest, Parse_MissingCloseParen_ThrowsOnMalformedSyntax) {
	// "for (var i = 0; i < 3; i = i + 1 {}" -- ')' 누락
	TokenList tokenList = {
		MakeToken(TokenType::KwFor, "for", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::KwVar, "var", 0, 2),
		MakeToken(TokenType::Identifier, "i", 0, 3),
		MakeToken(TokenType::Semicolon, ";", 0, 4),
		MakeToken(TokenType::Identifier, "i", 0, 5),
		MakeToken(TokenType::Identifier, "i", 0, 6),
		MakeToken(TokenType::EndOfFile, "", 0, 7),
	};

	EXPECT_CALL(*mockInitParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume 'var' only (opaque init clause)
			return std::make_unique<VarDeclareStatementNode>();
		});
	EXPECT_CALL(*mockIdentifierParser, Parse(_, _))
		.Times(2)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume condition 'i' only (opaque condition clause)
			return std::make_unique<BinaryExprNode>();
		})
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume update 'i' only (opaque update clause)
			return std::make_unique<AssignExprNode>();
		});

	ExpectParseThrows(tokenList, "Expected ')' after for-loop update expression at line 0");
}

TEST_F(ForStmtParserTest, Parse_MissingUpdateExpression_ThrowsOnMalformedSyntax) {
	// "for (var i = 0; i < 3; ) {}" -- update 표현식 누락 (')'는 등록된 파서가 없음)
	TokenList tokenList = {
		MakeToken(TokenType::KwFor, "for", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::KwVar, "var", 0, 2),
		MakeToken(TokenType::Identifier, "i", 0, 3),
		MakeToken(TokenType::Semicolon, ";", 0, 4),
		MakeToken(TokenType::RParen, ")", 0, 5),
		MakeToken(TokenType::EndOfFile, "", 0, 6),
	};

	EXPECT_CALL(*mockInitParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume 'var' only (opaque init clause)
			return std::make_unique<VarDeclareStatementNode>();
		});
	EXPECT_CALL(*mockIdentifierParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume condition 'i' only (opaque condition clause)
			return std::make_unique<BinaryExprNode>();
		});

	ExpectParseThrows(tokenList, "Expected an update expression in 'for' at line 0");
}

TEST_F(ForStmtParserTest, Parse_MissingOpenBrace_ThrowsOnMalformedSyntax) {
	// "for (var i = 0; i < 3; i = i + 1) print i;" -- '{' 누락
	TokenList tokenList = {
		MakeToken(TokenType::KwFor, "for", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::KwVar, "var", 0, 2),
		MakeToken(TokenType::Identifier, "i", 0, 3),
		MakeToken(TokenType::Semicolon, ";", 0, 4),
		MakeToken(TokenType::Identifier, "i", 0, 5),
		MakeToken(TokenType::RParen, ")", 0, 6),
		MakeToken(TokenType::Print, "print", 0, 7),
		MakeToken(TokenType::EndOfFile, "", 0, 8),
	};

	EXPECT_CALL(*mockInitParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume 'var' only (opaque init clause)
			return std::make_unique<VarDeclareStatementNode>();
		});
	EXPECT_CALL(*mockIdentifierParser, Parse(_, _))
		.Times(2)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume condition 'i' only (opaque condition clause)
			return std::make_unique<BinaryExprNode>();
		})
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume update 'i' only (opaque update clause)
			return std::make_unique<AssignExprNode>();
		});

	ExpectParseThrows(tokenList, "Expected '{' to start for-loop body at line 0");
}

TEST_F(ForStmtParserTest, Parse_UnclosedBody_ThrowsOnMalformedSyntax) {
	// "for (var i = 0; i < 3; i = i + 1) {" -- 닫는 '}' 없이 입력이 끝남
	TokenList tokenList = {
		MakeToken(TokenType::KwFor, "for", 0, 0),
		MakeToken(TokenType::LParen, "(", 0, 1),
		MakeToken(TokenType::KwVar, "var", 0, 2),
		MakeToken(TokenType::Identifier, "i", 0, 3),
		MakeToken(TokenType::Semicolon, ";", 0, 4),
		MakeToken(TokenType::Identifier, "i", 0, 5),
		MakeToken(TokenType::RParen, ")", 0, 6),
		MakeToken(TokenType::LBrace, "{", 0, 7),
		// no EndOfFile sentinel: input truly ends here, with no closing '}'
	};

	EXPECT_CALL(*mockInitParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume 'var' only (opaque init clause)
			return std::make_unique<VarDeclareStatementNode>();
		});
	EXPECT_CALL(*mockIdentifierParser, Parse(_, _))
		.Times(2)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume condition 'i' only (opaque condition clause)
			return std::make_unique<BinaryExprNode>();
		})
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 1; // consume update 'i' only (opaque update clause)
			return std::make_unique<AssignExprNode>();
		});

	// Body parsing is now delegated to the real BlockParser (registered for
	// LBrace), so an unclosed body surfaces BlockParser's own error message.
	ExpectParseThrows(tokenList, "Expected '}' to close block");
}
#endif
