#ifdef _DEBUG
#include "gmock/gmock.h"
#include "ExecutorUnit.h"
#include "SyntaxNode.h"
#include <memory>
#include <vector>

using namespace testing;

namespace {

	// 테스트에서 파서 없이 SyntaxNode 트리를 직접 구성하기 위한 헬퍼
	// (excuterunit-idempotent-umbrella.md의 children 배치 규칙을 따른다)
	SyntaxNode MakeNumberLiteral(double value, int line = 1) {
		SyntaxNode node;
		node.type = NodeType::NumberLiteral;
		node.token.type = TokenType::Number;
		node.token.realValue = value;
		node.token.line = line;
		return node;
	}

	SyntaxNode MakeBinaryExpr(TokenType op, SyntaxNode left, SyntaxNode right, int line = 1) {
		SyntaxNode node;
		node.type = NodeType::BinaryExpr;
		node.token.type = op;
		node.token.line = line;
		node.children.push_back(std::make_unique<SyntaxNode>(std::move(left)));
		node.children.push_back(std::make_unique<SyntaxNode>(std::move(right)));
		return node;
	}

	SyntaxNode MakePrintStmt(SyntaxNode expression, int line = 1) {
		SyntaxNode node;
		node.type = NodeType::PrintStmt;
		node.token.line = line;
		node.children.push_back(std::make_unique<SyntaxNode>(std::move(expression)));
		return node;
	}

	// SyntaxNode는 unique_ptr children을 가져 move-only이므로,
	// std::vector<SyntaxNode> 브레이스 초기화(복사 필요) 대신 가변 인자 템플릿으로 구성한다.
	template <typename... Stmts>
	SyntaxNode MakeProgram(Stmts&&... statements) {
		SyntaxNode node;
		node.type = NodeType::Program;
		(node.children.push_back(std::make_unique<SyntaxNode>(std::forward<Stmts>(statements))), ...);
		return node;
	}

	// PDF p.78: 아직 ExecutorUnit이 지원하지 않는 노드(VarDeclStmt/Identifier/IfStmt/BlockStmt)용 헬퍼.
	// 트리 형태는 p.78 다이어그램(변수 a 선언 -> if (a > 5) { print 3 + 2; })을 따른다.
	SyntaxNode MakeIdentifier(const std::string& name, int line = 1) {
		SyntaxNode node;
		node.type = NodeType::Identifier;
		node.token.type = TokenType::Identifier;
		node.token.lexeme = name;
		node.token.line = line;
		return node;
	}

	// var <name> = <initializer>;
	SyntaxNode MakeVarDeclStmt(const std::string& name, SyntaxNode initializer, int line = 1) {
		SyntaxNode node;
		node.type = NodeType::VarDeclStmt;
		node.token.type = TokenType::Identifier;
		node.token.lexeme = name;
		node.token.line = line;
		node.children.push_back(std::make_unique<SyntaxNode>(std::move(initializer)));
		return node;
	}

	template <typename... Stmts>
	SyntaxNode MakeBlockStmt(Stmts&&... statements) {
		SyntaxNode node;
		node.type = NodeType::BlockStmt;
		(node.children.push_back(std::make_unique<SyntaxNode>(std::forward<Stmts>(statements))), ...);
		return node;
	}

	// if (<condition>) <thenBranch>
	SyntaxNode MakeIfStmt(SyntaxNode condition, SyntaxNode thenBranch, int line = 1) {
		SyntaxNode node;
		node.type = NodeType::IfStmt;
		node.token.line = line;
		node.children.push_back(std::make_unique<SyntaxNode>(std::move(condition)));
		node.children.push_back(std::make_unique<SyntaxNode>(std::move(thenBranch)));
		return node;
	}

}

TEST(ExecutorUnitTest, Execute_EmptyProgram_DoesNotCrash) {
	ExecutorUnit executor;
	SyntaxNode program;
	program.type = NodeType::Program;

	executor.Execute(program);
}

// PDF p.77: print 3 + 2; 실행 시 stdout에 5가 출력되어야 한다.
TEST(ExecutorUnitTest, Execute_PrintBinaryAddition_PrintsFive) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakePrintStmt(MakeBinaryExpr(TokenType::Plus, MakeNumberLiteral(3), MakeNumberLiteral(2)))
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, HasSubstr("5"));
}

// PDF p.78: var a = 10; if (a > 5) { print 3 + 2; } 실행 시 조건이 참이므로
// thenBranch가 실행되어 stdout에 5가 출력되어야 한다.
// NOTE: ExecutorUnit이 아직 VarDeclStmt/Identifier/IfStmt/BlockStmt를 지원하지 않아
// 현재는 실패(Red)한다. 해당 기능 구현 후 통과해야 한다.
TEST(ExecutorUnitTest, Execute_IfConditionTrue_ExecutesThenBranch_PrintsFive) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(10)),
		MakeIfStmt(
			MakeBinaryExpr(TokenType::Gt, MakeIdentifier("a"), MakeNumberLiteral(5)),
			MakeBlockStmt(
				MakePrintStmt(MakeBinaryExpr(TokenType::Plus, MakeNumberLiteral(3), MakeNumberLiteral(2)))
			)
		)
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, HasSubstr("5"));
}

// PDF p.78: 조건이 거짓이면(a <= 5) thenBranch가 실행되지 않아 아무 것도 출력되지 않아야 한다.
TEST(ExecutorUnitTest, Execute_IfConditionFalse_SkipsThenBranch_PrintsNothing) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(1)),
		MakeIfStmt(
			MakeBinaryExpr(TokenType::Gt, MakeIdentifier("a"), MakeNumberLiteral(5)),
			MakeBlockStmt(
				MakePrintStmt(MakeBinaryExpr(TokenType::Plus, MakeNumberLiteral(3), MakeNumberLiteral(2)))
			)
		)
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, IsEmpty());
}
#endif
