#ifdef _DEBUG
#include "gmock/gmock.h"
#include "AssemblerUnit.h"
#include "Tokenizer.h"

using namespace testing;

class TestableTokenizer : public Tokenizer {
public:
	MOCK_METHOD(TokenList, CreateTokenForCode, (), (override));
};

TEST(AssemblerUnitTest, Parse_CallsWithoutCrashing) {
	AssemblerUnit assembler;
	TokenList tokenList;

	assembler.Parse(tokenList);
}

TEST(AssemblerUnitTest, Parse_VarDeclare_BuildsVarDeclareStatementTree) {
	// var a = 3;
	TokenList tokenList = {
		{ TokenType::KwVar, "var", 0.0, "var a = 3;", 1, 1 },
		{ TokenType::Identifier, "a", 0.0, "var a = 3;", 1, 5 },
		{ TokenType::Assign, "=", 0.0, "var a = 3;", 1, 7 },
		{ TokenType::Number, "3", 3.0, "var a = 3;", 1, 9 },
		{ TokenType::Semicolon, ";", 0.0, "var a = 3;", 1, 10 },
		{ TokenType::EndOfFile, "", 0.0, "var a = 3;", 1, 11 },
	};

	AssemblerUnit assembler;
	std::unique_ptr<SyntaxNode> root = assembler.Parse(tokenList);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::VarDeclareStatement));
	EXPECT_THAT(root->token.lexeme, Eq("var"));
	ASSERT_THAT(root->children, SizeIs(1));

	const std::unique_ptr<SyntaxNode>& assignNode = root->children[0];
	EXPECT_THAT(assignNode->type, Eq(NodeType::AssignExpr));
	ASSERT_THAT(assignNode->children, SizeIs(2));

	const std::unique_ptr<SyntaxNode>& identNode = assignNode->children[0];
	EXPECT_THAT(identNode->type, Eq(NodeType::Identifier));
	EXPECT_THAT(identNode->token.lexeme, Eq("a"));

	const std::unique_ptr<SyntaxNode>& valueNode = assignNode->children[1];
	EXPECT_THAT(valueNode->type, Eq(NodeType::NumberLiteral));
	EXPECT_THAT(valueNode->token.lexeme, Eq("3"));
}

TEST(AssemblerUnitTest, Parse_ExpressionTest) {
	AssemblerUnit assembler;
	NiceMock<TestableTokenizer> tokenizer;
	EXPECT_CALL(tokenizer, CreateTokenForCode())
		.WillOnce(Return(TokenList{
			Token{TokenType::KwVar, "a", 0.0, "a + 1;", 1, 0},
			Token{TokenType::Plus, "+", 0.0, "a + 1;", 1, 2},
			Token{TokenType::Number, "1", 1.0, "a + 1;", 1, 4},
			Token{TokenType::Semicolon, ";", 0.0, "a + 1;", 1, 5},
			Token{TokenType::EndOfFile, "", 0.0, "a + 1;", 1, 6} }));

	assembler.Parse(tokenizer.CreateTokenForCode());
}

#endif
