#ifdef _DEBUG
#include "gmock/gmock.h"
#include "ImportStatementParser.h"
#include "TestTokenHelpers.h"

using namespace testing;

class ImportStatementParserTest : public Test {
protected:
	ImportStatementParser parser;

	void ExpectParseThrows(const TokenList& tokenList) {
		size_t pos = 0;
		EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
	}
};

TEST_F(ImportStatementParserTest, Parse_ValidImportAlias_ReturnsImportStmtNode) {
	TokenList tokenList = {
		MakeToken(TokenType::KwImport, "import", 1, 0),
		MakeToken(TokenType::String, "sum.txt", 1, 1),
		MakeToken(TokenType::KwAlias, "alias", 1, 2),
		MakeToken(TokenType::Identifier, "sum", 1, 3),
		MakeToken(TokenType::Semicolon, ";", 1, 4),
		MakeToken(TokenType::EndOfFile, "", 1, 5),
	};
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->type, Eq(NodeType::ImportStmt));
	const auto& importNode = static_cast<const ImportStmtNode&>(*root);
	EXPECT_THAT(importNode.pathToken.lexeme, Eq("sum.txt"));
	EXPECT_THAT(importNode.aliasToken.lexeme, Eq("sum"));
	EXPECT_THAT(pos, Eq(5u));
}

TEST_F(ImportStatementParserTest, Parse_MissingStringPath_Throws) {
	ExpectParseThrows({
		MakeToken(TokenType::KwImport, "import", 1, 0),
		MakeToken(TokenType::Identifier, "path", 1, 1),
		MakeToken(TokenType::KwAlias, "alias", 1, 2),
		MakeToken(TokenType::Identifier, "sum", 1, 3),
		MakeToken(TokenType::Semicolon, ";", 1, 4),
	});
}

TEST_F(ImportStatementParserTest, Parse_MissingAliasKeyword_Throws) {
	ExpectParseThrows({
		MakeToken(TokenType::KwImport, "import", 1, 0),
		MakeToken(TokenType::String, "sum.txt", 1, 1),
		MakeToken(TokenType::Identifier, "sum", 1, 2),
		MakeToken(TokenType::Semicolon, ";", 1, 3),
	});
}

TEST_F(ImportStatementParserTest, Parse_MissingSemicolon_Throws) {
	ExpectParseThrows({
		MakeToken(TokenType::KwImport, "import", 1, 0),
		MakeToken(TokenType::String, "sum.txt", 1, 1),
		MakeToken(TokenType::KwAlias, "alias", 1, 2),
		MakeToken(TokenType::Identifier, "sum", 1, 3),
		MakeToken(TokenType::EndOfFile, "", 1, 4),
	});
}
#endif
