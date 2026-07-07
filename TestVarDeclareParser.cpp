#ifdef _DEBUG
#include "gmock/gmock.h"
#include "VarDeclareParser.h"
#include "MockExpressionParser.h"
#include "MockStatementParser.h"
#include "StatementParserRegistry.h"
#include "TestTokenHelpers.h"

using namespace testing;

class VarDeclareParserTest : public Test {
protected:
	MockExpressionParser exprParser;
	VarDeclareParser parser{ exprParser };

	// VarDeclareParser resolves whatever is registered for the token right
	// after 'var' (Identifier here) via StatementParserRegistry -- in
	// production that will eventually be an ExprStmtParser. We stand in for
	// it with a mock so this TC doesn't depend on that parser's real
	// implementation existing yet.
	std::shared_ptr<MockStatementParser> mockTailParser = std::make_shared<MockStatementParser>();

	void SetUp() override {
		StatementParserRegistry::Instance().Register(TokenType::Identifier, [this](IExpressionParser&) {
			return mockTailParser;
		});
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

	// "var a = 3;"
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
};

TEST_F(VarDeclareParserTest, Parse_WithoutInitializer_DelegatesToRegisteredTailParser) {
	TokenList tokenList = MakeVarDeclareWithoutInitializerTokens();

	EXPECT_CALL(*mockTailParser, Parse(_, _))
		.Times(1)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			auto node = std::make_unique<SyntaxNode>();
			node->type = NodeType::Identifier;
			node->token = tokens[pos];
			pos++; // consume 'a'
			return node;
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::VarDeclareStatement));
	EXPECT_THAT(root->token.lexeme, Eq("var"));
	ASSERT_THAT(root->children, SizeIs(1));

	const std::unique_ptr<SyntaxNode>& identNode = root->children[0];
	EXPECT_THAT(identNode->type, Eq(NodeType::Identifier));
	EXPECT_THAT(identNode->token.lexeme, Eq("a"));

	EXPECT_THAT(pos, Eq(tokenList.size() - 1)); // consumed up through ';', stopped at EOF
}

TEST_F(VarDeclareParserTest, Parse_WithInitializer_DelegatesToRegisteredTailParser_RegardlessOfReturnedShape) {
	TokenList tokenList = MakeVarDeclareWithInitializerTokens();

	EXPECT_CALL(*mockTailParser, Parse(_, _))
		.Times(1)
		.WillOnce([](const TokenList& tokens, size_t& pos) {
			// simulate whatever gets registered for Identifier (e.g. ExprStmtParser)
			// recognizing "a = 3" as one assignment expression
			auto identNode = std::make_unique<SyntaxNode>();
			identNode->type = NodeType::Identifier;
			identNode->token = tokens[pos]; // 'a'

			auto valueNode = std::make_unique<SyntaxNode>();
			valueNode->type = NodeType::NumberLiteral;
			valueNode->token = tokens[pos + 2]; // '3'

			auto assignNode = std::make_unique<SyntaxNode>();
			assignNode->type = NodeType::AssignExpr;
			assignNode->token = tokens[pos + 1]; // '='
			assignNode->children.push_back(std::move(identNode));
			assignNode->children.push_back(std::move(valueNode));

			pos += 3; // consume 'a', '=', '3'
			return assignNode;
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::VarDeclareStatement));
	EXPECT_THAT(root->token.lexeme, Eq("var"));
	ASSERT_THAT(root->children, SizeIs(1));

	const std::unique_ptr<SyntaxNode>& assignNode = root->children[0];
	EXPECT_THAT(assignNode->type, Eq(NodeType::AssignExpr));
	EXPECT_THAT(assignNode->token.lexeme, Eq("="));
	ASSERT_THAT(assignNode->children, SizeIs(2));

	const std::unique_ptr<SyntaxNode>& identNode = assignNode->children[0];
	EXPECT_THAT(identNode->type, Eq(NodeType::Identifier));
	EXPECT_THAT(identNode->token.lexeme, Eq("a"));

	const std::unique_ptr<SyntaxNode>& valueNode = assignNode->children[1];
	EXPECT_THAT(valueNode->type, Eq(NodeType::NumberLiteral));
	EXPECT_THAT(valueNode->token.lexeme, Eq("3"));

	EXPECT_THAT(pos, Eq(tokenList.size() - 1)); // consumed up through ';', stopped at EOF
}
#endif
