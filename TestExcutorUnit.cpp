#ifdef _DEBUG
#include "gmock/gmock.h"
#include "ExecutorUnit.h"
#include "SyntaxNode.h"
#include <memory>
#include <vector>

using namespace testing;

namespace {

	// 테스트에서 파서 없이 SyntaxNode 트리를 직접 구성하기 위한 헬퍼.
	// SyntaxNode는 추상 베이스이므로 항상 구체 타입을 make_unique로 생성해 반환한다.
	std::unique_ptr<SyntaxNode> MakeNumberLiteral(double value, int line = 1) {
		auto node = std::make_unique<NumberLiteralNode>();
		node->token.type = TokenType::Number;
		node->token.realValue = value;
		node->token.line = line;
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeBinaryExpr(TokenType op, std::unique_ptr<SyntaxNode> left, std::unique_ptr<SyntaxNode> right, int line = 1) {
		auto node = std::make_unique<BinaryExprNode>();
		node->token.type = op;
		node->token.line = line;
		node->children.push_back(std::move(left));
		node->children.push_back(std::move(right));
		return node;
	}

	std::unique_ptr<SyntaxNode> MakePrintStmt(std::unique_ptr<SyntaxNode> expression, int line = 1) {
		auto node = std::make_unique<PrintStmtNode>();
		node->token.line = line;
		node->children.push_back(std::move(expression));
		return node;
	}

	template <typename... Stmts>
	std::unique_ptr<SyntaxNode> MakeProgram(Stmts&&... statements) {
		auto node = std::make_unique<ProgramNode>();
		(node->children.push_back(std::forward<Stmts>(statements)), ...);
		return node;
	}

	// PDF p.78: 아직 ExecutorUnit이 지원하지 않는 노드(VarDeclStmt/Identifier/IfStmt/BlockStmt)용 헬퍼.
	// 트리 형태는 p.78 다이어그램(변수 a 선언 -> if (a > 5) { print 3 + 2; })을 따른다.
	std::unique_ptr<SyntaxNode> MakeIdentifier(const std::string& name, int line = 1) {
		auto node = std::make_unique<IdentifierNode>();
		node->token.type = TokenType::Identifier;
		node->token.lexeme = name;
		node->token.line = line;
		return node;
	}

	// PDF p.79: <name> = <value> (실제 ExpressionParser가 만드는 모양을 따른다:
	// token은 '=' 연산자, children[0]은 Identifier, children[1]이 값)
	std::unique_ptr<SyntaxNode> MakeAssignExpr(const std::string& name, std::unique_ptr<SyntaxNode> value, int line = 1) {
		auto node = std::make_unique<AssignExprNode>();
		node->token.type = TokenType::Assign;
		node->token.lexeme = "=";
		node->token.line = line;
		node->children.push_back(MakeIdentifier(name, line));
		node->children.push_back(std::move(value));
		return node;
	}

	// var <name> = <initializer>;
	// 실제 VarDeclareParser가 만드는 모양을 따른다: token은 'var' 키워드,
	// children[0]은 AssignExpr(Identifier, initializer).
	std::unique_ptr<SyntaxNode> MakeVarDeclStmt(const std::string& name, std::unique_ptr<SyntaxNode> initializer, int line = 1) {
		auto node = std::make_unique<VarDeclareStatementNode>();
		node->token.type = TokenType::KwVar;
		node->token.lexeme = "var";
		node->token.line = line;
		node->children.push_back(MakeAssignExpr(name, std::move(initializer), line));
		return node;
	}

	template <typename... Stmts>
	std::unique_ptr<SyntaxNode> MakeBlockStmt(Stmts&&... statements) {
		auto node = std::make_unique<BlockStmtNode>();
		(node->children.push_back(std::forward<Stmts>(statements)), ...);
		return node;
	}

	// if (<condition>) <thenBranch>
	std::unique_ptr<SyntaxNode> MakeIfStmt(std::unique_ptr<SyntaxNode> condition, std::unique_ptr<SyntaxNode> thenBranch, int line = 1) {
		auto node = std::make_unique<IfStmtNode>();
		node->token.line = line;
		node->children.push_back(std::move(condition));
		node->children.push_back(std::move(thenBranch));
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeExprStmt(std::unique_ptr<SyntaxNode> expression, int line = 1) {
		auto node = std::make_unique<ExprStmtNode>();
		node->token.line = line;
		node->children.push_back(std::move(expression));
		return node;
	}

	// -a, !a 등 단항식 헬퍼
	std::unique_ptr<SyntaxNode> MakeUnaryExpr(TokenType op, std::unique_ptr<SyntaxNode> operand, int line = 1) {
		auto node = std::make_unique<UnaryExprNode>();
		node->token.type = op;
		node->token.line = line;
		node->children.push_back(std::move(operand));
		return node;
	}

	// true / false 리터럴. 값은 token.lexeme에 "true"/"false" 텍스트로 저장한다.
	std::unique_ptr<SyntaxNode> MakeBoolLiteral(bool value, int line = 1) {
		auto node = std::make_unique<BoolLiteralNode>();
		node->token.lexeme = value ? "true" : "false";
		node->token.line = line;
		return node;
	}

	// PDF p.80: for (init; condition; increment) { print "#"; } 에서 사용되는 헬퍼.
	std::unique_ptr<SyntaxNode> MakeStringLiteral(const std::string& value, int line = 1) {
		auto node = std::make_unique<StringLiteralNode>();
		node->token.type = TokenType::String;
		node->token.lexeme = value;
		node->token.line = line;
		return node;
	}

	// for (<init>; <condition>; <increment>) <body>
	// children 순서: init, condition, increment, body (PDF p.80 다이어그램 순서를 따른다)
	std::unique_ptr<SyntaxNode> MakeForStmt(std::unique_ptr<SyntaxNode> init, std::unique_ptr<SyntaxNode> condition,
		std::unique_ptr<SyntaxNode> increment, std::unique_ptr<SyntaxNode> body, int line = 1) {
		auto node = std::make_unique<ForStmtNode>();
		node->token.line = line;
		node->children.push_back(std::move(init));
		node->children.push_back(std::move(condition));
		node->children.push_back(std::move(increment));
		node->children.push_back(std::move(body));
		return node;
	}

	// Arr(<size>) 배열 생성식. token은 'Arr' 식별자 토큰, children = [size식].
	std::unique_ptr<SyntaxNode> MakeArrExpr(std::unique_ptr<SyntaxNode> sizeExpr, int line = 1) {
		auto node = std::make_unique<ArrExprNode>();
		node->token.type = TokenType::Identifier;
		node->token.lexeme = "Arr";
		node->token.line = line;
		node->children.push_back(std::move(sizeExpr));
		return node;
	}

	// <array식>[<index식>]. token은 여는 '[' 토큰, children = [array식, index식].
	std::unique_ptr<SyntaxNode> MakeIndexExpr(std::unique_ptr<SyntaxNode> arrayExpr, std::unique_ptr<SyntaxNode> indexExpr, int line = 1) {
		auto node = std::make_unique<IndexExprNode>();
		node->token.type = TokenType::LBracket;
		node->token.line = line;
		node->children.push_back(std::move(arrayExpr));
		node->children.push_back(std::move(indexExpr));
		return node;
	}

	// <indexExpr> = <value> (예: arr[0] = 10). AssignExpr의 대입 대상이 일반 Identifier가
	// 아니라 IndexExpr인 경우를 위한 헬퍼. MakeAssignExpr와 달리 대입 대상 노드를 그대로 받는다.
	std::unique_ptr<SyntaxNode> MakeIndexAssignExpr(std::unique_ptr<SyntaxNode> indexExpr, std::unique_ptr<SyntaxNode> value, int line = 1) {
		auto node = std::make_unique<AssignExprNode>();
		node->token.type = TokenType::Assign;
		node->token.lexeme = "=";
		node->token.line = line;
		node->children.push_back(std::move(indexExpr));
		node->children.push_back(std::move(value));
		return node;
	}

}

// 모든 ExecutorUnitTest 케이스가 공유하는 실행/검증 로직을 헬퍼로 모아
// 테스트 본문에서는 트리 구성과 기대값만 남긴다.
class ExecutorUnitTest : public ::testing::Test {
protected:
	ExecutorUnit executor;

	std::string RunAndCapture(const SyntaxNode& program) {
		testing::internal::CaptureStdout();
		executor.Execute(program);
		return testing::internal::GetCapturedStdout();
	}

	void ExpectRuntimeError(const SyntaxNode& program, const std::string& expectedSubstr) {
		try {
			executor.Execute(program);
			FAIL() << "std::runtime_error가 던져지길 기대했습니다.";
		} catch (const std::runtime_error& e) {
			EXPECT_THAT(std::string(e.what()), HasSubstr(expectedSubstr));
		}
	}
};

TEST_F(ExecutorUnitTest, Execute_EmptyProgram_DoesNotCrash) {
	ProgramNode program;

	executor.Execute(program);
}

// PDF p.77: print 3 + 2; 실행 시 stdout에 5가 출력되어야 한다.
TEST_F(ExecutorUnitTest, Execute_PrintBinaryAddition_PrintsFive) {
	auto program = MakeProgram(
		MakePrintStmt(MakeBinaryExpr(TokenType::Plus, MakeNumberLiteral(3), MakeNumberLiteral(2)))
	);

	EXPECT_THAT(RunAndCapture(*program), HasSubstr("5"));
}

// PDF p.78: var a = 10; if (a > 5) { print 3 + 2; } 실행 시 조건이 참이므로
// thenBranch가 실행되어 stdout에 5가 출력되어야 한다.
TEST_F(ExecutorUnitTest, Execute_IfConditionTrue_ExecutesThenBranch_PrintsFive) {
	auto program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(10)),
		MakeIfStmt(
			MakeBinaryExpr(TokenType::Gt, MakeIdentifier("a"), MakeNumberLiteral(5)),
			MakeBlockStmt(
				MakePrintStmt(MakeBinaryExpr(TokenType::Plus, MakeNumberLiteral(3), MakeNumberLiteral(2)))
			)
		)
	);

	EXPECT_THAT(RunAndCapture(*program), HasSubstr("5"));
}

// PDF p.78: 조건이 거짓이면(a <= 5) thenBranch가 실행되지 않아 아무 것도 출력되지 않아야 한다.
TEST_F(ExecutorUnitTest, Execute_IfConditionFalse_SkipsThenBranch_PrintsNothing) {
	auto program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(1)),
		MakeIfStmt(
			MakeBinaryExpr(TokenType::Gt, MakeIdentifier("a"), MakeNumberLiteral(5)),
			MakeBlockStmt(
				MakePrintStmt(MakeBinaryExpr(TokenType::Plus, MakeNumberLiteral(3), MakeNumberLiteral(2)))
			)
		)
	);

	EXPECT_THAT(RunAndCapture(*program), IsEmpty());
}

// PDF p.79: var a = 10; if (a > 0) a = 3 + 7 * 5; 실행 시 조건이 참이므로
// 블록 없는 단일 문(ExprStmt -> AssignExpr)이 실행되어 a가 38(3 + 7*5)이 되어야 한다.
TEST_F(ExecutorUnitTest, Execute_IfWithoutBlockConditionTrue_AssignsExpressionResult) {
	auto program = MakeProgram(
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

	EXPECT_THAT(RunAndCapture(*program), HasSubstr("38"));
}

// PDF p.79: 조건이 거짓이면(a <= 0) 대입이 일어나지 않아 a는 원래 값(10)을 유지해야 한다.
TEST_F(ExecutorUnitTest, Execute_IfWithoutBlockConditionFalse_KeepsOriginalValue) {
	auto program = MakeProgram(
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

	EXPECT_THAT(RunAndCapture(*program), HasSubstr("-1"));
}

// PDF p.80: for (var i = 0; i < 3; i = i + 1) { print "#"; } 실행 시
// i = 0, 1, 2 세 번 반복하며 "#"을 출력하고, i = 3이 되면 조건이 거짓이 되어 종료해야 한다.
TEST_F(ExecutorUnitTest, Execute_ForLoop_PrintsHashThreeTimes) {
	auto program = MakeProgram(
		MakeForStmt(
			/*init*/      MakeVarDeclStmt("i", MakeNumberLiteral(0)),
			/*condition*/ MakeBinaryExpr(TokenType::Lt, MakeIdentifier("i"), MakeNumberLiteral(3)),
			/*increment*/ MakeAssignExpr("i",
				MakeBinaryExpr(TokenType::Plus, MakeIdentifier("i"), MakeNumberLiteral(1))),
			/*body*/      MakeBlockStmt(MakePrintStmt(MakeStringLiteral("#")))
		)
	);

	EXPECT_THAT(RunAndCapture(*program), Eq("###"));
}

// PDF p.80: 초기 조건이 이미 거짓이면(i = 3) 본문이 한 번도 실행되지 않아야 한다.
TEST_F(ExecutorUnitTest, Execute_ForLoop_ConditionInitiallyFalse_NeverRunsBody) {
	auto program = MakeProgram(
		MakeForStmt(
			/*init*/      MakeVarDeclStmt("i", MakeNumberLiteral(3)),
			/*condition*/ MakeBinaryExpr(TokenType::Lt, MakeIdentifier("i"), MakeNumberLiteral(3)),
			/*increment*/ MakeAssignExpr("i",
				MakeBinaryExpr(TokenType::Plus, MakeIdentifier("i"), MakeNumberLiteral(1))),
			/*body*/      MakeBlockStmt(MakePrintStmt(MakeStringLiteral("#")))
		)
	);

	EXPECT_THAT(RunAndCapture(*program), IsEmpty());
}

// PDF p.81: var a = 5; for (var i = 0; i < 2; i = i + 1) { if (a > 3) a = a - 1; }
// i = 0(a: 5->4), i = 1(a: 4->3), i = 2에서 조건이 거짓이 되어 종료 -> 최종 a = 3.
// ForStmt/IfStmt/ExprStmt/AssignExpr가 모두 이미 구현되어 있으므로 통과(Green)해야 한다.
TEST_F(ExecutorUnitTest, Execute_ForLoopWithNestedIf_DecrementsAWhileGreaterThanThree) {
	auto program = MakeProgram(
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

	EXPECT_THAT(RunAndCapture(*program), Eq("3"));
}

// PDF p.81 변형: a가 이미 3 이하이면 if 조건이 항상 거짓이라 for 루프 동안 a가 변하지 않아야 한다.
TEST_F(ExecutorUnitTest, Execute_ForLoopWithNestedIf_ConditionAlwaysFalse_KeepsOriginalValue) {
	auto program = MakeProgram(
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

	EXPECT_THAT(RunAndCapture(*program), Eq("2"));
}

// PDF p.83: var a = 10; { var b = 20; print a + b; } var c = 20;
// 블록 내부에서 바깥 스코프의 a와 블록 지역 변수 b를 모두 참조할 수 있어야 한다 (a+b=30).
TEST_F(ExecutorUnitTest, Execute_BlockScope_AccessesOuterVariable_PrintsSum) {
	auto program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(10)),
		MakeBlockStmt(
			MakeVarDeclStmt("b", MakeNumberLiteral(20)),
			MakePrintStmt(MakeBinaryExpr(TokenType::Plus, MakeIdentifier("a"), MakeIdentifier("b")))
		),
		MakeVarDeclStmt("c", MakeNumberLiteral(20))
	);

	EXPECT_THAT(RunAndCapture(*program), Eq("30"));
}

// PDF p.83: "{ } 블록이 끝나면 지역 변수는 사라진다"를 검증한다.
TEST_F(ExecutorUnitTest, Execute_BlockScope_LocalVariableDestroyedAfterBlockEnds) {
	auto program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(10)),
		MakeBlockStmt(
			MakeVarDeclStmt("b", MakeNumberLiteral(20))
		),
		MakePrintStmt(MakeIdentifier("b"))
	);

	EXPECT_THROW(executor.Execute(*program), std::runtime_error);
}

// PDF p.84: var ga = 3; { var a = 2; { var a = 7; { print a; print ga; } print a; } }
// 안쪽 스코프에 a가 없으면 바깥으로 탐색하며(a: C->B, ga: C->B->A->Global), 가장 가까운 스코프의 값을 사용한다.
TEST_F(ExecutorUnitTest, Execute_NestedBlockShadowing_PrintsInnerAndGlobalValues) {
	auto program = MakeProgram(
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

	EXPECT_THAT(RunAndCapture(*program), Eq("737"));
}

// PDF p.84 개념의 확장: 안쪽 블록에서 같은 이름(a)을 재선언(shadowing)해도,
// 블록이 끝나면 바깥 스코프의 원래 값(2)으로 복원되어야 한다.
TEST_F(ExecutorUnitTest, Execute_NestedBlockShadowing_RestoresOuterValueAfterInnerBlockEnds) {
	auto program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(2)),
		MakeBlockStmt(
			MakeVarDeclStmt("a", MakeNumberLiteral(7)),
			MakePrintStmt(MakeIdentifier("a"))
		),
		MakePrintStmt(MakeIdentifier("a"))
	);

	EXPECT_THAT(RunAndCapture(*program), Eq("72"));
}

// 문자열 변수에 대한 블록 스코프 검증:
// var x = "global"; { var x = "inner"; print x; } print x;
// 블록 안에서는 섀도잉된 "inner"가, 블록 밖에서는 원래의 "global"이 출력되어야 한다.
TEST_F(ExecutorUnitTest, Execute_BlockScope_StringVariableShadowing_RestoresOuterValueAfterBlockEnds) {
	auto program = MakeProgram(
		MakeVarDeclStmt("x", MakeStringLiteral("global")),
		MakeBlockStmt(
			MakeVarDeclStmt("x", MakeStringLiteral("inner")),
			MakePrintStmt(MakeIdentifier("x"))
		),
		MakePrintStmt(MakeIdentifier("x"))
	);

	EXPECT_THAT(RunAndCapture(*program), Eq("innerglobal"));
}

// 블록 안에서의 대입은 재선언이 아니라 바깥 스코프 변수를 수정해야 한다:
// var count = 0; { count = count + 1; } print count;
// AssignExpr는 var가 아니므로 새 스코프에 선언하지 않고, ResolveVariable로 체인을 타고 올라가
// Global의 count를 직접 수정해야 한다.
TEST_F(ExecutorUnitTest, Execute_AssignInsideBlock_MutatesOuterVariable_NotShadow) {
	auto program = MakeProgram(
		MakeVarDeclStmt("count", MakeNumberLiteral(0)),
		MakeBlockStmt(
			MakeExprStmt(
				MakeAssignExpr("count",
					MakeBinaryExpr(TokenType::Plus, MakeIdentifier("count"), MakeNumberLiteral(1)))
			)
		),
		MakePrintStmt(MakeIdentifier("count"))
	);

	EXPECT_THAT(RunAndCapture(*program), Eq("1"));
}

// 중첩 블록에서 서로 다른 스코프에 있는 문자열 변수들을 + 로 연결한다:
// var outer = "A"; { var inner = "B"; { print outer + inner; } } -> expect "AB"
TEST_F(ExecutorUnitTest, Execute_NestedBlockScope_ConcatenatesStringsFromDifferentScopes) {
	auto program = MakeProgram(
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

	EXPECT_THAT(RunAndCapture(*program), Eq("AB"));
}

// var a = 10; var b = -a; print b; -> expect -10
// UnaryExpr(Minus)가 숫자를 음수화하여 다른 변수에 대입될 수 있어야 한다.
TEST_F(ExecutorUnitTest, Execute_UnaryMinus_NegatesVariableValue) {
	auto program = MakeProgram(
		MakeVarDeclStmt("a", MakeNumberLiteral(10)),
		MakeVarDeclStmt("b", MakeUnaryExpr(TokenType::Minus, MakeIdentifier("a"))),
		MakePrintStmt(MakeIdentifier("b"))
	);

	EXPECT_THAT(RunAndCapture(*program), Eq("-10"));
}

// var a = true; var b = !a; print b; -> expect "false"
TEST_F(ExecutorUnitTest, Execute_UnaryNot_NegatesBooleanVariable) {
	auto program = MakeProgram(
		MakeVarDeclStmt("a", MakeBoolLiteral(true)),
		MakeVarDeclStmt("b", MakeUnaryExpr(TokenType::Not, MakeIdentifier("a"))),
		MakePrintStmt(MakeIdentifier("b"))
	);

	EXPECT_THAT(RunAndCapture(*program), Eq("false"));
}

// PDF p.86: 3 - "hello"처럼 피연산자 타입이 맞지 않을 때 "Operand must be a number at line N"
// 형식으로 런타임 오류를 보고해야 한다.
TEST_F(ExecutorUnitTest, Execute_TypeMismatch_BooleanMultiplication_ThrowsWithLineMessage) {
	auto program = MakeProgram(
		MakeExprStmt(
			MakeBinaryExpr(TokenType::Star, MakeBoolLiteral(true), MakeBoolLiteral(false), 1)
		)
	);

	ExpectRuntimeError(*program, "Operand must be a number at line 1");
}

// PDF p.86: 3 - "hello" -> "Operand must be a number at line 1"
TEST_F(ExecutorUnitTest, Execute_TypeMismatch_NumberMinusString_ThrowsWithLineMessage) {
	auto program = MakeProgram(
		MakeExprStmt(
			MakeBinaryExpr(TokenType::Minus, MakeNumberLiteral(3), MakeStringLiteral("hello"), 1)
		)
	);

	ExpectRuntimeError(*program, "Operand must be a number at line 1");
}

// print -"FabCoding"; -> 단항 마이너스의 피연산자가 문자열이면
// AsNumber 타입 가드에 의해 "Operand must be a number at line 1"이 발생해야 한다.
TEST_F(ExecutorUnitTest, Execute_TypeMismatch_UnaryMinusOnString_ThrowsWithLineMessage) {
	auto program = MakeProgram(
		MakePrintStmt(
			MakeUnaryExpr(TokenType::Minus, MakeStringLiteral("FabCoding", 1), 1)
		)
	);

	ExpectRuntimeError(*program, "Operand must be a number at line 1");
}

// PDF p.87: 선언 없이 x = 5; -> "Undefined variable 'x' at line 1"
TEST_F(ExecutorUnitTest, Execute_UndefinedVariableAssignment_ThrowsWithLineMessage) {
	auto program = MakeProgram(
		MakeExprStmt(
			MakeAssignExpr("x", MakeNumberLiteral(5), 1)
		)
	);

	ExpectRuntimeError(*program, "Undefined variable 'x' at line 1");
}

// PDF p.88: a = 3 / 0; -> "Division by zero at line 1"
// AssignExpr는 값을 먼저 평가하므로, a가 선언되어 있지 않아도 나눗셈 오류가 먼저 발생한다.
TEST_F(ExecutorUnitTest, Execute_DivisionByZero_ThrowsWithLineMessage) {
	auto program = MakeProgram(
		MakeExprStmt(
			MakeAssignExpr("a",
				MakeBinaryExpr(TokenType::Slash, MakeNumberLiteral(3), MakeNumberLiteral(0), 1),
				1)
		)
	);

	ExpectRuntimeError(*program, "Division by zero at line 1");
}

// [정적 바인딩] Test Double 검증:
// var a = "outer"; { var a = "inner"; { print <a>; } }
// CheckerUnit이 정상 계산했다면 이 위치의 scopeDistance는 1(Scope A의 "inner")이어야
// 하지만, 여기서는 CheckerUnit을 거치지 않고 scopeDistance=2("inner"를 건너뛰고
// Global로 직행)를 test double로 직접 스텁한다.
// 만약 ExecutorUnit이 scopeDistance를 무시하고 이름으로 스코프를 거슬러 올라가며
// 동적 탐색했다면 가장 가까운 "inner"가 출력되었을 것이다. 결과가 "outer"라는 것은
// 스텁해 둔 거리값으로 해당 스코프에 즉시 접근했다는(탐색을 건너뛰었다는) 증거다.
TEST_F(ExecutorUnitTest, Execute_IdentifierWithStubbedScopeDistance_AccessesScopeDirectlyWithoutDynamicSearch) {
	auto stubbedRef = std::make_unique<IdentifierNode>();
	stubbedRef->token.type = TokenType::Identifier;
	stubbedRef->token.lexeme = "a";
	stubbedRef->scopeDistance = 2;

	auto program = MakeProgram(
		MakeVarDeclStmt("a", MakeStringLiteral("outer")),
		MakeBlockStmt( // Scope A: "a"를 섀도잉("inner") - 이름 기반 탐색이라면 여기서 먼저 발견됨
			MakeVarDeclStmt("a", MakeStringLiteral("inner")),
			MakeBlockStmt( // Scope B(가장 안쪽)
				MakePrintStmt(std::move(stubbedRef))
			)
		)
	);

	testing::internal::CaptureStdout();
	executor.Execute(*program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, Eq("outer"));
}

// [상수 폴딩] Test Double 검증:
// isConstantFolded=true, foldedValue=42로 미리 채워진 BinaryExprNode를 직접 구성하되,
// children에는 "poison"(정의되지 않은 변수를 참조하는 Identifier)을 심어둔다.
// 만약 ExecutorUnit이 폴딩 플래그를 확인하지 않고 매번 자식을 재계산했다면
// for 루프 5번 반복 중 첫 회차에 "Undefined variable" 런타임 예외가 발생했을 것이다.
// 예외 없이 5번 모두 42가 출력된다는 것은 재계산 횟수가 N(5)회에서 0회로
// 줄었다는(자식이 단 한 번도 평가되지 않았다는) 증거다.
TEST_F(ExecutorUnitTest, Execute_ConstantFoldedBinaryExpr_SkipsRecomputingPoisonedChildrenEachIteration) {
	auto foldedExpr = std::make_unique<BinaryExprNode>();
	foldedExpr->token.type = TokenType::Plus;
	foldedExpr->isConstantFolded = true;
	foldedExpr->foldedValue = 42.0;
	foldedExpr->children.push_back(MakeIdentifier("__poison_left__"));
	foldedExpr->children.push_back(MakeIdentifier("__poison_right__"));

	auto program = MakeProgram(
		MakeForStmt(
			/*init*/      MakeVarDeclStmt("i", MakeNumberLiteral(0)),
			/*condition*/ MakeBinaryExpr(TokenType::Lt, MakeIdentifier("i"), MakeNumberLiteral(5)),
			/*increment*/ MakeAssignExpr("i",
				MakeBinaryExpr(TokenType::Plus, MakeIdentifier("i"), MakeNumberLiteral(1))),
			/*body*/      MakeBlockStmt(MakePrintStmt(std::move(foldedExpr)))
		)
	);

	testing::internal::CaptureStdout();
	executor.Execute(*program);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, Eq("4242424242"));
}

// ---- 배열(Array) 관련 테스트 ----

// var arr = Arr(3); print arr[0];
// Arr(size)로 생성된 배열의 초기화되지 않은 칸은 std::monostate(null)이어야 한다.
TEST_F(ExecutorUnitTest, Execute_ArrExpr_UninitializedElement_PrintsNull) {
	auto program = MakeProgram(
		MakeVarDeclStmt("arr", MakeArrExpr(MakeNumberLiteral(3))),
		MakePrintStmt(MakeIndexExpr(MakeIdentifier("arr"), MakeNumberLiteral(0)))
	);

	EXPECT_THAT(RunAndCapture(*program), Eq("null"));
}

// var arr = Arr(3); arr[0] = 42; print arr[0];
TEST_F(ExecutorUnitTest, Execute_IndexAssign_SetsElement_PrintsAssignedValue) {
	auto program = MakeProgram(
		MakeVarDeclStmt("arr", MakeArrExpr(MakeNumberLiteral(3))),
		MakeExprStmt(
			MakeIndexAssignExpr(MakeIndexExpr(MakeIdentifier("arr"), MakeNumberLiteral(0)), MakeNumberLiteral(42))
		),
		MakePrintStmt(MakeIndexExpr(MakeIdentifier("arr"), MakeNumberLiteral(0)))
	);

	EXPECT_THAT(RunAndCapture(*program), Eq("42"));
}

// var arr = Arr(3); arr[0]=1; arr[1]=2; arr[2]=3; print arr[0]+arr[1]+arr[2];
TEST_F(ExecutorUnitTest, Execute_IndexAssign_MultipleElements_SumsToSix) {
	auto program = MakeProgram(
		MakeVarDeclStmt("arr", MakeArrExpr(MakeNumberLiteral(3))),
		MakeExprStmt(MakeIndexAssignExpr(MakeIndexExpr(MakeIdentifier("arr"), MakeNumberLiteral(0)), MakeNumberLiteral(1))),
		MakeExprStmt(MakeIndexAssignExpr(MakeIndexExpr(MakeIdentifier("arr"), MakeNumberLiteral(1)), MakeNumberLiteral(2))),
		MakeExprStmt(MakeIndexAssignExpr(MakeIndexExpr(MakeIdentifier("arr"), MakeNumberLiteral(2)), MakeNumberLiteral(3))),
		MakePrintStmt(
			MakeBinaryExpr(TokenType::Plus,
				MakeBinaryExpr(TokenType::Plus,
					MakeIndexExpr(MakeIdentifier("arr"), MakeNumberLiteral(0)),
					MakeIndexExpr(MakeIdentifier("arr"), MakeNumberLiteral(1))),
				MakeIndexExpr(MakeIdentifier("arr"), MakeNumberLiteral(2)))
		)
	);

	EXPECT_THAT(RunAndCapture(*program), Eq("6"));
}

// var a = Arr(2); var b = a; b[0] = 9; print a[0];
// 배열은 shared_ptr<ArrayObject>로 다뤄지므로 변수 대입은 참조를 복사한다(값 복사가 아님).
// b를 통해 수정한 원소가 a를 통해서도 보여야 한다.
TEST_F(ExecutorUnitTest, Execute_ArrayAssignedToAnotherVariable_SharesUnderlyingStorage) {
	auto program = MakeProgram(
		MakeVarDeclStmt("a", MakeArrExpr(MakeNumberLiteral(2))),
		MakeVarDeclStmt("b", MakeIdentifier("a")),
		MakeExprStmt(MakeIndexAssignExpr(MakeIndexExpr(MakeIdentifier("b"), MakeNumberLiteral(0)), MakeNumberLiteral(9))),
		MakePrintStmt(MakeIndexExpr(MakeIdentifier("a"), MakeNumberLiteral(0)))
	);

	EXPECT_THAT(RunAndCapture(*program), Eq("9"));
}

// var arr = Arr(-1); -> [1번째줄] 배열 크기는 0 이상의 정수여야 한다는 런타임 오류.
TEST_F(ExecutorUnitTest, Execute_ArrSize_Negative_ThrowsRuntimeError) {
	auto program = MakeProgram(
		MakeVarDeclStmt("arr", MakeArrExpr(MakeUnaryExpr(TokenType::Minus, MakeNumberLiteral(1)), 1), 1)
	);

	ExpectRuntimeError(*program, "Expected a non-negative integer for array size at line 1");
}

// var arr = Arr(2.5); -> 정수가 아닌 크기는 런타임 오류.
TEST_F(ExecutorUnitTest, Execute_ArrSize_NonInteger_ThrowsRuntimeError) {
	auto program = MakeProgram(
		MakeVarDeclStmt("arr", MakeArrExpr(MakeNumberLiteral(2.5), 1), 1)
	);

	ExpectRuntimeError(*program, "Expected a non-negative integer for array size at line 1");
}

// var arr = Arr("3"); -> 숫자가 아닌 크기는 런타임 오류.
TEST_F(ExecutorUnitTest, Execute_ArrSize_NonNumber_ThrowsRuntimeError) {
	auto program = MakeProgram(
		MakeVarDeclStmt("arr", MakeArrExpr(MakeStringLiteral("3"), 1), 1)
	);

	ExpectRuntimeError(*program, "Expected a number for array size at line 1");
}

// var x = 5; print x[0]; -> 배열이 아닌 값에 인덱싱하면 런타임 오류.
TEST_F(ExecutorUnitTest, Execute_IndexingNonArrayValue_ThrowsRuntimeError) {
	auto program = MakeProgram(
		MakeVarDeclStmt("x", MakeNumberLiteral(5)),
		MakePrintStmt(MakeIndexExpr(MakeIdentifier("x"), MakeNumberLiteral(0), 1), 1)
	);

	ExpectRuntimeError(*program, "Expected an array at line 1");
}

// var arr = Arr(3); print arr["a"]; -> 인덱스가 숫자가 아니면 런타임 오류.
TEST_F(ExecutorUnitTest, Execute_IndexWithNonNumberValue_ThrowsRuntimeError) {
	auto program = MakeProgram(
		MakeVarDeclStmt("arr", MakeArrExpr(MakeNumberLiteral(3))),
		MakePrintStmt(MakeIndexExpr(MakeIdentifier("arr"), MakeStringLiteral("a"), 1), 1)
	);

	ExpectRuntimeError(*program, "Expected a number as array index at line 1");
}

// var arr = Arr(3); print arr[-1]; -> 음수 인덱스는 범위 초과 런타임 오류.
TEST_F(ExecutorUnitTest, Execute_IndexOutOfRange_Negative_ThrowsRuntimeError) {
	auto program = MakeProgram(
		MakeVarDeclStmt("arr", MakeArrExpr(MakeNumberLiteral(3))),
		MakePrintStmt(MakeIndexExpr(MakeIdentifier("arr"), MakeUnaryExpr(TokenType::Minus, MakeNumberLiteral(1)), 1), 1)
	);

	ExpectRuntimeError(*program, "Array index out of range at line 1");
}

// var arr = Arr(3); print arr[3]; -> 마지막 유효 인덱스는 size-1(=2)이므로 3은 범위 초과.
TEST_F(ExecutorUnitTest, Execute_IndexOutOfRange_EqualsSize_ThrowsRuntimeError) {
	auto program = MakeProgram(
		MakeVarDeclStmt("arr", MakeArrExpr(MakeNumberLiteral(3))),
		MakePrintStmt(MakeIndexExpr(MakeIdentifier("arr"), MakeNumberLiteral(3), 1), 1)
	);

	ExpectRuntimeError(*program, "Array index out of range at line 1");
}

// var arr = Arr(3); print arr[1.5]; -> 정수가 아닌 인덱스도 범위 초과와 동일한 오류로 처리된다.
TEST_F(ExecutorUnitTest, Execute_IndexNonInteger_ThrowsRuntimeError) {
	auto program = MakeProgram(
		MakeVarDeclStmt("arr", MakeArrExpr(MakeNumberLiteral(3))),
		MakePrintStmt(MakeIndexExpr(MakeIdentifier("arr"), MakeNumberLiteral(1.5), 1), 1)
	);

	ExpectRuntimeError(*program, "Array index out of range at line 1");
}
#endif
