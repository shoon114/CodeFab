#ifdef _DEBUG
#include "gmock/gmock.h"
#include "ImportStatementParser.h"
#include "MockStatementParser.h"
#include "StatementParserRegistry.h"
#include "TestTokenHelpers.h"
#include "VarDeclareParser.h"

using namespace testing;

class ImportStatementParserTest : public Test {
protected:
	ImportStatementParser parser;

	// ImportStatementParser는 module content의 각 문장을 leading token으로 registry에서
	// resolve한 파서에 그대로 위임한다 -- production에서는 KwVar에 VarDeclareParser가
	// 등록되어 있다. 그 파서 자체의 동작에 의존하지 않도록 Mock으로 대체한다.
	std::shared_ptr<MockStatementParser> mockModuleContentParser = std::make_shared<MockStatementParser>();

	void SetUp() override {
		StatementParserRegistry::Instance().Register(TokenType::KwVar, [contentParser = mockModuleContentParser]() {
			return contentParser;
		});
	}

	void TearDown() override {
		// KwVar의 유일한 production 소유자는 VarDeclareParser이므로(static 초기화 시
		// 자기 등록), 여기서 nullptr로 지워버리면 이후 다른 테스트 파일들이 실제
		// VarDeclareParser를 필요로 할 때 영구히 깨진다. LBrace/BlockParser와 동일한
		// 이유로 실제 파서를 복원한다.
		StatementParserRegistry::Instance().Register(TokenType::KwVar, []() {
			return std::make_shared<VarDeclareParser>();
		});
	}

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
	// "import "sum.txt" alias sum;" -- Tokenizer가 스플라이스 내용 없이(빈 파일) 심어두는
	// ImportEnd 마커까지 포함한 모양.
	TokenList tokenList = {
		MakeToken(TokenType::KwImport, "import", 0, 0),
		MakeToken(TokenType::String, "sum.txt", 0, 1),
		MakeToken(TokenType::KwAlias, "alias", 0, 2),
		MakeToken(TokenType::Identifier, "sum", 0, 3),
		MakeToken(TokenType::Semicolon, ";", 0, 4),
		MakeToken(TokenType::ImportEnd, "", 0, 5),
		MakeToken(TokenType::EndOfFile, "", 0, 6),
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

TEST_F(ImportStatementParserTest, Parse_ImportEndBoundary_ParsesModuleContentIntoChildren) {
	// Tokenizer가 module content 자리에 KwVar로 시작하는 문장 하나를 스플라이스하고
	// 그 뒤에 ImportEnd를 심어둔 모양. mockModuleContentParser가 그 한 문장(KwVar 토큰
	// 하나)을 소비하고 VarDeclareStatementNode를 돌려준다고 가정한다.
	TokenList tokenList = {
		MakeToken(TokenType::KwImport, "import", 0, 0),
		MakeToken(TokenType::String, "math.cf", 0, 1),
		MakeToken(TokenType::KwAlias, "alias", 0, 2),
		MakeToken(TokenType::Identifier, "sum", 0, 3),
		MakeToken(TokenType::Semicolon, ";", 0, 4),
		MakeToken(TokenType::KwVar, "var", 1, 0),
		MakeToken(TokenType::ImportEnd, "", 1, 1),
		MakeToken(TokenType::EndOfFile, "", 1, 2),
	};

	EXPECT_CALL(*mockModuleContentParser, Parse(_, _))
		.WillOnce([](const TokenList&, size_t& pos) {
			pos++; // KwVar 토큰만 소비했다고 가정
			return std::make_unique<VarDeclareStatementNode>();
		});

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root->children, SizeIs(3));
	EXPECT_THAT(root->children[2]->type, Eq(NodeType::VarDeclareStatement));
	EXPECT_THAT(pos, Eq(tokenList.size() - 1)); // ImportEnd까지 소비하고 EndOfFile 앞에서 멈춘다
}

TEST_F(ImportStatementParserTest, Parse_NestedImportBeforeImportEnd_ConsumesEachBoundarySeparately) {
	// outer import 대상 파일이 자신 안에 inner import를 갖고 있는 경우: outer.children[2]는
	// 그 자체로 ImportStmtNode이고, inner의 ImportEnd와 outer의 ImportEnd가 각각 자기
	// 위치에서 소비되어야 한다.
	TokenList tokenList = {
		MakeToken(TokenType::KwImport, "import", 0, 0),
		MakeToken(TokenType::String, "outer.cf", 0, 1),
		MakeToken(TokenType::KwAlias, "alias", 0, 2),
		MakeToken(TokenType::Identifier, "outer", 0, 3),
		MakeToken(TokenType::Semicolon, ";", 0, 4),
		MakeToken(TokenType::KwImport, "import", 1, 0),
		MakeToken(TokenType::String, "inner.cf", 1, 1),
		MakeToken(TokenType::KwAlias, "alias", 1, 2),
		MakeToken(TokenType::Identifier, "inner", 1, 3),
		MakeToken(TokenType::Semicolon, ";", 1, 4),
		MakeToken(TokenType::ImportEnd, "", 1, 5), // inner의 경계
		MakeToken(TokenType::ImportEnd, "", 0, 6), // outer의 경계
		MakeToken(TokenType::EndOfFile, "", 0, 7),
	};

	size_t pos = 0;
	std::unique_ptr<SyntaxNode> root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root->children, SizeIs(3));
	EXPECT_THAT(root->children[2]->type, Eq(NodeType::ImportStmt));
	EXPECT_THAT(root->children[2]->children, SizeIs(2)); // inner 자신은 module content가 없음
	EXPECT_THAT(pos, Eq(tokenList.size() - 1));
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
