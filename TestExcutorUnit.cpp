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

	// PDF p.79: <name> = <value> (실제 ExpressionParser/VarDeclareParser가 만드는 모양을 따른다:
	// token은 '=' 연산자, children[0]은 Identifier, children[1]이 값)
	SyntaxNode MakeAssignExpr(const std::string& name, SyntaxNode value, int line = 1) {
		SyntaxNode node;
		node.type = NodeType::AssignExpr;
		node.token.type = TokenType::Assign;
		node.token.lexeme = "=";
		node.token.line = line;
		node.children.push_back(std::make_unique<SyntaxNode>(MakeIdentifier(name, line)));
		node.children.push_back(std::make_unique<SyntaxNode>(std::move(value)));
		return node;
	}

	// var <name> = <initializer>;
	// 실제 VarDeclareParser가 만드는 모양을 따른다: token은 'var' 키워드,
	// children[0]은 AssignExpr(Identifier, initializer).
	SyntaxNode MakeVarDeclStmt(const std::string& name, SyntaxNode initializer, int line = 1) {
		SyntaxNode node;
		node.type = NodeType::VarDeclareStatement;
		node.token.type = TokenType::KwVar;
		node.token.lexeme = "var";
		node.token.line = line;
		node.children.push_back(std::make_unique<SyntaxNode>(MakeAssignExpr(name, std::move(initializer), line)));
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

	SyntaxNode MakeExprStmt(SyntaxNode expression, int line = 1) {
		SyntaxNode node;
		node.type = NodeType::ExprStmt;
		node.token.line = line;
		node.children.push_back(std::make_unique<SyntaxNode>(std::move(expression)));
		return node;
	}

	// -a, !a 등 단항식 헬퍼
	SyntaxNode MakeUnaryExpr(TokenType op, SyntaxNode operand, int line = 1) {
		SyntaxNode node;
		node.type = NodeType::UnaryExpr;
		node.token.type = op;
		node.token.line = line;
		node.children.push_back(std::make_unique<SyntaxNode>(std::move(operand)));
		return node;
	}

	// true / false 리터럴. 값은 token.lexeme에 "true"/"false" 텍스트로 저장한다.
	SyntaxNode MakeBoolLiteral(bool value, int line = 1) {
		SyntaxNode node;
		node.type = NodeType::BoolLiteral;
		node.token.lexeme = value ? "true" : "false";
		node.token.line = line;
		return node;
	}

	// PDF p.80: for (init; condition; increment) { print "#"; } 에서 사용되는 헬퍼.
	SyntaxNode MakeStringLiteral(const std::string& value, int line = 1) {
		SyntaxNode node;
		node.type = NodeType::StringLiteral;
		node.token.type = TokenType::String;
		node.token.lexeme = value;
		node.token.line = line;
		return node;
	}

	// for (<init>; <condition>; <increment>) <body>
	// children 순서: init, condition, increment, body (PDF p.80 다이어그램 순서를 따른다)
	SyntaxNode MakeForStmt(SyntaxNode init, SyntaxNode condition, SyntaxNode increment, SyntaxNode body, int line = 1) {
		SyntaxNode node;
		node.type = NodeType::ForStmt;
		node.token.line = line;
		node.children.push_back(std::make_unique<SyntaxNode>(std::move(init)));
		node.children.push_back(std::make_unique<SyntaxNode>(std::move(condition)));
		node.children.push_back(std::make_unique<SyntaxNode>(std::move(increment)));
		node.children.push_back(std::make_unique<SyntaxNode>(std::move(body)));
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

// PDF p.79: var a = 10; if (a > 0) a = 3 + 7 * 5; 실행 시 조건이 참이므로
// 블록 없는 단일 문(ExprStmt -> AssignExpr)이 실행되어 a가 38(3 + 7*5)이 되어야 한다.
// NOTE: ExecutorUnit이 아직 ExprStmt/AssignExpr를 지원하지 않아 현재는 실패(Red)한다.
TEST(ExecutorUnitTest, Execute_IfWithoutBlockConditionTrue_AssignsExpressionResult) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(10)),
		MakeIfStmt(
			MakeBinaryExpr(TokenType::Gt, MakeIdentifier("a"), MakeNumberLiteral(0)),
			MakeExprStmt(
				MakeAssignExpr("a",
					MakeBinaryExpr(TokenType::Plus,
						MakeNumberLiteral(3),
						MakeBinaryExpr(TokenType::Star, MakeNumberLiteral(7), MakeNumberLiteral(5))))
			)
		),
		MakePrintStmt(MakeIdentifier("a"))
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, HasSubstr("38"));
}

// PDF p.79: 조건이 거짓이면(a <= 0) 대입이 일어나지 않아 a는 원래 값(10)을 유지해야 한다.
TEST(ExecutorUnitTest, Execute_IfWithoutBlockConditionFalse_KeepsOriginalValue) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(-1)),
		MakeIfStmt(
			MakeBinaryExpr(TokenType::Gt, MakeIdentifier("a"), MakeNumberLiteral(0)),
			MakeExprStmt(
				MakeAssignExpr("a",
					MakeBinaryExpr(TokenType::Plus,
						MakeNumberLiteral(3),
						MakeBinaryExpr(TokenType::Star, MakeNumberLiteral(7), MakeNumberLiteral(5))))
			)
		),
		MakePrintStmt(MakeIdentifier("a"))
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, HasSubstr("-1"));
}

// PDF p.80: for (var i = 0; i < 3; i = i + 1) { print "#"; } 실행 시
// i = 0, 1, 2 세 번 반복하며 "#"을 출력하고, i = 3이 되면 조건이 거짓이 되어 종료해야 한다.
// NOTE: ExecutorUnit이 아직 ForStmt/StringLiteral을 지원하지 않아 현재는 실패(Red)한다.
TEST(ExecutorUnitTest, Execute_ForLoop_PrintsHashThreeTimes) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeForStmt(
			/*init*/      MakeVarDeclStmt("i", MakeNumberLiteral(0)),
			/*condition*/ MakeBinaryExpr(TokenType::Lt, MakeIdentifier("i"), MakeNumberLiteral(3)),
			/*increment*/ MakeAssignExpr("i",
				MakeBinaryExpr(TokenType::Plus, MakeIdentifier("i"), MakeNumberLiteral(1))),
			/*body*/      MakeBlockStmt(MakePrintStmt(MakeStringLiteral("#")))
		)
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, Eq("###"));
}

// PDF p.80: 초기 조건이 이미 거짓이면(i = 3) 본문이 한 번도 실행되지 않아야 한다.
TEST(ExecutorUnitTest, Execute_ForLoop_ConditionInitiallyFalse_NeverRunsBody) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeForStmt(
			/*init*/      MakeVarDeclStmt("i", MakeNumberLiteral(3)),
			/*condition*/ MakeBinaryExpr(TokenType::Lt, MakeIdentifier("i"), MakeNumberLiteral(3)),
			/*increment*/ MakeAssignExpr("i",
				MakeBinaryExpr(TokenType::Plus, MakeIdentifier("i"), MakeNumberLiteral(1))),
			/*body*/      MakeBlockStmt(MakePrintStmt(MakeStringLiteral("#")))
		)
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, IsEmpty());
}

// PDF p.81: var a = 5; for (var i = 0; i < 2; i = i + 1) { if (a > 3) a = a - 1; }
// i = 0(a: 5->4), i = 1(a: 4->3), i = 2에서 조건이 거짓이 되어 종료 -> 최종 a = 3.
// ForStmt/IfStmt/ExprStmt/AssignExpr가 모두 이미 구현되어 있으므로 통과(Green)해야 한다.
TEST(ExecutorUnitTest, Execute_ForLoopWithNestedIf_DecrementsAWhileGreaterThanThree) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(5)),
		MakeForStmt(
			/*init*/      MakeVarDeclStmt("i", MakeNumberLiteral(0)),
			/*condition*/ MakeBinaryExpr(TokenType::Lt, MakeIdentifier("i"), MakeNumberLiteral(2)),
			/*increment*/ MakeAssignExpr("i",
				MakeBinaryExpr(TokenType::Plus, MakeIdentifier("i"), MakeNumberLiteral(1))),
			/*body*/      MakeBlockStmt(
				MakeIfStmt(
					MakeBinaryExpr(TokenType::Gt, MakeIdentifier("a"), MakeNumberLiteral(3)),
					MakeExprStmt(
						MakeAssignExpr("a",
							MakeBinaryExpr(TokenType::Minus, MakeIdentifier("a"), MakeNumberLiteral(1)))
					)
				)
			)
		),
		MakePrintStmt(MakeIdentifier("a"))
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, Eq("3"));
}

// PDF p.81 변형: a가 이미 3 이하이면 if 조건이 항상 거짓이라 for 루프 동안 a가 변하지 않아야 한다.
TEST(ExecutorUnitTest, Execute_ForLoopWithNestedIf_ConditionAlwaysFalse_KeepsOriginalValue) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(2)),
		MakeForStmt(
			/*init*/      MakeVarDeclStmt("i", MakeNumberLiteral(0)),
			/*condition*/ MakeBinaryExpr(TokenType::Lt, MakeIdentifier("i"), MakeNumberLiteral(2)),
			/*increment*/ MakeAssignExpr("i",
				MakeBinaryExpr(TokenType::Plus, MakeIdentifier("i"), MakeNumberLiteral(1))),
			/*body*/      MakeBlockStmt(
				MakeIfStmt(
					MakeBinaryExpr(TokenType::Gt, MakeIdentifier("a"), MakeNumberLiteral(3)),
					MakeExprStmt(
						MakeAssignExpr("a",
							MakeBinaryExpr(TokenType::Minus, MakeIdentifier("a"), MakeNumberLiteral(1)))
					)
				)
			)
		),
		MakePrintStmt(MakeIdentifier("a"))
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, Eq("2"));
}

// PDF p.83: var a = 10; { var b = 20; print a + b; } var c = 20;
// 블록 내부에서 바깥 스코프의 a와 블록 지역 변수 b를 모두 참조할 수 있어야 한다 (a+b=30).
TEST(ExecutorUnitTest, Execute_BlockScope_AccessesOuterVariable_PrintsSum) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(10)),
		MakeBlockStmt(
			MakeVarDeclStmt("b", MakeNumberLiteral(20)),
			MakePrintStmt(MakeBinaryExpr(TokenType::Plus, MakeIdentifier("a"), MakeIdentifier("b")))
		),
		MakeVarDeclStmt("c", MakeNumberLiteral(20))
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, Eq("30"));
}

// PDF p.83: "{ } 블록이 끝나면 지역 변수는 사라진다"를 검증한다.
// NOTE: 현재 ExecutorUnit은 스코프가 없는 flat map이라 블록이 끝나도 b가 남아있어
// 이 테스트는 실패(Red)한다. 스코프 구현 후 통과해야 한다.
TEST(ExecutorUnitTest, Execute_BlockScope_LocalVariableDestroyedAfterBlockEnds) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(10)),
		MakeBlockStmt(
			MakeVarDeclStmt("b", MakeNumberLiteral(20))
		),
		MakePrintStmt(MakeIdentifier("b"))
	);

	EXPECT_THROW(executor.Execute(program), std::runtime_error);
}

// PDF p.84: var ga = 3; { var a = 2; { var a = 7; { print a; print ga; } print a; } }
// 안쪽 스코프에 a가 없으면 바깥으로 탐색하며(a: C->B, ga: C->B->A->Global), 가장 가까운 스코프의 값을 사용한다.
// 다만 이 예제의 모든 print는 a=7로 재선언된 B 스코프가 닫히기 전에 실행되므로,
// 현재의 flat map 구현으로도 우연히 "737"이 출력되어 이 테스트만으로는 Red를 보장하지 못한다
// (스코프 복원 여부는 아래 Execute_NestedBlockShadowing_RestoresOuterValueAfterInnerBlockEnds에서 검증한다).
TEST(ExecutorUnitTest, Execute_NestedBlockShadowing_PrintsInnerAndGlobalValues) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("ga", MakeNumberLiteral(3)),
		MakeBlockStmt( // Scope A
			MakeVarDeclStmt("a", MakeNumberLiteral(2)),
			MakeBlockStmt( // Scope B
				MakeVarDeclStmt("a", MakeNumberLiteral(7)),
				MakeBlockStmt( // Scope C
					MakePrintStmt(MakeIdentifier("a")),
					MakePrintStmt(MakeIdentifier("ga"))
				),
				MakePrintStmt(MakeIdentifier("a"))
			)
		)
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, Eq("737"));
}

// PDF p.84 개념의 확장: 안쪽 블록에서 같은 이름(a)을 재선언(shadowing)해도,
// 블록이 끝나면 바깥 스코프의 원래 값(2)으로 복원되어야 한다.
// NOTE: 현재 flat map 구현은 재선언 시 덮어쓰기만 하고 복원하지 않아 실패(Red)한다.
TEST(ExecutorUnitTest, Execute_NestedBlockShadowing_RestoresOuterValueAfterInnerBlockEnds) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(2)),
		MakeBlockStmt(
			MakeVarDeclStmt("a", MakeNumberLiteral(7)),
			MakePrintStmt(MakeIdentifier("a"))
		),
		MakePrintStmt(MakeIdentifier("a"))
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, Eq("72"));
}

// 문자열 변수에 대한 블록 스코프 검증:
// var x = "global"; { var x = "inner"; print x; } print x;
// 블록 안에서는 섀도잉된 "inner"가, 블록 밖에서는 원래의 "global"이 출력되어야 한다.
// NOTE: 현재 ExecutorUnit은 변수를 double로만 저장하므로(Evaluate가 StringLiteral을
// 지원하지 않음) 이 테스트는 실패(Red)한다. 문자열 값을 지원하는 Value 타입 도입 후 통과해야 한다.
TEST(ExecutorUnitTest, Execute_BlockScope_StringVariableShadowing_RestoresOuterValueAfterBlockEnds) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("x", MakeStringLiteral("global")),
		MakeBlockStmt(
			MakeVarDeclStmt("x", MakeStringLiteral("inner")),
			MakePrintStmt(MakeIdentifier("x"))
		),
		MakePrintStmt(MakeIdentifier("x"))
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, Eq("innerglobal"));
}

// 블록 안에서의 대입은 재선언이 아니라 바깥 스코프 변수를 수정해야 한다:
// var count = 0; { count = count + 1; } print count;
// AssignExpr는 var가 아니므로 새 스코프에 선언하지 않고, ResolveVariable로 체인을 타고 올라가
// Global의 count를 직접 수정해야 한다.
TEST(ExecutorUnitTest, Execute_AssignInsideBlock_MutatesOuterVariable_NotShadow) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("count", MakeNumberLiteral(0)),
		MakeBlockStmt(
			MakeExprStmt(
				MakeAssignExpr("count",
					MakeBinaryExpr(TokenType::Plus, MakeIdentifier("count"), MakeNumberLiteral(1)))
			)
		),
		MakePrintStmt(MakeIdentifier("count"))
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, Eq("1"));
}

// 중첩 블록에서 서로 다른 스코프에 있는 문자열 변수들을 + 로 연결한다:
// var outer = "A"; { var inner = "B"; { print outer + inner; } } -> expect "AB"
// NOTE: 현재 BinaryExpr의 Plus는 AsNumber로 항상 숫자 변환을 강제하므로,
// 문자열 피연산자에 대해서는 "Expected a number" 예외가 던져져 실패(Red)한다.
// 문자열 연결(+)을 지원하려면 BinaryExpr의 Plus 케이스에서 두 피연산자가 모두
// 문자열일 때 std::string 연결을 수행하도록 분기를 추가해야 한다.
TEST(ExecutorUnitTest, Execute_NestedBlockScope_ConcatenatesStringsFromDifferentScopes) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("outer", MakeStringLiteral("A")),
		MakeBlockStmt(
			MakeVarDeclStmt("inner", MakeStringLiteral("B")),
			MakeBlockStmt(
				MakePrintStmt(
					MakeBinaryExpr(TokenType::Plus, MakeIdentifier("outer"), MakeIdentifier("inner"))
				)
			)
		)
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, Eq("AB"));
}

// var a = 10; var b = -a; print b; -> expect -10
// UnaryExpr(Minus)가 숫자를 음수화하여 다른 변수에 대입될 수 있어야 한다.
TEST(ExecutorUnitTest, Execute_UnaryMinus_NegatesVariableValue) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(10)),
		MakeVarDeclStmt("b", MakeUnaryExpr(TokenType::Minus, MakeIdentifier("a"))),
		MakePrintStmt(MakeIdentifier("b"))
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, Eq("-10"));
}

// var a = true; var b = !a; print b; -> expect "false"
// NOTE: Evaluate가 아직 NodeType::BoolLiteral을 지원하지 않아 이 테스트는 실패(Red)한다.
// BoolLiteral 평가(token.lexeme == "true" 여부를 bool로 반환)를 Evaluate에 추가해야 통과한다.
TEST(ExecutorUnitTest, Execute_UnaryNot_NegatesBooleanVariable) {
	ExecutorUnit executor;
	SyntaxNode program = MakeProgram(
		MakeVarDeclStmt("a", MakeBoolLiteral(true)),
		MakeVarDeclStmt("b", MakeUnaryExpr(TokenType::Not, MakeIdentifier("a"))),
		MakePrintStmt(MakeIdentifier("b"))
	);

	testing::internal::CaptureStdout();
	executor.Execute(program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, Eq("false"));
}
#endif
