#ifdef _DEBUG
#include "gmock/gmock.h"
#include "AssemblerUnit.h"

using namespace testing;

TEST(AssemblerUnitTest, Parse_CallsWithoutCrashing) {
	AssemblerUnit assembler;
	TokenList tokenList;

	assembler.Parse(tokenList);
}

TEST(AssemblerUnitTest, Parse_VarDeclare_BuildsVarDeclareStatementTree) {
	// var a = 3;
	TokenList tokenList = {
		{ TokenType::KwVar, "var", 1, 1 },
		{ TokenType::Identifier, "a", 1, 5 },
		{ TokenType::Assign, "=", 1, 7 },
		{ TokenType::Number, "3", 1, 9 },
		{ TokenType::Semicolon, ";", 1, 10 },
		{ TokenType::EndOfFile, "", 1, 11 },
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
#endif
