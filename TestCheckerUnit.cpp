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
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::VarDeclareStatement;
		node->token = MakeToken(TokenType::Identifier, name, 1, 1);
		if (initializer) {
			node->children.push_back(std::move(initializer));
		}
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeIdentifier(const std::string& name) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::Identifier;
		node->token = MakeToken(TokenType::Identifier, name, 1, 1);
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeBlock(std::vector<std::unique_ptr<SyntaxNode>> statements) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::BlockStmt;
		node->token = MakeToken(TokenType::LBrace, "{", 1, 1);
		for (auto& statement : statements) {
			node->children.push_back(std::move(statement));
		}
		return node;
	}

	// FuncDeclStmt: token.lexeme = 함수명, children = [파라미터(Identifier)..., body(BlockStmt)]
	std::unique_ptr<SyntaxNode> MakeFuncDecl(const std::string& name, std::vector<std::string> params,
		std::unique_ptr<SyntaxNode> body) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::FuncDeclStmt;
		node->token = MakeToken(TokenType::Identifier, name, 1, 1);
		for (const auto& param : params) {
			node->children.push_back(MakeIdentifier(param));
		}
		node->children.push_back(std::move(body));
		return node;
	}

	// ReturnStmt: children[0] = 반환식(없으면 children 비움)
	std::unique_ptr<SyntaxNode> MakeReturnStmt(std::unique_ptr<SyntaxNode> expr = nullptr) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::ReturnStmt;
		node->token = MakeToken(TokenType::KwReturn, "return", 1, 1);
		if (expr) {
			node->children.push_back(std::move(expr));
		}
		return node;
	}

	// CallExpr: token.lexeme = 호출 대상 이름, children = 인자 표현식들
	std::unique_ptr<SyntaxNode> MakeCallExpr(const std::string& name, std::vector<std::unique_ptr<SyntaxNode>> args = {}) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::CallExpr;
		node->token = MakeToken(TokenType::Identifier, name, 1, 1);
		for (auto& arg : args) {
			node->children.push_back(std::move(arg));
		}
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeProgram(std::vector<std::unique_ptr<SyntaxNode>> statements) {
		auto root = std::make_unique<SyntaxNode>();
		root->type = NodeType::Program;
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
#endif
