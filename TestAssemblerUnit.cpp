#ifdef _DEBUG
#include "gmock/gmock.h"
#include "AssemblerUnit.h"
#include "MockExpressionParser.h"
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
	std::shared_ptr<NiceMock<MockExpressionParser>> exprParser = std::make_shared<NiceMock<MockExpressionParser>>();
	AssemblerUnit assembler{ exprParser };

	// no source code -> empty token list
	TokenList MakeEmptyTokens() {
		EXPECT_CALL(tokenizer, CreateTokenForCode()).WillOnce(Return(TokenList{}));
		return tokenizer.CreateTokenForCode();
	}

	// "var a = 3;"
	TokenList MakeVarDeclareTokens() {
		EXPECT_CALL(tokenizer, CreateTokenForCode())
			.WillOnce(Return(TokenList{
				MakeToken(TokenType::KwVar, "var", 0, 1),
				MakeToken(TokenType::Identifier, "a", 0, 5),
				MakeToken(TokenType::Assign, "=", 0, 7),
				MakeToken(TokenType::Number, "3", 0, 9),
				MakeToken(TokenType::Semicolon, ";", 0, 10),
				MakeToken(TokenType::EndOfFile, "", 0, 11),
			}));
		return tokenizer.CreateTokenForCode();
	}

	// "a + 1;" -- smoke test token stream (no assertions on the resulting tree)
	TokenList MakeExpressionTokens() {
		EXPECT_CALL(tokenizer, CreateTokenForCode())
			.WillOnce(Return(TokenList{
				MakeToken(TokenType::KwVar, "a", 0, 0),
				MakeToken(TokenType::Plus, "+", 0, 2),
				MakeToken(TokenType::Number, "1", 0, 4),
				MakeToken(TokenType::Semicolon, ";", 0, 5),
				MakeToken(TokenType::EndOfFile, "", 0, 6),
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

	EXPECT_CALL(*exprParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<SyntaxNode>();
			node->type = NodeType::NumberLiteral;
			node->token = tokens[pos];
			pos++;
			return node;
		});

	std::unique_ptr<SyntaxNode> root = assembler.Parse(tokenList);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::VarDeclareStatement));
}

TEST_F(AssemblerUnitTest, Parse_ExpressionTest) {
	TokenList tokenList = MakeExpressionTokens();

	assembler.Parse(tokenList);
}

#endif
