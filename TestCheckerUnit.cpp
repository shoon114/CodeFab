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
		EXPECT_TRUE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("중복 선언"));
}

TEST_F(CheckerUnitTest, Check_SelfReferenceInInitializer_ReportsError) {
	std::vector<std::unique_ptr<SyntaxNode>> statements;
	statements.push_back(MakeVarDecl("a", MakeIdentifier("a")));
	auto root = MakeProgram(std::move(statements));

	std::string output = CaptureStderr([&]() {
		EXPECT_TRUE(checker.Check(root.get()));
	});

	EXPECT_THAT(output, HasSubstr("자신의 초기화식"));
}