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
#endif
