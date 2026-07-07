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
			MakeToken(TokenType::KwVar, "var", 0, 0),
			MakeToken(TokenType::Identifier, "a", 0, 1),
			MakeToken(TokenType::Assign, "=", 0, 2),
			MakeToken(TokenType::Number, "3", 0, 3),
			MakeToken(TokenType::Semicolon, ";", 0, 4),
			MakeToken(TokenType::EndOfFile, "", 0, 5),
		};
	}

	// "var a;"
	TokenList MakeVarDeclareWithoutInitializerTokens() {
		return TokenList{
			MakeToken(TokenType::KwVar, "var", 0, 0),
			MakeToken(TokenType::Identifier, "a", 0, 1),
			MakeToken(TokenType::Semicolon, ";", 0, 2),
			MakeToken(TokenType::EndOfFile, "", 0, 3),
		};
	}

	// "var a = b;"
	TokenList MakeVarDeclareWithIdentifierInitializerTokens() {
		return TokenList{
			MakeToken(TokenType::KwVar, "var", 0, 0),
			MakeToken(TokenType::Identifier, "a", 0, 1),
			MakeToken(TokenType::Assign, "=", 0, 2),
			MakeToken(TokenType::Identifier, "b", 0, 3),
			MakeToken(TokenType::Semicolon, ";", 0, 4),
			MakeToken(TokenType::EndOfFile, "", 0, 5),
		};
	}

	// "var a = 3 + 4;"
	TokenList MakeVarDeclareWithBinaryExprInitializerTokens() {
		return TokenList{
			MakeToken(TokenType::KwVar, "var", 0, 0),
			MakeToken(TokenType::Identifier, "a", 0, 1),
			MakeToken(TokenType::Assign, "=", 0, 2),
			MakeToken(TokenType::Number, "3", 0, 3),
			MakeToken(TokenType::Plus, "+", 0, 4),
			MakeToken(TokenType::Number, "4", 0, 5),
			MakeToken(TokenType::Semicolon, ";", 0, 6),
			MakeToken(TokenType::EndOfFile, "", 0, 7),
		};
	}
};

TEST_F(VarDeclareParserTest, Parse_WithInitializer_AttachesExpressionParserResultAsValue) {
	TokenList tokenList = MakeVarDeclareWithInitializerTokens();

	EXPECT_CALL(exprParser, Parse(_, _))
		.Times(1)
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

TEST_F(VarDeclareParserTest, Parse_WithoutInitializer_DoesNotDelegateToExpressionParser) {
	TokenList tokenList = MakeVarDeclareWithoutInitializerTokens();

	EXPECT_CALL(exprParser, Parse(_, _)).Times(0);

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::VarDeclareStatement));
	EXPECT_THAT(root->token.lexeme, Eq("var"));
	ASSERT_THAT(root->children, SizeIs(1));

	const std::unique_ptr<SyntaxNode>& identNode = root->children[0];
	EXPECT_THAT(identNode->type, Eq(NodeType::Identifier));
	EXPECT_THAT(identNode->token.lexeme, Eq("a"));
}

TEST_F(VarDeclareParserTest, Parse_WithIdentifierInitializer_AttachesExpressionParserResultAsValue) {
	// VarDeclareParser must work the same regardless of the node shape exprParser returns
	TokenList tokenList = MakeVarDeclareWithIdentifierInitializerTokens();

	EXPECT_CALL(exprParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<SyntaxNode>();
			node->type = NodeType::Identifier;
			node->token = tokens[pos];
			pos++; // consume the value token
			return node;
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->children, SizeIs(1));

	const std::unique_ptr<SyntaxNode>& assignNode = root->children[0];
	ASSERT_THAT(assignNode->children, SizeIs(2));

	const std::unique_ptr<SyntaxNode>& valueNode = assignNode->children[1];
	EXPECT_THAT(valueNode->type, Eq(NodeType::Identifier));
	EXPECT_THAT(valueNode->token.lexeme, Eq("b"));
}

TEST_F(VarDeclareParserTest, Parse_WithBinaryExprInitializer_AttachesExpressionParserResultAsValue) {
	// VarDeclareParser must work the same regardless of the node shape exprParser returns
	TokenList tokenList = MakeVarDeclareWithBinaryExprInitializerTokens();

	EXPECT_CALL(exprParser, Parse(_, _))
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto left = std::make_unique<SyntaxNode>();
			left->type = NodeType::NumberLiteral;
			left->token = tokens[pos]; // '3'

			const Token& opToken = tokens[pos + 1]; // '+'

			auto right = std::make_unique<SyntaxNode>();
			right->type = NodeType::NumberLiteral;
			right->token = tokens[pos + 2]; // '4'

			auto binaryNode = std::make_unique<SyntaxNode>();
			binaryNode->type = NodeType::BinaryExpr;
			binaryNode->token = opToken;
			binaryNode->children.push_back(std::move(left));
			binaryNode->children.push_back(std::move(right));

			pos += 3; // consume '3', '+', '4'
			return binaryNode;
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->children, SizeIs(1));

	const std::unique_ptr<SyntaxNode>& assignNode = root->children[0];
	ASSERT_THAT(assignNode->children, SizeIs(2));

	const std::unique_ptr<SyntaxNode>& valueNode = assignNode->children[1];
	EXPECT_THAT(valueNode->type, Eq(NodeType::BinaryExpr));
	EXPECT_THAT(valueNode->token.lexeme, Eq("+"));
	ASSERT_THAT(valueNode->children, SizeIs(2));
	EXPECT_THAT(valueNode->children[0]->token.lexeme, Eq("3"));
	EXPECT_THAT(valueNode->children[1]->token.lexeme, Eq("4"));
}
#endif
