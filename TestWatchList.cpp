#ifdef _DEBUG
#include "gmock/gmock.h"
#include "WatchList.h"
#include "ExecutionObserver.h"
#include "ExecutorUnit.h"
#include "SyntaxNode.h"
#include <memory>
#include <string>

using namespace testing;

// ExecutionObserver는 순수가상이라 뭔가는 구현해야 한다. 특정 statement(marker)에
// 진입하는 순간(그 스코프가 아직 팝되기 전) watches 출력을 캡처하기 위한 옵저버.
// fixture 상태와 무관하게 독립적으로 쓰이므로 fixture 밖에 둔다.
class CaptureAtMarkerObserver : public ExecutionObserver {
public:
	CaptureAtMarkerObserver(const SyntaxNode* marker, const WatchList& watchList,
		const ExecutorUnit& executor, std::string& output)
		: marker(marker), watchList(watchList), executor(executor), output(output) {}

	void OnStmtEnter(const SyntaxNode& node) override {
		if (&node == marker) {
			testing::internal::CaptureStdout();
			watchList.PrintAll(executor);
			output = testing::internal::GetCapturedStdout();
		}
	}
	void OnStmtExit(const SyntaxNode&) override {}

private:
	const SyntaxNode* marker;
	const WatchList& watchList;
	const ExecutorUnit& executor;
	std::string& output;
};

// TestExcutorUnit.cpp과 동일한 목적(파서 없이 트리를 직접 구성)의 헬퍼지만,
// 다른 .cpp의 fixture와 공유되지 않으므로 여기서 필요한 만큼만 멤버 함수로 다시 둔다.
class WatchListTest : public ::testing::Test {
protected:
	std::unique_ptr<SyntaxNode> MakeIdentifier(const std::string& name, int line = 1) {
		auto node = std::make_unique<IdentifierNode>();
		node->token.type = TokenType::Identifier;
		node->token.lexeme = name;
		node->token.line = line;
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeNumberLiteral(double value, int line = 1) {
		auto node = std::make_unique<NumberLiteralNode>();
		node->token.type = TokenType::Number;
		node->token.realValue = value;
		node->token.line = line;
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeBoolLiteral(bool value, int line = 1) {
		auto node = std::make_unique<BoolLiteralNode>();
		node->token.lexeme = value ? "true" : "false";
		node->token.line = line;
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeStringLiteral(const std::string& value, int line = 1) {
		auto node = std::make_unique<StringLiteralNode>();
		node->token.type = TokenType::String;
		node->token.lexeme = value;
		node->token.line = line;
		return node;
	}

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
	std::unique_ptr<SyntaxNode> MakeVarDeclStmt(const std::string& name, std::unique_ptr<SyntaxNode> initializer, int line = 1) {
		auto node = std::make_unique<VarDeclareStatementNode>();
		node->token.type = TokenType::KwVar;
		node->token.lexeme = "var";
		node->token.line = line;
		node->children.push_back(MakeAssignExpr(name, std::move(initializer), line));
		return node;
	}

	std::unique_ptr<SyntaxNode> MakeExprStmt(std::unique_ptr<SyntaxNode> expression, int line = 1) {
		auto node = std::make_unique<ExprStmtNode>();
		node->token.line = line;
		node->children.push_back(std::move(expression));
		return node;
	}

	template <typename... Stmts>
	std::unique_ptr<SyntaxNode> MakeBlockStmt(Stmts&&... statements) {
		auto node = std::make_unique<BlockStmtNode>();
		(node->children.push_back(std::forward<Stmts>(statements)), ...);
		return node;
	}

	template <typename... Stmts>
	std::unique_ptr<SyntaxNode> MakeProgram(Stmts&&... statements) {
		auto node = std::make_unique<ProgramNode>();
		(node->children.push_back(std::forward<Stmts>(statements)), ...);
		return node;
	}

	// watchList.PrintAll(executor)의 출력을 캡처해 문자열로 돌려주는 공용 헬퍼.
	std::string CapturePrintAll(const WatchList& watchList, const ExecutorUnit& executor) {
		testing::internal::CaptureStdout();
		watchList.PrintAll(executor);
		return testing::internal::GetCapturedStdout();
	}
};

TEST_F(WatchListTest, Add_NewName_IsInList) {
	WatchList watchList;
	watchList.Add("a");

	EXPECT_TRUE(watchList.Contains("a"));
}

TEST_F(WatchListTest, Remove_WatchedName_IsNoLongerInList) {
	WatchList watchList;
	watchList.Add("a");

	watchList.Remove("a");//unWatch

	EXPECT_FALSE(watchList.Contains("a"));
}

// var a = 4; 실행 후(전역 스코프에 남아있는 상태) watches를 조회하면
// "[LOCAL] a = 4 (number)"가 출력되어야 한다.
TEST_F(WatchListTest, PrintAll_GlobalNumberVariable_PrintsGlobalLabelAndNumberType) {
	auto program = MakeProgram(MakeVarDeclStmt("a", MakeNumberLiteral(4)));

	ExecutorUnit executor;
	executor.Execute(*program);

	WatchList watchList;
	watchList.Add("a");

	EXPECT_THAT(CapturePrintAll(watchList, executor), HasSubstr("[LOCAL] a = 4 (number)"));
}

// var b = true; 실행 후 watches를 조회하면 "[LOCAL] b = true (Boolean)"가 출력되어야 한다.
TEST_F(WatchListTest, PrintAll_LocalBooleanVariable_PrintsLocalLabelAndBooleanType) {
	auto program = MakeProgram(MakeVarDeclStmt("b", MakeBoolLiteral(true)));

	ExecutorUnit executor;
	executor.Execute(*program);

	WatchList watchList;
	watchList.Add("b");

	EXPECT_THAT(CapturePrintAll(watchList, executor), HasSubstr("[LOCAL] b = true (Boolean)"));
}

// var c = "hello"; 실행 후 watches를 조회하면 "[LOCAL] c = \"hello\" (string)"가
// 출력되어야 한다 (문자열 값은 큰따옴표로 감싸서 출력).
TEST_F(WatchListTest, PrintAll_LocalStringVariable_PrintsLocalLabelAndStringType) {
	auto program = MakeProgram(MakeVarDeclStmt("c", MakeStringLiteral("hello")));

	ExecutorUnit executor;
	executor.Execute(*program);

	WatchList watchList;
	watchList.Add("c");

	EXPECT_THAT(CapturePrintAll(watchList, executor), HasSubstr("[LOCAL] c = \"hello\" (string)"));
}

// var b = true; { var a = 4; <marker> } 구조에서 a, b를 모두 watch하면, 블록이
// 아직 살아있는(a의 스코프가 팝되기 전) 시점엔 a는 [LOCAL], b는 [GLOBAL]로
// 각각 올바르게 구분되어 추가한 순서(a, b)대로 출력되어야 한다.
TEST_F(WatchListTest, PrintAll_MultipleVariablesAcrossLocalAndGlobalScopes_LabelsEachCorrectly) {
	auto marker = MakeExprStmt(MakeIdentifier("a", 3), 3);
	const SyntaxNode* markerPtr = marker.get();

	auto program = MakeProgram(
		MakeVarDeclStmt("b", MakeBoolLiteral(true)),
		MakeBlockStmt(
			MakeVarDeclStmt("a", MakeNumberLiteral(4), 2),
			std::move(marker)
		)
	);

	WatchList watchList;
	watchList.Add("a");
	watchList.Add("b");

	ExecutorUnit executor;
	std::string output;
	CaptureAtMarkerObserver observer(markerPtr, watchList, executor, output);
	executor.SetObserver(&observer);
	executor.Execute(*program);
	executor.SetObserver(nullptr);

	EXPECT_EQ(output, "[LOCAL] a = 4 (number)\n[GLOBAL] b = true (Boolean)\n");
}
#endif
