#ifdef _DEBUG
#include "gmock/gmock.h"
#include "ImportStatementParser.h"
#include "TestTokenHelpers.h"

using namespace testing;

class ImportStatementParserTest : public Test {
protected:
	ImportStatementParser parser;

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
};

TEST_F(ImportStatementParserTest, Parse_ValidImportStatement_BuildsImportStmtTree) {
	// "import "sum.txt" alias sum;"
	TokenList tokenList = {
		MakeToken(TokenType::KwImport, "import", 0, 0),
		MakeToken(TokenType::String, "sum.txt", 0, 1),
		MakeToken(TokenType::KwAlias, "alias", 0, 2),
		MakeToken(TokenType::Identifier, "sum", 0, 3),
		MakeToken(TokenType::Semicolon, ";", 0, 4),
		MakeToken(TokenType::EndOfFile, "", 0, 5),
	};

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::ImportStmt));
	EXPECT_THAT(root->token.lexeme, Eq("import"));
	ASSERT_THAT(root->children, SizeIs(2));

	const std::unique_ptr<SyntaxNode>& pathNode = root->children[0];
	EXPECT_THAT(pathNode->type, Eq(NodeType::StringLiteral));
	EXPECT_THAT(pathNode->token.lexeme, Eq("sum.txt"));

	const std::unique_ptr<SyntaxNode>& aliasNode = root->children[1];
	EXPECT_THAT(aliasNode->type, Eq(NodeType::Identifier));
	EXPECT_THAT(aliasNode->token.lexeme, Eq("sum"));

	EXPECT_THAT(pos, Eq(tokenList.size() - 1));
}

TEST_F(ImportStatementParserTest, Parse_MissingPathStringLiteral_ThrowsOnMalformedSyntax) {
	// "import alias sum;" -- 경로 문자열 리터럴 누락
	TokenList tokenList = {
		MakeToken(TokenType::KwImport, "import", 0, 0),
		MakeToken(TokenType::KwAlias, "alias", 0, 1),
		MakeToken(TokenType::Identifier, "sum", 0, 2),
		MakeToken(TokenType::Semicolon, ";", 0, 3),
		MakeToken(TokenType::EndOfFile, "", 0, 4),
	};

	ExpectParseThrows(tokenList, "Expected a string literal for the import path at line 0");
}

TEST_F(ImportStatementParserTest, Parse_MissingAliasKeyword_ThrowsOnMalformedSyntax) {
	// "import "sum.txt" sum;" -- 'alias' 키워드 누락
	TokenList tokenList = {
		MakeToken(TokenType::KwImport, "import", 0, 0),
		MakeToken(TokenType::String, "sum.txt", 0, 1),
		MakeToken(TokenType::Identifier, "sum", 0, 2),
		MakeToken(TokenType::Semicolon, ";", 0, 3),
		MakeToken(TokenType::EndOfFile, "", 0, 4),
	};

	ExpectParseThrows(tokenList, "Expected 'alias' after import path at line 0");
}

TEST_F(ImportStatementParserTest, Parse_MissingAliasIdentifier_ThrowsOnMalformedSyntax) {
	// "import "sum.txt" alias;" -- 별칭 식별자 누락
	TokenList tokenList = {
		MakeToken(TokenType::KwImport, "import", 0, 0),
		MakeToken(TokenType::String, "sum.txt", 0, 1),
		MakeToken(TokenType::KwAlias, "alias", 0, 2),
		MakeToken(TokenType::Semicolon, ";", 0, 3),
		MakeToken(TokenType::EndOfFile, "", 0, 4),
	};

	ExpectParseThrows(tokenList, "Expected an alias name after 'alias' at line 0");
}

TEST_F(ImportStatementParserTest, Parse_MissingSemicolon_ThrowsOnMalformedSyntax) {
	// "import "sum.txt" alias sum" -- trailing ';' 누락
	TokenList tokenList = {
		MakeToken(TokenType::KwImport, "import", 0, 0),
		MakeToken(TokenType::String, "sum.txt", 0, 1),
		MakeToken(TokenType::KwAlias, "alias", 0, 2),
		MakeToken(TokenType::Identifier, "sum", 0, 3),
		MakeToken(TokenType::EndOfFile, "", 0, 4),
	};

	ExpectParseThrows(tokenList, "Expected ';' after import statement at line 0");
}
#endif
