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

	// 실제 VarDeclareParser가 만드는 모양을 따른다: token은 'var' 키워드,
	// 초기화식이 있으면 children[0]은 AssignExpr(Identifier, initializer),
	// 없으면 children[0]은 Identifier 노드 하나뿐이다.
	std::unique_ptr<SyntaxNode> MakeVarDecl(const std::string& name, std::unique_ptr<SyntaxNode> initializer = nullptr) {
		auto node = std::make_unique<VarDeclareStatementNode>();
		node->token = MakeToken(TokenType::KwVar, "var", 1, 1);
		if (initializer) {
			auto assignNode = std::make_unique<AssignExprNode>();
			assignNode->token = MakeToken(TokenType::Assign, "=", 1, 1);
			assignNode->children.push_back(MakeIdentifier(name));
			assignNode->children.push_back(std::move(initializer));
			node->children.push_back(std::move(assignNode));
		} else {
			node->children.push_back(MakeIdentifier(name));
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

	std::unique_ptr<SyntaxNode> MakeStringLiteral(const std::string& value) {
		auto node = std::make_unique<StringLiteralNode>();
		node->token = MakeToken(TokenType::String, value, 1, 1);
		return node;
	}

	// Array(sizeExpr)
	std::unique_ptr<SyntaxNode> MakeArrExpr(std::unique_ptr<SyntaxNode> sizeExpr) {
		auto node = std::make_unique<ArrExprNode>();
		node->token = MakeToken(TokenType::Identifier, "Array", 1, 1);
		node->children.push_back(std::move(sizeExpr));
		return node;
	}

	// arrayExpr[indexExpr]
	std::unique_ptr<SyntaxNode> MakeIndexExpr(std::unique_ptr<SyntaxNode> arrayExpr, std::unique_ptr<SyntaxNode> indexExpr) {
		auto node = std::make_unique<IndexExprNode>();
		node->token = MakeToken(TokenType::LBracket, "[", 1, 1);
		node->children.push_back(std::move(arrayExpr));
		node->children.push_back(std::move(indexExpr));
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

	std::unique_ptr<SyntaxNode> MakeThisExpr() {
		auto node = std::make_unique<ThisExprNode>();
		node->token = MakeToken(TokenType::KwThis, "this", 1, 1);
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeSuperExpr() {
		auto node = std::make_unique<SuperExprNode>();
		node->token = MakeToken(TokenType::KwSuper, "Super", 1, 1);
		return node;
	}

	// target.memberName
	std::unique_ptr<SyntaxNode> MakeMemberAccess(std::unique_ptr<SyntaxNode> target, const std::string& memberName) {
		auto node = std::make_unique<MemberAccessExprNode>();
		node->token = MakeToken(TokenType::Identifier, memberName, 1, 1);
		node->children.push_back(std::move(target));
		return node;
	}

	// target.methodName(args...) — 메서드 호출. CallExprNode의 token은 메서드 이름,
	// children[0]은 MemberAccessExprNode(target.methodName), 그 뒤는 인자들.
	// ExpressionParser::ParseCallExpr이 만드는 모양과 동일하다.
	std::unique_ptr<SyntaxNode> MakeMethodCallExpr(std::unique_ptr<SyntaxNode> target, const std::string& methodName,
		std::vector<std::unique_ptr<SyntaxNode>> args = {}) {
		auto node = std::make_unique<CallExprNode>();
		node->token = MakeToken(TokenType::Identifier, methodName, 1, 1);
		node->children.push_back(MakeMemberAccess(std::move(target), methodName));
		for (auto& arg : args) {
			node->children.push_back(std::move(arg));
		}
		return node;
	}

	// Class name [: parentName] { methods... } — methods는 MakeFuncDecl로 만든 노드를 재사용.
	// parentName이 빈 문자열이면 부모가 없는 클래스로 취급한다.
	std::unique_ptr<SyntaxNode> MakeClassDecl(const std::string& name, const std::string& parentName,
		std::vector<std::unique_ptr<SyntaxNode>> methods) {
		auto node = std::make_unique<ClassDeclStmtNode>();
		node->token = MakeToken(TokenType::KwClass, name, 1, 1);
		if (!parentName.empty()) {
			node->parentNameToken = MakeToken(TokenType::Identifier, parentName, 1, 1);
		}
		for (auto& method : methods) {
			node->children.push_back(std::move(method));
		}
		return node;
	}

	// import "path" alias aliasName; — KwImport/KwAlias 토큰이 아직 파서 쪽에 없어서
	// (import 문법은 미구현) token은 다른 초기 스텁 노드들과 마찬가지로 Identifier로 둔다.
	// CheckerUnit은 token.type을 보지 않고 line 정보와 children의 lexeme만 사용한다.
	std::unique_ptr<SyntaxNode> MakeImportStmt(const std::string& path, const std::string& aliasName) {
		auto node = std::make_unique<ImportStmtNode>();
		node->token = MakeToken(TokenType::Identifier, "import", 1, 1);
		node->children.push_back(MakeStringLiteral(path));
		node->children.push_back(MakeIdentifier(aliasName));
		return node;
	}

	// for (var i = 0; i < 3; i = i + 1) { <bodyStatements> }
	std::unique_ptr<SyntaxNode> MakeForStmt(std::vector<std::unique_ptr<SyntaxNode>> bodyStatements) {
		auto node = std::make_unique<ForStmtNode>();
		node->token = MakeToken(TokenType::KwFor, "for", 1, 1);
		node->children.push_back(MakeVarDecl("i", MakeNumberLiteral(0.0)));
		node->children.push_back(MakeBinaryExpr(TokenType::Lt, MakeIdentifier("i"), MakeNumberLiteral(3.0)));
		node->children.push_back(MakeAssignExpr(MakeIdentifier("i"), MakeNumberLiteral(1.0)));
		node->children.push_back(MakeBlock(std::move(bodyStatements)));
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

// Func add(a, b) {...} Func add(a, b) {...} - 같은 스코프에 같은 이름의 함수를 두 번 선언
TEST_F(CheckerUnitTest, Check_DuplicateFunctionDeclaration_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeFuncDecl("add", { "a", "b" }, MakeBlock({})));
	statements.push_back(MakeFuncDecl("add", { "a", "b" }, MakeBlock({})));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("함수 'add' 중복 선언"));
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

// Arr("hi")처럼 배열 크기 인자가 숫자 리터럴이 아니면 에러를 보고해야 한다.
TEST_F(CheckerUnitTest, Check_ArrSizeStringLiteral_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeExprStmt(MakeArrExpr(MakeStringLiteral("hi"))));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("배열 크기"));
}

// Arr(3)처럼 크기 인자가 숫자 리터럴이면 에러가 없어야 한다.
TEST_F(CheckerUnitTest, Check_ArrSizeNumberLiteral_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeExprStmt(MakeArrExpr(MakeNumberLiteral(3.0))));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}

// arr["hello"]처럼 인덱스가 숫자 리터럴이 아니면 에러를 보고해야 한다.
TEST_F(CheckerUnitTest, Check_IndexStringLiteral_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeExprStmt(MakeIndexExpr(MakeIdentifier("arr"), MakeStringLiteral("hello"))));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("인덱스"));
}

// 10[0]처럼 배열이 아닌 리터럴에 []를 사용하면 에러를 보고해야 한다.
TEST_F(CheckerUnitTest, Check_IndexOnNumberLiteral_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeExprStmt(MakeIndexExpr(MakeNumberLiteral(10.0), MakeNumberLiteral(0.0))));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("배열이 아닌"));
}

// arr[0]처럼 식별자에 []를 사용하는 경우, 변수의 실제 타입은 정적으로 알 수 없으므로
// (값 흐름 추적을 하지 않음) 에러를 보고하지 않아야 한다.
TEST_F(CheckerUnitTest, Check_IndexOnIdentifier_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeExprStmt(MakeIndexExpr(MakeIdentifier("arr"), MakeNumberLiteral(0.0))));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}

// Arr(3)[5]처럼 크기와 인덱스가 모두 상수로 확정되고 범위를 벗어나면 에러를 보고해야 한다.
TEST_F(CheckerUnitTest, Check_InlineArrIndexOutOfRange_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeExprStmt(MakeIndexExpr(MakeArrExpr(MakeNumberLiteral(3.0)), MakeNumberLiteral(5.0))));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("범위를 벗어났습니다"));
}

// Arr(3)[1]처럼 범위 안이면 에러가 없어야 한다.
TEST_F(CheckerUnitTest, Check_InlineArrIndexInRange_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeExprStmt(MakeIndexExpr(MakeArrExpr(MakeNumberLiteral(3.0)), MakeNumberLiteral(1.0))));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}

// Arr(3)[i]처럼 인덱스가 변수라 상수로 확정되지 않으면 범위 검사를 건너뛰어야 한다.
TEST_F(CheckerUnitTest, Check_InlineArrIndexWithVariableIndex_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeExprStmt(MakeIndexExpr(MakeArrExpr(MakeNumberLiteral(3.0)), MakeIdentifier("i"))));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}

// Class Robot : Robot { } - 자기 자신을 상속하면 에러를 보고해야 한다.
TEST_F(CheckerUnitTest, Check_ClassSelfInheritance_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeClassDecl("Robot", "Robot", {}));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("자기 자신을 상속"));
}

// Class Robot : NotDefined { } - 정의되지 않은 이름을 상속하면 에러를 보고해야 한다.
TEST_F(CheckerUnitTest, Check_ClassInheritFromUndefinedName_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeClassDecl("Robot", "NotDefined", {}));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("정의되지 않은 클래스"));
}

// var x = 10; Class Robot : x { } - 클래스가 아닌 변수를 상속하면 에러를 보고해야 한다.
TEST_F(CheckerUnitTest, Check_ClassInheritFromVariable_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeVarDecl("x", MakeNumberLiteral(10.0)));
	statements.push_back(MakeClassDecl("Robot", "x", {}));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("클래스가 아니므로"));
}

// Class Robot { } Class SpeedRobot : Robot { } - 정상적인 상속은 에러가 없어야 한다.
TEST_F(CheckerUnitTest, Check_ValidInheritance_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeClassDecl("Robot", "", {}));
	statements.push_back(MakeClassDecl("SpeedRobot", "Robot", {}));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}

// 같은 스코프에서 같은 이름의 클래스를 두 번 선언하면 에러를 보고해야 한다.
TEST_F(CheckerUnitTest, Check_DuplicateClassDeclaration_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeClassDecl("Robot", "", {}));
	statements.push_back(MakeClassDecl("Robot", "", {}));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("클래스 'Robot' 중복 선언"));
}

// Class Robot { move(){...} move(){...} } - 같은 클래스 내 메서드 이름 중복은 에러를 보고해야 한다.
TEST_F(CheckerUnitTest, Check_DuplicateMethodName_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> methods;
	methods.push_back(MakeFuncDecl("move", {}, MakeBlock({})));
	methods.push_back(MakeFuncDecl("move", {}, MakeBlock({})));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeClassDecl("Robot", "", std::move(methods)));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("메서드 'move'"));
	EXPECT_THAT(output, HasSubstr("중복"));
}

// print this; 를 클래스 밖(최상위)에서 사용하면 에러를 보고해야 한다.
TEST_F(CheckerUnitTest, Check_ThisOutsideClass_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeExprStmt(MakeThisExpr()));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("'this'"));
}

// Class Robot { report() { this를 사용 } } - 메서드 내부의 this는 허용되어야 한다.
TEST_F(CheckerUnitTest, Check_ThisInsideMethod_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> body;
	body.push_back(MakeExprStmt(MakeThisExpr()));

	std::vector<std::unique_ptr<SyntaxNode>> methods;
	methods.push_back(MakeFuncDecl("report", {}, MakeBlock(std::move(body))));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeClassDecl("Robot", "", std::move(methods)));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}

// Super.move(); 를 클래스 밖(최상위)에서 사용하면 에러를 보고해야 한다.
TEST_F(CheckerUnitTest, Check_SuperOutsideClass_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeExprStmt(MakeSuperExpr()));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("'Super'"));
}

// Class Robot(부모 없음) { move() { Super 사용 } } - 부모 없는 클래스의 Super 사용은 에러.
TEST_F(CheckerUnitTest, Check_SuperInClassWithoutParent_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> body;
	body.push_back(MakeExprStmt(MakeSuperExpr()));

	std::vector<std::unique_ptr<SyntaxNode>> methods;
	methods.push_back(MakeFuncDecl("move", {}, MakeBlock(std::move(body))));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeClassDecl("Robot", "", std::move(methods)));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("부모 클래스가 없는"));
}

// Class Robot {} Class SpeedRobot : Robot { move() { Super.move(); } } - 부모가 있으면 허용.
TEST_F(CheckerUnitTest, Check_SuperInClassWithParent_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> body;
	body.push_back(MakeExprStmt(MakeMemberAccess(MakeSuperExpr(), "move")));

	std::vector<std::unique_ptr<SyntaxNode>> methods;
	methods.push_back(MakeFuncDecl("move", {}, MakeBlock(std::move(body))));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeClassDecl("Robot", "", {}));
	statements.push_back(MakeClassDecl("SpeedRobot", "Robot", std::move(methods)));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}

// Class Robot { init(name) {...} } Class SpeedRobot : Robot { init(name) { Super.init(name); } }
// - 메서드 호출(Super.init(...))은 functionScopeStack에 없는 이름이라도 "정의되지 않은
// 함수"로 오판하면 안 된다. 메서드는 애초에 전역 함수 네임스페이스에 등록되지 않는다.
TEST_F(CheckerUnitTest, Check_MethodCallViaSuper_DoesNotReportUndefinedFunction) {
	std::vector<std::unique_ptr<SyntaxNode>> robotBody;
	robotBody.push_back(MakeExprStmt(MakeAssignExpr(MakeMemberAccess(MakeThisExpr(), "name"), MakeIdentifier("name"))));
	std::vector<std::unique_ptr<SyntaxNode>> robotMethods;
	robotMethods.push_back(MakeFuncDecl("init", { "name" }, MakeBlock(std::move(robotBody))));

	std::vector<std::unique_ptr<SyntaxNode>> speedRobotArgs;
	speedRobotArgs.push_back(MakeIdentifier("name"));
	std::vector<std::unique_ptr<SyntaxNode>> speedRobotBody;
	speedRobotBody.push_back(MakeExprStmt(MakeMethodCallExpr(MakeSuperExpr(), "init", std::move(speedRobotArgs))));
	std::vector<std::unique_ptr<SyntaxNode>> speedRobotMethods;
	speedRobotMethods.push_back(MakeFuncDecl("init", { "name" }, MakeBlock(std::move(speedRobotBody))));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeClassDecl("Robot", "", std::move(robotMethods)));
	statements.push_back(MakeClassDecl("SpeedRobot", "Robot", std::move(speedRobotMethods)));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_TRUE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, IsEmpty());
}

// Class Robot { } var r = Robot(); - 클래스 이름을 호출(인스턴스 생성)하는 것을
// "정의되지 않은 함수" 호출로 오판하면 안 된다. 클래스는 functionScopeStack이 아니라
// classScopeStack에 등록된다.
TEST_F(CheckerUnitTest, Check_CallClassNameAsConstructor_DoesNotReportUndefinedFunction) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeClassDecl("Robot", "", {}));
	statements.push_back(MakeVarDecl("r", MakeCallExpr("Robot")));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_TRUE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, IsEmpty());
}

// Class Robot { init() { return 5; } } - 생성자(init)에서 return을 쓰면 에러를 보고해야 한다.
TEST_F(CheckerUnitTest, Check_ReturnInsideInit_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> body;
	body.push_back(MakeReturnStmt(MakeNumberLiteral(5.0)));

	std::vector<std::unique_ptr<SyntaxNode>> methods;
	methods.push_back(MakeFuncDecl("init", {}, MakeBlock(std::move(body))));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeClassDecl("Robot", "", std::move(methods)));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("생성자(init)"));
}

// Class Robot { move() { return 5; } } - init이 아닌 일반 메서드의 return은 허용되어야 한다.
TEST_F(CheckerUnitTest, Check_ReturnInsideNonInitMethod_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> body;
	body.push_back(MakeReturnStmt(MakeNumberLiteral(5.0)));

	std::vector<std::unique_ptr<SyntaxNode>> methods;
	methods.push_back(MakeFuncDecl("move", {}, MakeBlock(std::move(body))));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeClassDecl("Robot", "", std::move(methods)));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}

// "hello".field 처럼 문자열 리터럴에 직접 멤버 접근하면 에러를 보고해야 한다.
TEST_F(CheckerUnitTest, Check_MemberAccessOnStringLiteral_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeExprStmt(MakeMemberAccess(MakeStringLiteral("hello"), "field")));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("인스턴스가 아닌"));
}

// r.field처럼 식별자에 멤버 접근하는 경우, 변수의 실제 타입은 정적으로 알 수 없으므로
// (값 흐름 추적을 하지 않음) 에러를 보고하지 않아야 한다.
TEST_F(CheckerUnitTest, Check_MemberAccessOnIdentifier_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeExprStmt(MakeMemberAccess(MakeIdentifier("r"), "field")));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}

// import "sum.txt" alias sum; — 정상적인 단일 import는 에러가 없어야 한다.
TEST_F(CheckerUnitTest, Check_ValidImport_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeImportStmt("sum.txt", "sum"));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}

// var sum = 0; import "sum.txt" alias sum; — alias 이름이 이미 선언된 변수와 충돌하면 에러.
TEST_F(CheckerUnitTest, Check_ImportAliasCollidesWithVariable_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeVarDecl("sum"));
	statements.push_back(MakeImportStmt("sum.txt", "sum"));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("별칭 'sum' 중복 선언"));
}

// import "sum.txt" alias sum; import "sum.txt" alias sum2; — alias가 달라도 같은 파일을
// 같은 스코프에서 두 번 import하면 에러.
TEST_F(CheckerUnitTest, Check_DuplicateImportInSameScope_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeImportStmt("sum.txt", "sum"));
	statements.push_back(MakeImportStmt("sum.txt", "sum2"));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("이미 import되었습니다"));
}

// import "sum.txt" alias sum; { import "sum.txt" alias sum2; }
// 상위(조상) 스코프에서 이미 import된 파일을 자손 스코프에서 다시 import하면 에러.
TEST_F(CheckerUnitTest, Check_ReimportFileAlreadyImportedInAncestorScope_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> innerStatements;
	innerStatements.push_back(MakeImportStmt("sum.txt", "sum2"));
	auto innerBlock = MakeBlock(std::move(innerStatements));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeImportStmt("sum.txt", "sum"));
	statements.push_back(std::move(innerBlock));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("이미 import되었습니다"));
}

// { import "sum.txt" alias sum; } { import "sum.txt" alias sum; }
// 형제 스코프에서 각자 독립적으로 같은 파일을 import하는 건 정상이어야 한다.
TEST_F(CheckerUnitTest, Check_ImportInSiblingScopes_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> firstBlockStatements;
	firstBlockStatements.push_back(MakeImportStmt("sum.txt", "sum"));
	auto firstBlock = MakeBlock(std::move(firstBlockStatements));

	std::vector<std::unique_ptr<SyntaxNode>> secondBlockStatements;
	secondBlockStatements.push_back(MakeImportStmt("sum.txt", "sum"));
	auto secondBlock = MakeBlock(std::move(secondBlockStatements));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(std::move(firstBlock));
	statements.push_back(std::move(secondBlock));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}

// for (...) { import "sum.txt" alias sum; } — 반복문 내부에서 import를 쓰면 에러.
TEST_F(CheckerUnitTest, Check_ImportInsideForLoop_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> bodyStatements;
	bodyStatements.push_back(MakeImportStmt("sum.txt", "sum"));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeForStmt(std::move(bodyStatements)));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_FALSE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("반복문 내부"));
}

// for (...) { ... } (import 없이 평범한 반복문 본문) — 에러가 없어야 한다.
TEST_F(CheckerUnitTest, Check_ForLoopWithoutImport_ReturnsTrue) {
	std::vector<std::unique_ptr<SyntaxNode>> bodyStatements;
	bodyStatements.push_back(MakeExprStmt(MakeIdentifier("i")));

	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeForStmt(std::move(bodyStatements)));
	auto root = MakeProgram(std::move(statements));

	EXPECT_TRUE(checker.Check(root.get()));
}
#endif
