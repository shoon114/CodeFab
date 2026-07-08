#ifdef _DEBUG
#include "gmock/gmock.h"
#include "CheckerUnit.h"
#include "TestTokenHelpers.h"
#include <functional>
#include <iostream>
#include <sstream>
#include <utility>

using namespace testing;

class CheckerUnitTest : public Test {
protected:
	CheckerUnit checker;

	std::unique_ptr<SyntaxNode> MakeVarDecl(const std::string& name, std::unique_ptr<SyntaxNode> initializer = nullptr) {
		auto node = std::make_unique<VarDeclareStatementNode>();
		node->token = MakeToken(TokenType::Identifier, name, 1, 1);
		if (initializer) {
			node->children.push_back(std::move(initializer));
		}
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeIdentifier(const std::string& name) {
		auto node = std::make_unique<IdentifierNode>();
		node->token = MakeToken(TokenType::Identifier, name, 1, 1);
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeNumberLiteral(double value) {
		auto node = std::make_unique<NumberLiteralNode>();
		node->token = MakeToken(TokenType::Number, std::to_string(value), 1, 1);
		node->token.realValue = value;
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeBoolLiteral(bool value) {
		auto node = std::make_unique<BoolLiteralNode>();
		node->token = MakeToken(value ? TokenType::KwTrue : TokenType::KwFalse, value ? "true" : "false", 1, 1);
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeBinaryExpr(TokenType op, std::unique_ptr<SyntaxNode> left, std::unique_ptr<SyntaxNode> right) {
		auto node = std::make_unique<BinaryExprNode>();
		node->token = MakeToken(op, "", 1, 1);
		node->children.push_back(std::move(left));
		node->children.push_back(std::move(right));
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeUnaryExpr(TokenType op, std::unique_ptr<SyntaxNode> operand) {
		auto node = std::make_unique<UnaryExprNode>();
		node->token = MakeToken(op, "", 1, 1);
		node->children.push_back(std::move(operand));
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeAssignExpr(std::unique_ptr<SyntaxNode> target, std::unique_ptr<SyntaxNode> value) {
		auto node = std::make_unique<AssignExprNode>();
		node->token = MakeToken(TokenType::Assign, "=", 1, 1);
		node->children.push_back(std::move(target));
		node->children.push_back(std::move(value));
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeExprStmt(std::unique_ptr<SyntaxNode> expr) {
		auto node = std::make_unique<ExprStmtNode>();
		node->children.push_back(std::move(expr));
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeBlock(std::vector<std::unique_ptr<SyntaxNode>> statements) {
		auto node = std::make_unique<BlockStmtNode>();
		node->token = MakeToken(TokenType::LBrace, "{", 1, 1);
		for (auto& statement : statements) {
			node->children.push_back(std::move(statement));
		}
		return node;
	}

	// FuncDeclStmt: token.lexeme = 함수명, children = [파라미터(Identifier)..., body(BlockStmt)]
	std::unique_ptr<SyntaxNode> MakeFuncDecl(const std::string& name, std::vector<std::string> params,
		std::unique_ptr<SyntaxNode> body) {
		auto node = std::make_unique<FuncDeclStmtNode>();
		node->token = MakeToken(TokenType::Identifier, name, 1, 1);
		for (const auto& param : params) {
			node->children.push_back(MakeIdentifier(param));
		}
		node->children.push_back(std::move(body));
		return node;
	}

	// ReturnStmt: children[0] = 반환식(없으면 children 비움)
	std::unique_ptr<SyntaxNode> MakeReturnStmt(std::unique_ptr<SyntaxNode> expr = nullptr) {
		auto node = std::make_unique<ReturnStmtNode>();
		node->token = MakeToken(TokenType::KwReturn, "return", 1, 1);
		if (expr) {
			node->children.push_back(std::move(expr));
		}
		return node;
	}

	// CallExpr: token.lexeme = 호출 대상 이름, children = 인자 표현식들
	std::unique_ptr<SyntaxNode> MakeCallExpr(const std::string& name, std::vector<std::unique_ptr<SyntaxNode>> args = {}) {
		auto node = std::make_unique<CallExprNode>();
		node->token = MakeToken(TokenType::Identifier, name, 1, 1);
		for (auto& arg : args) {
			node->children.push_back(std::move(arg));
		}
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeProgram(std::vector<std::unique_ptr<SyntaxNode>> statements) {
		auto root = std::make_unique<ProgramNode>();
		for (auto& statement : statements) {
			root->children.push_back(std::move(statement));
		}
		return root;
	}

	std::string CaptureStderr(std::function<void()> action) {
		std::ostringstream output;
		auto* oldBuffer = std::cerr.rdbuf(output.rdbuf());
		action();
		std::cerr.rdbuf(oldBuffer);
		return output.str();
	}
};

TEST_F(CheckerUnitTest, Check_NullRoot_ReturnsTrue) {
	EXPECT_TRUE(checker.Check(nullptr));
}

TEST_F(CheckerUnitTest, Check_DuplicateDeclaration_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeVarDecl("a"));
	statements.push_back(MakeVarDecl("a"));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("중복 선언"));
}

TEST_F(CheckerUnitTest, Check_SelfReferenceInInitializer_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeVarDecl("a", MakeIdentifier("a")));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("자신의 초기화식"));
}

// Func add(a, b) { return a + b; }
// ret = add(3, 7);
TEST_F(CheckerUnitTest, Check_ValidFuncDeclAndCall_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> body;
	body.push_back(MakeReturnStmt(MakeIdentifier("a")));
	auto funcDecl = MakeFuncDecl("add", { "a", "b" }, MakeBlock(std::move(body)));

	std::vector<std::unique_ptr<SyntaxNode>> args;
	args.push_back(MakeIdentifier("3"));
	args.push_back(MakeIdentifier("7"));
	auto call = MakeCallExpr("add", std::move(args));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(std::move(funcDecl));
	statements.push_back(std::move(call));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}

// Func fact(n) { if(...) return 1; return n * fact(n - 1); }
// 재귀 호출은 함수 자신의 body 안에서 자기 이름을 호출할 수 있어야 한다.
TEST_F(CheckerUnitTest, Check_RecursiveCall_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> body;
	std::vector<std::unique_ptr<SyntaxNode>> args;
	args.push_back(MakeIdentifier("n"));
	body.push_back(MakeReturnStmt(MakeCallExpr("fact", std::move(args))));
	auto funcDecl = MakeFuncDecl("fact", { "n" }, MakeBlock(std::move(body)));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(std::move(funcDecl));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}

// return 5; 를 함수 밖에서 사용
TEST_F(CheckerUnitTest, Check_ReturnOutsideFunction_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeReturnStmt(MakeIdentifier("5")));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("함수 내부"));
}

// Func foo(a, a) { ... } - 파라미터 이름 중복
TEST_F(CheckerUnitTest, Check_DuplicateParamName_ReportsError) {
	auto funcDecl = MakeFuncDecl("foo", { "a", "a" }, MakeBlock({}));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(std::move(funcDecl));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("파라미터"));
	EXPECT_THAT(output, HasSubstr("중복"));
}

// var x = "hello"; x();  - 함수가 아닌 대상을 호출
TEST_F(CheckerUnitTest, Check_CallNonFunctionVariable_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeVarDecl("x"));
	statements.push_back(MakeCallExpr("x"));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("함수가 아니므로"));
}

// Func foo(a, b, c) { ... } / foo(1, 2); - 인자 개수 불일치
TEST_F(CheckerUnitTest, Check_ArgumentCountMismatch_ReportsError) {
	auto funcDecl = MakeFuncDecl("foo", { "a", "b", "c" }, MakeBlock({}));

	std::vector<std::unique_ptr<SyntaxNode>> args;
	args.push_back(MakeIdentifier("1"));
	args.push_back(MakeIdentifier("2"));
	auto call = MakeCallExpr("foo", std::move(args));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(std::move(funcDecl));
	statements.push_back(std::move(call));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("인자 개수"));
}

// 정의되지 않은 함수를 호출
TEST_F(CheckerUnitTest, Check_CallUndefinedFunction_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeCallExpr("notDefined"));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("정의되지 않은 함수"));
}

// var a = 0;
// { { a = a + 1; } }
// 중첩된 블록 안에서 참조된 a는 전역 스코프까지 2단계 떨어져 있으므로
// scopeDistance가 2로 미리 계산되어야 한다(정적 바인딩).
TEST_F(CheckerUnitTest, Check_NestedBlockVariableReference_ComputesScopeDistance) {
	auto assignTarget = MakeIdentifier("a");
	auto* assignTargetPtr = static_cast<IdentifierNode*>(assignTarget.get());

	auto readRef = MakeIdentifier("a");
	auto* readRefPtr = static_cast<IdentifierNode*>(readRef.get());

	auto addExpr = MakeBinaryExpr(TokenType::Plus, std::move(readRef), MakeNumberLiteral(1.0));
	auto assignExpr = MakeAssignExpr(std::move(assignTarget), std::move(addExpr));

	std::vector<std::unique_ptr<SyntaxNode>> innerStatements;
	innerStatements.push_back(MakeExprStmt(std::move(assignExpr)));
	auto innerBlock = MakeBlock(std::move(innerStatements));

	std::vector<std::unique_ptr<SyntaxNode>> outerStatements;
	outerStatements.push_back(std::move(innerBlock));
	auto outerBlock = MakeBlock(std::move(outerStatements));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeVarDecl("a", MakeNumberLiteral(0.0)));
	statements.push_back(std::move(outerBlock));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
	EXPECT_EQ(assignTargetPtr->scopeDistance, 2);
	EXPECT_EQ(readRefPtr->scopeDistance, 2);
}

// 선언되지 않은 변수를 참조하면 scopeDistance는 기본값(-1, 미해결)으로 남아야
// 한다. ExecutorUnit이 이 경우 기존 동적 탐색으로 폴백할 수 있게 하기 위함이다.
TEST_F(CheckerUnitTest, Check_UndeclaredVariableReference_LeavesScopeDistanceUnresolved) {
	auto ref = MakeIdentifier("missing");
	auto* refPtr = static_cast<IdentifierNode*>(ref.get());

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeExprStmt(std::move(ref)));
	auto root = MakeProgram(std::move(statements));

	checker.Check(root.get());

	EXPECT_EQ(refPtr->scopeDistance, -1);
}

// var x = 1 + 2 * 3; 처럼 리터럴로만 구성된 산술식은 체크 단계에서
// 결과값(7)으로 미리 계산되어야 한다(상수 폴딩).
TEST_F(CheckerUnitTest, Check_ConstantArithmeticExpression_FoldsAtCheckTime) {
	auto mul = MakeBinaryExpr(TokenType::Star, MakeNumberLiteral(2.0), MakeNumberLiteral(3.0));
	auto add = MakeBinaryExpr(TokenType::Plus, MakeNumberLiteral(1.0), std::move(mul));
	auto* addPtr = static_cast<BinaryExprNode*>(add.get());

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeVarDecl("x", std::move(add)));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
	ASSERT_TRUE(addPtr->isConstantFolded);
	ASSERT_TRUE(std::holds_alternative<double>(addPtr->foldedValue));
	EXPECT_DOUBLE_EQ(std::get<double>(addPtr->foldedValue), 7.0);
}

// -(5) 처럼 리터럴 단항식도 상수 폴딩 대상이다.
TEST_F(CheckerUnitTest, Check_ConstantUnaryExpression_FoldsAtCheckTime) {
	auto unary = MakeUnaryExpr(TokenType::Minus, MakeNumberLiteral(5.0));
	auto* unaryPtr = static_cast<UnaryExprNode*>(unary.get());

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeVarDecl("x", std::move(unary)));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
	ASSERT_TRUE(unaryPtr->isConstantFolded);
	ASSERT_TRUE(std::holds_alternative<double>(unaryPtr->foldedValue));
	EXPECT_DOUBLE_EQ(std::get<double>(unaryPtr->foldedValue), -5.0);
}

// true && (3 > 2) 처럼 논리/비교 연산의 조합도 리터럴로만 이루어져 있으면 폴딩된다.
TEST_F(CheckerUnitTest, Check_ConstantLogicalExpression_FoldsAtCheckTime) {
	auto comparison = MakeBinaryExpr(TokenType::Gt, MakeNumberLiteral(3.0), MakeNumberLiteral(2.0));
	auto logical = MakeBinaryExpr(TokenType::And, MakeBoolLiteral(true), std::move(comparison));
	auto* logicalPtr = static_cast<BinaryExprNode*>(logical.get());

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeVarDecl("x", std::move(logical)));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
	ASSERT_TRUE(logicalPtr->isConstantFolded);
	ASSERT_TRUE(std::holds_alternative<bool>(logicalPtr->foldedValue));
	EXPECT_TRUE(std::get<bool>(logicalPtr->foldedValue));
}

// 1 / 0 처럼 0으로 나누는 상수식은 폴딩하지 않고 그대로 남겨, 기존처럼
// 실행 단계에서 라인 정보를 포함한 런타임 에러가 나도록 한다.
TEST_F(CheckerUnitTest, Check_ConstantDivisionByZero_DoesNotFold) {
	auto div = MakeBinaryExpr(TokenType::Slash, MakeNumberLiteral(1.0), MakeNumberLiteral(0.0));
	auto* divPtr = static_cast<BinaryExprNode*>(div.get());

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeVarDecl("x", std::move(div)));
	auto root = MakeProgram(std::move(statements));

	checker.Check(root.get());

	EXPECT_FALSE(divPtr->isConstantFolded);
}

// var a = 0; var b = a + 1; 처럼 변수가 관여하는 식은 컴파일 타임에 값을
// 확정할 수 없으므로 폴딩되면 안 된다.
TEST_F(CheckerUnitTest, Check_ExpressionWithVariable_DoesNotFold) {
	auto add = MakeBinaryExpr(TokenType::Plus, MakeIdentifier("a"), MakeNumberLiteral(1.0));
	auto* addPtr = static_cast<BinaryExprNode*>(add.get());

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeVarDecl("a", MakeNumberLiteral(0.0)));
	statements.push_back(MakeVarDecl("b", std::move(add)));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
	EXPECT_FALSE(addPtr->isConstantFolded);
}
#endif
