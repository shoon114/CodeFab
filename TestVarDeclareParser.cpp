#ifdef _DEBUG
#include "gmock/gmock.h"
#include "VarDeclareParser.h"
#include "MockExpressionParser.h"
#include "TestTokenHelpers.h"

using namespace testing;

class VarDeclareParserTest : public Test {
protected:
	MockExpressionParser exprParser;
	VarDeclareParser parser{ exprParser };

	// "var a = <expr>;"
	TokenList MakeVarDeclareWithInitializerTokens() {
		return TokenList{
			MakeToken(TokenType::KwVar, "var", 1, 1),
			MakeToken(TokenType::Identifier, "a", 1, 5),
			MakeToken(TokenType::Assign, "=", 1, 7),
			MakeToken(TokenType::Number, "3", 1, 9),
			MakeToken(TokenType::Semicolon, ";", 1, 10),
			MakeToken(TokenType::EndOfFile, "", 1, 11),
		};
	}
};

TEST_F(VarDeclareParserTest, Parse_WithInitializer_AttachesExpressionParserResultAsValue) {
	TokenList tokenList = MakeVarDeclareWithInitializerTokens();

	EXPECT_CALL(exprParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<SyntaxNode>();
			node->type = NodeType::NumberLiteral;
			node->token = tokens[pos];
			pos++; // consume the value token
			return node;
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

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
