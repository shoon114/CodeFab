#ifdef _DEBUG
#include "gmock/gmock.h"
#include "BlockParser.h"
#include "ClassStatementParser.h"
#include "MockStatementParser.h"
#include "StatementParserRegistry.h"
#include "TestTokenHelpers.h"
using namespace testing;

class ClassStatementParserTest : public Test {
protected:
	ClassStatementParser parser;
	std::shared_ptr<MockStatementParser> mockBodyParser = std::make_shared<MockStatementParser>();

	void SetUp() override {
		StatementParserRegistry::Instance().Register(TokenType::LBrace,
			[bodyParser = mockBodyParser]() { return bodyParser; });
	}

	void TearDown() override {
		// nullptr 대신 실제 BlockParser로 복원한다. LBrace의 유일한 실제 소유자는
		// BlockParser이며, nullptr로 덮어쓰면 이후 실행되는 ForStmtParserTest 등
		// LBrace를 직접 목킹하지 않는 테스트가 깨진다(ForStmtParser.TearDown의
		// KwVar 복원 방식과 동일한 이유).
		StatementParserRegistry::Instance().Register(TokenType::LBrace,
			[]() { return std::make_shared<BlockParser>(); });
	}

	void StubBodyParserToConsume(int tokenCount) {
		EXPECT_CALL(*mockBodyParser, Parse(_, _))
			.Times(1)
			.WillOnce([tokenCount](const TokenList& tokens, size_t& pos) {
				auto node = std::make_unique<BlockStmtNode>();
				node->token = tokens[pos];
				pos += tokenCount;
				return node;
			});
	}
};

TEST_F(ClassStatementParserTest, Parse_EmptyClass_CreatesClassDeclNode) {
	TokenList tokenList = {
		MakeToken(TokenType::KwClass, "Class", 1, 0),
		MakeToken(TokenType::Identifier, "Robot", 1, 1),
		MakeToken(TokenType::LBrace, "{", 1, 2),
		MakeToken(TokenType::RBrace, "}", 1, 3),
		MakeToken(TokenType::EndOfFile, "", 1, 4),
	};

	size_t pos = 0;
	auto root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::ClassDeclStmt));
	EXPECT_THAT(root->token.lexeme, Eq("Robot"));
	EXPECT_THAT(static_cast<ClassDeclStmtNode*>(root.get())->parentNameToken.lexeme, IsEmpty());
	EXPECT_THAT(root->children, IsEmpty());
	EXPECT_THAT(pos, Eq(4u));
}

TEST_F(ClassStatementParserTest, Parse_InheritedClass_SetsParentNameToken) {
	TokenList tokenList = {
		MakeToken(TokenType::KwClass, "Class", 1, 0),
		MakeToken(TokenType::Identifier, "SpeedRobot", 1, 1),
		MakeToken(TokenType::Colon, ":", 1, 2),
		MakeToken(TokenType::Identifier, "Robot", 1, 3),
		MakeToken(TokenType::LBrace, "{", 1, 4),
		MakeToken(TokenType::RBrace, "}", 1, 5),
		MakeToken(TokenType::EndOfFile, "", 1, 6),
	};

	size_t pos = 0;
	auto root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	EXPECT_THAT(root->type, Eq(NodeType::ClassDeclStmt));
	EXPECT_THAT(root->token.lexeme, Eq("SpeedRobot"));
	EXPECT_THAT(static_cast<ClassDeclStmtNode*>(root.get())->parentNameToken.lexeme, Eq("Robot"));
	EXPECT_THAT(root->children, IsEmpty());
	EXPECT_THAT(pos, Eq(6u));
}

TEST_F(ClassStatementParserTest, Parse_MethodWithParams_AttachesFuncDeclNode) {
	// Class Robot { init(name, speed) { } }
	TokenList tokenList = {
		MakeToken(TokenType::KwClass, "Class", 1, 0),
		MakeToken(TokenType::Identifier, "Robot", 1, 1),
		MakeToken(TokenType::LBrace, "{", 1, 2),
		MakeToken(TokenType::Identifier, "init", 2, 0),
		MakeToken(TokenType::LParen, "(", 2, 1),
		MakeToken(TokenType::Identifier, "name", 2, 2),
		MakeToken(TokenType::Comma, ",", 2, 3),
		MakeToken(TokenType::Identifier, "speed", 2, 4),
		MakeToken(TokenType::RParen, ")", 2, 5),
		MakeToken(TokenType::LBrace, "{", 2, 6),
		MakeToken(TokenType::RBrace, "}", 2, 7),
		MakeToken(TokenType::RBrace, "}", 3, 0),
		MakeToken(TokenType::EndOfFile, "", 3, 1),
	};
	StubBodyParserToConsume(2);

	size_t pos = 0;
	auto root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->children, SizeIs(1));
	const SyntaxNode* method = root->children[0].get();
	EXPECT_THAT(method->type, Eq(NodeType::FuncDeclStmt));
	EXPECT_THAT(method->token.lexeme, Eq("init"));
	ASSERT_THAT(method->children, SizeIs(3)); // name, speed, body
	EXPECT_THAT(method->children[0]->token.lexeme, Eq("name"));
	EXPECT_THAT(method->children[1]->token.lexeme, Eq("speed"));
	EXPECT_THAT(method->children[2]->type, Eq(NodeType::BlockStmt));
	EXPECT_THAT(pos, Eq(12u));
}

TEST_F(ClassStatementParserTest, Parse_MethodNoParams_AttachesFuncDeclNode) {
	// Class Robot { move() { } }
	TokenList tokenList = {
		MakeToken(TokenType::KwClass, "Class", 1, 0),
		MakeToken(TokenType::Identifier, "Robot", 1, 1),
		MakeToken(TokenType::LBrace, "{", 1, 2),
		MakeToken(TokenType::Identifier, "move", 2, 0),
		MakeToken(TokenType::LParen, "(", 2, 1),
		MakeToken(TokenType::RParen, ")", 2, 2),
		MakeToken(TokenType::LBrace, "{", 2, 3),
		MakeToken(TokenType::RBrace, "}", 2, 4),
		MakeToken(TokenType::RBrace, "}", 3, 0),
		MakeToken(TokenType::EndOfFile, "", 3, 1),
	};
	StubBodyParserToConsume(2);

	size_t pos = 0;
	auto root = parser.Parse(tokenList, pos);

	ASSERT_THAT(root, NotNull());
	ASSERT_THAT(root->children, SizeIs(1));
	const SyntaxNode* method = root->children[0].get();
	EXPECT_THAT(method->type, Eq(NodeType::FuncDeclStmt));
	EXPECT_THAT(method->token.lexeme, Eq("move"));
	ASSERT_THAT(method->children, SizeIs(1)); // body only
	EXPECT_THAT(method->children[0]->type, Eq(NodeType::BlockStmt));
	EXPECT_THAT(pos, Eq(9u));
}

TEST_F(ClassStatementParserTest, Parse_MissingClassName_Throws) {
	TokenList tokenList = {
		MakeToken(TokenType::KwClass, "Class", 1, 0),
		MakeToken(TokenType::LBrace, "{", 1, 1),
		MakeToken(TokenType::EndOfFile, "", 1, 2),
	};

	size_t pos = 0;
	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(ClassStatementParserTest, Parse_MissingOpenBrace_Throws) {
	TokenList tokenList = {
		MakeToken(TokenType::KwClass, "Class", 1, 0),
		MakeToken(TokenType::Identifier, "Robot", 1, 1),
		MakeToken(TokenType::EndOfFile, "", 1, 2),
	};

	size_t pos = 0;
	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

TEST_F(ClassStatementParserTest, Parse_UnexpectedEOFInBody_Throws) {
	TokenList tokenList = {
		MakeToken(TokenType::KwClass, "Class", 1, 0),
		MakeToken(TokenType::Identifier, "Robot", 1, 1),
		MakeToken(TokenType::LBrace, "{", 1, 2),
		MakeToken(TokenType::EndOfFile, "", 1, 3),
	};

	size_t pos = 0;
	EXPECT_THROW(parser.Parse(tokenList, pos), std::runtime_error);
}

#endif
