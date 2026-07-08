#ifdef _DEBUG
#include "gmock/gmock.h"
#include "AssemblerUnit.h"
#include "MockStatementParser.h"
#include "StatementParserRegistry.h"
#include "Tokenizer.h"
#include "TestTokenHelpers.h"

using namespace testing;

class TestableTokenizer : public Tokenizer {
public:
	MOCK_METHOD(TokenList, CreateTokenForCode, (), (override));
};

class AssemblerUnitTest : public Test {
protected:
	NiceMock<TestableTokenizer> tokenizer;
	AssemblerUnit assembler;

	// VarDeclareParser (and top-level Identifier-led statements) resolve
	// whatever is registered for Identifier via StatementParserRegistry --
	// in production that's the real ExpressionParser.
	std::shared_ptr<MockStatementParser> mockTailParser = std::make_shared<MockStatementParser>();

	void SetUp() override {
		// Capture mockTailParser by value (not `this`): the factory lambda is
		// stored in the global StatementParserRegistry and can outlive this
		// fixture, so capturing `this` would leave a dangling pointer once
		// this test finishes and a later test resolves Identifier again.
		StatementParserRegistry::Instance().Register(TokenType::Identifier, [tailParser = mockTailParser]() {
			return tailParser;
		});
	}

	void TearDown() override {
		// Release our captured mockTailParser from the global registry so it
		// doesn't outlive this fixture (which would otherwise be reported as
		// a leaked mock, or get called again -- with already-satisfied
		// expectations -- by a later test that also resolves Identifier).
		StatementParserRegistry::Instance().Register(TokenType::Identifier, []() {
			return nullptr;
		});
	}

	// no source code -> empty token list
	TokenList MakeEmptyTokens() {
		EXPECT_CALL(tokenizer, CreateTokenForCode()).WillOnce(Return(TokenList{}));
		return tokenizer.CreateTokenForCode();
	}

	// "var a = 3;"
	TokenList MakeVarDeclareTokens() {
		EXPECT_CALL(tokenizer, CreateTokenForCode())
			.WillOnce(Return(TokenList{
				MakeToken(TokenType::KwVar, "var", 0, 0),
				MakeToken(TokenType::Identifier, "a", 0, 1),
				MakeToken(TokenType::Assign, "=", 0, 2),
				MakeToken(TokenType::Number, "3", 0, 3),
				MakeToken(TokenType::Semicolon, ";", 0, 4),
				MakeToken(TokenType::EndOfFile, "", 0, 5),
			}));
		return tokenizer.CreateTokenForCode();
	}

	// "a + 1;" -- top-level Identifier-led statement, no assertions on the resulting tree
	TokenList MakeExpressionTokens() {
		EXPECT_CALL(tokenizer, CreateTokenForCode())
			.WillOnce(Return(TokenList{
				MakeToken(TokenType::Identifier, "a", 0, 0),
				MakeToken(TokenType::Plus, "+", 0, 1),
				MakeToken(TokenType::Number, "1", 0, 2),
				MakeToken(TokenType::Semicolon, ";", 0, 3),
				MakeToken(TokenType::EndOfFile, "", 0, 4),
			}));
		return tokenizer.CreateTokenForCode();
	}
};

TEST_F(AssemblerUnitTest, Parse_CallsWithoutCrashing) {
	TokenList tokenList = MakeEmptyTokens();

	assembler.Parse(tokenList);
}

TEST_F(AssemblerUnitTest, Parse_VarDeclare_DelegatesToRegisteredParser) {
	TokenList tokenList = MakeVarDeclareTokens();

	EXPECT_CALL(*mockTailParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<SyntaxNode>();
			node->type = NodeType::AssignExpr;
			node->token = tokens[pos + 1]; // '='
			pos += 3; // consume 'a', '=', '3'
			return node;
		});

	std::unique_ptr<SyntaxNode> root = assembler.Parse(tokenList);

	// AssemblerUnit wraps every top-level statement in a Program node so
	// ExecutorUnit::Execute (which only runs NodeType::Program trees) can
	// execute multi-statement input.
	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->type, Eq(NodeType::Program));
	ASSERT_THAT(root->children, SizeIs(1));
	EXPECT_THAT(root->children[0]->type, Eq(NodeType::VarDeclareStatement));
}

TEST_F(AssemblerUnitTest, Parse_ExpressionTest) {
	// mockTailParser (registered in SetUp) handles the bare, top-level
	// Identifier-led statement here too -- in this smoke test we don't
	// care what it returns, just that AssemblerUnit delegates to it.
	TokenList tokenList = MakeExpressionTokens();

	EXPECT_CALL(*mockTailParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			pos += 4; // consume 'a', '+', '1', ';'
			return nullptr;
		});

	std::unique_ptr<SyntaxNode> root = assembler.Parse(tokenList);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::Program));
}

#endif
