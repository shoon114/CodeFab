#ifdef _DEBUG
#include <string>
#include <utility>
#include "gmock/gmock.h"
#include "AssemblerUnit.h"
#include "BlockParser.h"
#include "CheckerUnit.h"
#include "ExecutorUnit.h"
#include "ExpressionParser.h"
#include "IfStatementParser.h"
#include "MockStatementParser.h"
#include "SourceFileLoader.h"
#include "StatementParserRegistry.h"
#include "TestTokenHelpers.h"

using namespace testing;

namespace {
	TokenList MakeTokens(std::initializer_list<std::pair<TokenType, std::string>> tokens) {
		TokenList result;
		int column = 0;
		for (const auto& [type, lexeme] : tokens) {
			result.push_back(MakeToken(type, lexeme, 1, column++));
		}
		return result;
	}
}

class IfStatementParserTest : public Test {
protected:
	IfStatementParser parser;

	// IfStatementParser resolves whatever is registered for the token right
	// after '(' (Identifier here) via StatementParserRegistry -- in
	// production that's the real ExpressionParser. We stand in for it with
	// a mock so this TC doesn't depend on ExpressionParser's real behavior.
	std::shared_ptr<MockStatementParser> mockConditionParser = std::make_shared<MockStatementParser>();

	// IfStatementParser resolves whatever is registered for the token right
	// after ')' (LBrace here) via StatementParserRegistry -- in production
	// that's the real BlockParser. We stand in for it with a mock so this TC
	// doesn't depend on BlockParser's real behavior.
	std::shared_ptr<MockStatementParser> mockThenParser = std::make_shared<MockStatementParser>();

	void SetUp() override {
		// Capture mocks by value (not `this`): the factory lambdas are
		// stored in the global StatementParserRegistry and can outlive
		// this fixture.
		StatementParserRegistry::Instance().Register(TokenType::Identifier, [conditionParser = mockConditionParser]() {
			return conditionParser;
		});
		StatementParserRegistry::Instance().Register(TokenType::LBrace, [thenParser = mockThenParser]() {
			return thenParser;
		});
	}

	void TearDown() override {
		// Release our captured mockConditionParser from the global registry
		// so it doesn't outlive this fixture (which would otherwise be
		// reported as a leaked mock, or get called again -- with
		// already-satisfied expectations -- by a later test).
		StatementParserRegistry::Instance().Register(TokenType::Identifier, []() {
			return nullptr;
		});

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

	// "if (a <op> 3) { }"
	TokenList MakeIfConditionTokens(TokenType opType, const std::string& opLexeme) {
		return MakeTokens({
			{TokenType::KwIf, "if"},
			{TokenType::LParen, "("},
			{TokenType::Identifier, "a"},
			{opType, opLexeme},
			{TokenType::Number, "3"},
			{TokenType::RParen, ")"},
			{TokenType::LBrace, "{"},
			{TokenType::RBrace, "}"},
			{TokenType::EndOfFile, ""},
			});
	}

	void StubConditionParserToConsumeCondition() {
		EXPECT_CALL(*mockConditionParser, Parse(_, _))
			.Times(1)
			.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<BinaryExprNode>();
			node->token = tokens[pos];
			pos += 3;
			return node;
				});
	}

	void StubThenParserToConsumeBody() {
		EXPECT_CALL(*mockThenParser, Parse(_, _))
			.Times(1)
			.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<BlockStmtNode>();
			node->token = tokens[pos];
			pos += 2;
			return node;
				});
	}

	void StubThenParserToConsumeBodies(int times) {
		EXPECT_CALL(*mockThenParser, Parse(_, _))
			.Times(times)
			.WillRepeatedly([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<BlockStmtNode>();
			node->token = tokens[pos];
			pos += 2;
			return node;
				});
	}

	void StubConditionParserToConsumeConditions(int times) {
		EXPECT_CALL(*mockConditionParser, Parse(_, _))
			.Times(times)
			.WillRepeatedly([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<BinaryExprNode>();
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
	StubConditionParserToConsumeCondition();
	StubThenParserToConsumeBody();

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::IfStmt));
	EXPECT_THAT(root->token.lexeme, Eq("if"));
	ASSERT_THAT(root->children, SizeIs(2));

	const std::unique_ptr<SyntaxNode>& conditionNode = root->children[0];
	EXPECT_THAT(conditionNode->type, Eq(NodeType::BinaryExpr));

	const std::unique_ptr<SyntaxNode>& thenNode = root->children[1];
	EXPECT_THAT(thenNode->type, Eq(NodeType::BlockStmt));
}

TEST_F(IfStatementParserTest, Parse_Condition_WithGtEqOperator_AttachesExpressionParserResultAsCondition) {
	TokenList tokenList = MakeIfConditionTokens(TokenType::GtEq, ">=");
	StubConditionParserToConsumeCondition();
	StubThenParserToConsumeBody();

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::IfStmt));
	EXPECT_THAT(root->token.lexeme, Eq("if"));
	ASSERT_THAT(root->children, SizeIs(2));

	const std::unique_ptr<SyntaxNode>& conditionNode = root->children[0];
	EXPECT_THAT(conditionNode->type, Eq(NodeType::BinaryExpr));

	const std::unique_ptr<SyntaxNode>& thenNode = root->children[1];
	EXPECT_THAT(thenNode->type, Eq(NodeType::BlockStmt));
}

TEST_F(IfStatementParserTest, Parse_MissingOpenParen_ThrowsOnMalformedSyntax) {
	// if a>3)   -- '(' 누락
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::Identifier, "a"},
		{TokenType::Gt, ">"},
		{TokenType::Number, "3"},
		{TokenType::RParen, ")"},
		{TokenType::EndOfFile, ""},
		});

	ExpectParseThrows(tokenList, "Expected '(' after 'if' at line 1");
}

TEST_F(IfStatementParserTest, Parse_MissingCloseParen_ThrowsOnMalformedSyntax) {
	// if(a>3   -- ')' 누락
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "a"},
		{TokenType::Gt, ">"},
		{TokenType::Number, "3"},
		{TokenType::EndOfFile, ""},
		});

	EXPECT_CALL(*mockConditionParser, Parse(_, _))
		.Times(1)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
		auto node = std::make_unique<BinaryExprNode>();
		node->token = tokens[pos];
		pos += 3;
		return node;
			});

	ExpectParseThrows(tokenList, "Expected ')' after if-condition at line 1");
}

TEST_F(IfStatementParserTest, Parse_MissingCondition_ThrowsOnMalformedSyntax) {
	// if () { }   -- 조건식 누락
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::RParen, ")"},
		{TokenType::LBrace, "{"},
		{TokenType::RBrace, "}"},
		{TokenType::EndOfFile, ""},
		});

	ExpectParseThrows(tokenList, "Expected a condition expression in 'if' at line 1");
}

TEST_F(IfStatementParserTest, Parse_WithElse_AttachesElseBranchAsThirdChild) {
	// if (a > 3) { } else { }
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "a"},
		{TokenType::Gt, ">"},
		{TokenType::Number, "3"},
		{TokenType::RParen, ")"},
		{TokenType::LBrace, "{"},
		{TokenType::RBrace, "}"},
		{TokenType::KwElse, "else"},
		{TokenType::LBrace, "{"},
		{TokenType::RBrace, "}"},
		{TokenType::EndOfFile, ""},
		});
	StubConditionParserToConsumeCondition();
	StubThenParserToConsumeBodies(2);

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->children, SizeIs(3));

	const std::unique_ptr<SyntaxNode>& elseNode = root->children[2];
	EXPECT_THAT(elseNode->type, Eq(NodeType::BlockStmt));
}

TEST_F(IfStatementParserTest, Parse_WithElseIf_AttachesNestedIfStmtAsThirdChild) {
	// if (a > 3) { } else if (b < 5) { }
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "a"},
		{TokenType::Gt, ">"},
		{TokenType::Number, "3"},
		{TokenType::RParen, ")"},
		{TokenType::LBrace, "{"},
		{TokenType::RBrace, "}"},
		{TokenType::KwElse, "else"},
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "b"},
		{TokenType::Lt, "<"},
		{TokenType::Number, "5"},
		{TokenType::RParen, ")"},
		{TokenType::LBrace, "{"},
		{TokenType::RBrace, "}"},
		{TokenType::EndOfFile, ""},
		});
	StubConditionParserToConsumeConditions(2);
	StubThenParserToConsumeBodies(2);

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->children, SizeIs(3));

	const std::unique_ptr<SyntaxNode>& elseIfNode = root->children[2];
	EXPECT_THAT(elseIfNode->type, Eq(NodeType::IfStmt));
	ASSERT_THAT(elseIfNode->children, SizeIs(2));
	EXPECT_THAT(elseIfNode->children[0]->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(elseIfNode->children[1]->type, Eq(NodeType::BlockStmt));
}

TEST_F(IfStatementParserTest, Parse_WithElseIfAndElse_AttachesFullChain) {
	// if (a > 3) { } else if (b < 5) { } else { }
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "a"},
		{TokenType::Gt, ">"},
		{TokenType::Number, "3"},
		{TokenType::RParen, ")"},
		{TokenType::LBrace, "{"},
		{TokenType::RBrace, "}"},
		{TokenType::KwElse, "else"},
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "b"},
		{TokenType::Lt, "<"},
		{TokenType::Number, "5"},
		{TokenType::RParen, ")"},
		{TokenType::LBrace, "{"},
		{TokenType::RBrace, "}"},
		{TokenType::KwElse, "else"},
		{TokenType::LBrace, "{"},
		{TokenType::RBrace, "}"},
		{TokenType::EndOfFile, ""},
		});
	StubConditionParserToConsumeConditions(2);
	StubThenParserToConsumeBodies(3);

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->children, SizeIs(3));
	EXPECT_THAT(root->children[0]->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(root->children[1]->type, Eq(NodeType::BlockStmt));

	const std::unique_ptr<SyntaxNode>& elseIfNode = root->children[2];
	EXPECT_THAT(elseIfNode->type, Eq(NodeType::IfStmt));
	ASSERT_THAT(elseIfNode->children, SizeIs(3));
	EXPECT_THAT(elseIfNode->children[0]->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(elseIfNode->children[1]->type, Eq(NodeType::BlockStmt));
	EXPECT_THAT(elseIfNode->children[2]->type, Eq(NodeType::BlockStmt));
}

TEST_F(IfStatementParserTest, Parse_ThenBodyWithoutBraces_AttachesSingleStatementAsThenNode) {
	// if (a > 3) x = 1;
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "a"},
		{TokenType::Gt, ">"},
		{TokenType::Number, "3"},
		{TokenType::RParen, ")"},
		{TokenType::Identifier, "x"},
		{TokenType::Assign, "="},
		{TokenType::Number, "1"},
		{TokenType::Semicolon, ";"},
		{TokenType::EndOfFile, ""},
		});
	StubConditionParserToConsumeConditions(2); // condition(a>3) + no-brace then-body(x=1)

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->children, SizeIs(2));
	EXPECT_THAT(pos, Eq(tokenList.size() - 1)); // 세미콜론까지 소비하고 EndOfFile에 멈춰야 함
}

TEST_F(IfStatementParserTest, Parse_ThenAndElseBodyWithoutBraces_AttachesBothAsRawStatements) {
	// if (a > 3) x = 1; else y = 2;
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "a"},
		{TokenType::Gt, ">"},
		{TokenType::Number, "3"},
		{TokenType::RParen, ")"},
		{TokenType::Identifier, "x"},
		{TokenType::Assign, "="},
		{TokenType::Number, "1"},
		{TokenType::Semicolon, ";"},
		{TokenType::KwElse, "else"},
		{TokenType::Identifier, "y"},
		{TokenType::Assign, "="},
		{TokenType::Number, "2"},
		{TokenType::Semicolon, ";"},
		{TokenType::EndOfFile, ""},
		});
	StubConditionParserToConsumeConditions(3); // condition(a>3) + then-body(x=1) + else-body(y=2)

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->children, SizeIs(3));
	EXPECT_THAT(pos, Eq(tokenList.size() - 1));
}

TEST_F(IfStatementParserTest, Parse_BraceThenAndNoBraceElse_AttachesMixedBranches) {
	// if (a > 3) { } else x = 1;
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "a"},
		{TokenType::Gt, ">"},
		{TokenType::Number, "3"},
		{TokenType::RParen, ")"},
		{TokenType::LBrace, "{"},
		{TokenType::RBrace, "}"},
		{TokenType::KwElse, "else"},
		{TokenType::Identifier, "x"},
		{TokenType::Assign, "="},
		{TokenType::Number, "1"},
		{TokenType::Semicolon, ";"},
		{TokenType::EndOfFile, ""},
		});
	StubConditionParserToConsumeConditions(2); // condition(a>3) + no-brace else-body(x=1)
	StubThenParserToConsumeBody(); // brace then-body { }

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->children, SizeIs(3));
	EXPECT_THAT(root->children[1]->type, Eq(NodeType::BlockStmt)); // then은 여전히 중괄호 블록
	EXPECT_THAT(pos, Eq(tokenList.size() - 1));
}

TEST_F(IfStatementParserTest, Parse_ElseIfWithNoBraceBody_AttachesNestedIfWithRawStatement) {
	// if (a > 3) x = 1; else if (b < 5) y = 2;
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "a"},
		{TokenType::Gt, ">"},
		{TokenType::Number, "3"},
		{TokenType::RParen, ")"},
		{TokenType::Identifier, "x"},
		{TokenType::Assign, "="},
		{TokenType::Number, "1"},
		{TokenType::Semicolon, ";"},
		{TokenType::KwElse, "else"},
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "b"},
		{TokenType::Lt, "<"},
		{TokenType::Number, "5"},
		{TokenType::RParen, ")"},
		{TokenType::Identifier, "y"},
		{TokenType::Assign, "="},
		{TokenType::Number, "2"},
		{TokenType::Semicolon, ";"},
		{TokenType::EndOfFile, ""},
		});
	StubConditionParserToConsumeConditions(4); // cond(a>3) + body(x=1) + cond(b<5) + body(y=2)

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->children, SizeIs(3));

	const std::unique_ptr<SyntaxNode>& elseIfNode = root->children[2];
	EXPECT_THAT(elseIfNode->type, Eq(NodeType::IfStmt));
	ASSERT_THAT(elseIfNode->children, SizeIs(2));
	EXPECT_THAT(pos, Eq(tokenList.size() - 1));
}

TEST_F(IfStatementParserTest, Parse_ThenBodyWithoutBraces_DoesNotSwallowFollowingStatement) {
	// if (a > 3) x = 1; y = 2;
	// 중괄호 없는 then-body는 딱 한 문장(x = 1;)까지만 소비하고,
	// 그 다음 문장(y = 2;)은 건드리지 않아야 한다.
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "a"},
		{TokenType::Gt, ">"},
		{TokenType::Number, "3"},
		{TokenType::RParen, ")"},
		{TokenType::Identifier, "x"},
		{TokenType::Assign, "="},
		{TokenType::Number, "1"},
		{TokenType::Semicolon, ";"},
		{TokenType::Identifier, "y"},
		{TokenType::Assign, "="},
		{TokenType::Number, "2"},
		{TokenType::Semicolon, ";"},
		{TokenType::EndOfFile, ""},
		});
	StubConditionParserToConsumeConditions(2); // condition(a>3) + no-brace then-body(x=1)

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->children, SizeIs(2));

	ASSERT_THAT(pos, Lt(tokenList.size()));
	EXPECT_THAT(tokenList[pos].lexeme, Eq("y"));
}

TEST_F(IfStatementParserTest, Execute_IfSingleStatementVarDeclare_DoesNotLeakOutsideScope) {
	// if (true) var a = 1;
	// print a;
	// 여러 모듈이 함께 만드는 버그라 Mock으로는 검증이 어려워서 Fake Object로 진행.
	StatementParserRegistry::Instance().Register(TokenType::Identifier, []() {
		return std::make_shared<ExpressionParser>();
		});

	TokenList tokenList = SourceFileLoader::Tokenize(
		"if (true) var a = 1;\n"
		"print a;\n");

	AssemblerUnit assembler;
	CheckerUnit checker;
	ExecutorUnit executor;
	std::unique_ptr<SyntaxNode> tree = assembler.Parse(tokenList);
	checker.Check(tree.get());

	try {
		executor.Execute(*tree);
		FAIL();
	}
	catch (const std::runtime_error& e) {
		EXPECT_THAT(std::string(e.what()), HasSubstr("Undefined variable 'a'"));
	}
}

TEST_F(IfStatementParserTest, Parse_ElseIfPropagatesInnerIfError_ThrowsOnMalformedSyntax) {
	// if (a > 3) { } else if a   -- else if 안쪽 if에 '(' 누락, 재귀 호출이
	// 안쪽 에러를 그대로 밖으로 전달하는지 확인
	TokenList tokenList = MakeTokens({
		{TokenType::KwIf, "if"},
		{TokenType::LParen, "("},
		{TokenType::Identifier, "a"},
		{TokenType::Gt, ">"},
		{TokenType::Number, "3"},
		{TokenType::RParen, ")"},
		{TokenType::LBrace, "{"},
		{TokenType::RBrace, "}"},
		{TokenType::KwElse, "else"},
		{TokenType::KwIf, "if"},
		{TokenType::Identifier, "a"},
		{TokenType::EndOfFile, ""},
		});
	StubConditionParserToConsumeCondition();
	StubThenParserToConsumeBody();

	ExpectParseThrows(tokenList, "Expected '(' after 'if' at line 1");
}
#endif
