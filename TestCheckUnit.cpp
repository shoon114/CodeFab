#ifdef _DEBUG
#include "gmock/gmock.h"
#include <memory>

using namespace testing;

namespace {

	// CheckerUnit::Visit는 raw pointer 트리(SyntaxNode::children이 unique_ptr)를 순회하므로,
	// 테스트에서도 파서 없이 unique_ptr 기반 트리를 직접 조립한다.
	std::unique_ptr<SyntaxNode> MakeNumberLiteral(const std::string& lexeme, int line = 1) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::NumberLiteral;
		node->token.type = TokenType::Number;
		node->token.lexeme = lexeme;
		node->token.line = line;
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeIdentifier(const std::string& name, int line = 1) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::Identifier;
		node->token.type = TokenType::Identifier;
		node->token.lexeme = name;
		node->token.line = line;
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeBinary(std::unique_ptr<SyntaxNode> left, TokenType op, std::unique_ptr<SyntaxNode> right, int line = 1) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::BinaryExpr;
		node->token.type = op;
		node->token.line = line;
		node->children.push_back(std::move(left));
		node->children.push_back(std::move(right));
		return node;
	}

	// var <name> = <initializer>; (initializer가 없으면 nullptr을 전달)
	std::unique_ptr<SyntaxNode> MakeVarDecl(const std::string& name, std::unique_ptr<SyntaxNode> initializer, int line = 1) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::VarDeclareStatement;
		node->token.type = TokenType::Identifier;
		node->token.lexeme = name;
		node->token.line = line;
		if (initializer) {
			node->children.push_back(std::move(initializer));
		}
		return node;
	}

	template <typename... Stmts>
	std::unique_ptr<SyntaxNode> MakeBlock(Stmts... statements) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::BlockStmt;
		(node->children.push_back(std::move(statements)), ...);
		return node;
	}

	template <typename... Stmts>
	std::unique_ptr<SyntaxNode> MakeProgram(Stmts... statements) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::Program;
		(node->children.push_back(std::move(statements)), ...);
		return node;
	}

} // namespace

TEST(CheckerUnitTest, Check_NullRoot_ReturnsTrue) {
	CheckerUnit checker;

	EXPECT_TRUE(checker.Check(nullptr));
}

TEST(CheckerUnitTest, Check_EmptyProgram_ReturnsTrue) {
	CheckerUnit checker;
	auto root = MakeProgram();

	EXPECT_TRUE(checker.Check(root.get()));
}

TEST(CheckerUnitTest, Check_NoDuplicateDeclaration_PrintsNoError) {
	CheckerUnit checker;
	auto root = MakeProgram(
		MakeBlock(
			MakeVarDecl("a", MakeNumberLiteral("1")),
			MakeVarDecl("b", MakeNumberLiteral("2"))));

	testing::internal::CaptureStderr();
	checker.Check(root.get());
	std::string output = testing::internal::GetCapturedStderr();

	EXPECT_THAT(output, IsEmpty());
}

TEST(CheckerUnitTest, Check_DuplicateDeclarationInSameBlock_PrintsError) {
	CheckerUnit checker;
	auto root = MakeProgram(
		MakeBlock(
			MakeVarDecl("a", MakeNumberLiteral("1")),
			MakeVarDecl("a", MakeNumberLiteral("2"))));

	testing::internal::CaptureStderr();
	checker.Check(root.get());
	std::string output = testing::internal::GetCapturedStderr();

	EXPECT_THAT(output, HasSubstr("중복 선언"));
}

TEST(CheckerUnitTest, Check_DuplicateDeclarationDirectlyInGlobalScope_PrintsError) {
	// CheckerUnit은 Program 자체를 Global 스코프로 취급하므로,
	// 블록 없이 최상위에 선언해도 중복 검사 대상이다.
	CheckerUnit checker;
	auto root = MakeProgram(
		MakeVarDecl("a", MakeNumberLiteral("1")),
		MakeVarDecl("a", MakeNumberLiteral("2")));

	testing::internal::CaptureStderr();
	checker.Check(root.get());
	std::string output = testing::internal::GetCapturedStderr();

	EXPECT_THAT(output, HasSubstr("중복 선언"));
}

TEST(CheckerUnitTest, Check_SameNameInNestedScope_PrintsNoError) {
	// { var a = 1; { var a = 2; } }
	CheckerUnit checker;
	auto root = MakeProgram(
		MakeBlock(
			MakeVarDecl("a", MakeNumberLiteral("1")),
			MakeBlock(
				MakeVarDecl("a", MakeNumberLiteral("2")))));

	testing::internal::CaptureStderr();
	checker.Check(root.get());
	std::string output = testing::internal::GetCapturedStderr();

	EXPECT_THAT(output, IsEmpty());
}

TEST(CheckerUnitTest, Check_SelfReferenceInInitializer_PrintsError) {
	// var a = a + 1;
	CheckerUnit checker;
	auto root = MakeProgram(
		MakeBlock(
			MakeVarDecl("a", MakeBinary(MakeIdentifier("a"), TokenType::Plus, MakeNumberLiteral("1")))));

	testing::internal::CaptureStderr();
	checker.Check(root.get());
	std::string output = testing::internal::GetCapturedStderr();

	EXPECT_THAT(output, HasSubstr("자신의 초기화식"));
}

TEST(CheckerUnitTest, Check_ReferencingOtherVariable_PrintsNoError) {
	// var a = 10; var b = a + 1;
	CheckerUnit checker;
	auto root = MakeProgram(
		MakeBlock(
			MakeVarDecl("a", MakeNumberLiteral("10")),
			MakeVarDecl("b", MakeBinary(MakeIdentifier("a"), TokenType::Plus, MakeNumberLiteral("1")))));

	testing::internal::CaptureStderr();
	checker.Check(root.get());
	std::string output = testing::internal::GetCapturedStderr();

	EXPECT_THAT(output, IsEmpty());
}

TEST(CheckerUnitTest, Check_VarDeclWithoutInitializer_PrintsNoError) {
	CheckerUnit checker;
	auto root = MakeProgram(
		MakeBlock(
			MakeVarDecl("a", nullptr)));

	testing::internal::CaptureStderr();
	checker.Check(root.get());
	std::string output = testing::internal::GetCapturedStderr();

	EXPECT_THAT(output, IsEmpty());
}
#endif
