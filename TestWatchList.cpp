#ifdef _DEBUG
#include "gmock/gmock.h"
#include "WatchList.h"
#include "ExecutionObserver.h"
#include "ExecutorUnit.h"
#include "SyntaxNode.h"
#include <memory>
#include <string>

using namespace testing;

TEST(WatchListTest, Add_NewName_IsInList) {
	WatchList watchList;
	watchList.Add("a");

	EXPECT_TRUE(watchList.Contains("a"));
}

TEST(WatchListTest, Remove_WatchedName_IsNoLongerInList) {
	WatchList watchList;
	watchList.Add("a");

	watchList.Remove("a");//unWatch

	EXPECT_FALSE(watchList.Contains("a"));
}

// var a = 4; 실행 후(전역 스코프에 남아있는 상태) watches를 조회하면
// "[LOCAL] a = 4 (number)"가 출력되어야 한다.
TEST(WatchListTest, PrintAll_GlobalNumberVariable_PrintsGlobalLabelAndNumberType) {
	auto identifier = std::make_unique<IdentifierNode>();
	identifier->token.type = TokenType::Identifier;
	identifier->token.lexeme = "a";
	identifier->token.line = 1;

	auto numberLiteral = std::make_unique<NumberLiteralNode>();
	numberLiteral->token.type = TokenType::Number;
	numberLiteral->token.realValue = 4;
	numberLiteral->token.line = 1;

	auto assign = std::make_unique<AssignExprNode>();
	assign->token.type = TokenType::Assign;
	assign->token.lexeme = "=";
	assign->token.line = 1;
	assign->children.push_back(std::move(identifier));
	assign->children.push_back(std::move(numberLiteral));

	auto varDecl = std::make_unique<VarDeclareStatementNode>();
	varDecl->token.type = TokenType::KwVar;
	varDecl->token.lexeme = "var";
	varDecl->token.line = 1;
	varDecl->children.push_back(std::move(assign));

	auto program = std::make_unique<ProgramNode>();
	program->children.push_back(std::move(varDecl));

	ExecutorUnit executor;
	executor.Execute(*program);

	//watch a
	WatchList watchList;
	watchList.Add("a");

	testing::internal::CaptureStdout();
	watchList.PrintAll(executor);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, HasSubstr("[LOCAL] a = 4 (number)"));
}

// var b = true; 실행 후 watches를 조회하면 "[LOCAL] b = true (Boolean)"가 출력되어야 한다.
TEST(WatchListTest, PrintAll_LocalBooleanVariable_PrintsLocalLabelAndBooleanType) {
	auto identifier = std::make_unique<IdentifierNode>();
	identifier->token.type = TokenType::Identifier;
	identifier->token.lexeme = "b";
	identifier->token.line = 1;

	auto boolLiteral = std::make_unique<BoolLiteralNode>();
	boolLiteral->token.lexeme = "true";
	boolLiteral->token.line = 1;

	auto assign = std::make_unique<AssignExprNode>();
	assign->token.type = TokenType::Assign;
	assign->token.lexeme = "=";
	assign->token.line = 1;
	assign->children.push_back(std::move(identifier));
	assign->children.push_back(std::move(boolLiteral));

	auto varDecl = std::make_unique<VarDeclareStatementNode>();
	varDecl->token.type = TokenType::KwVar;
	varDecl->token.lexeme = "var";
	varDecl->token.line = 1;
	varDecl->children.push_back(std::move(assign));

	auto program = std::make_unique<ProgramNode>();
	program->children.push_back(std::move(varDecl));

	ExecutorUnit executor;
	executor.Execute(*program);

	//watch b
	WatchList watchList;
	watchList.Add("b");

	testing::internal::CaptureStdout();
	watchList.PrintAll(executor);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, HasSubstr("[LOCAL] b = true (Boolean)"));
}

// var c = "hello"; 실행 후 watches를 조회하면 "[LOCAL] c = \"hello\" (string)"가
// 출력되어야 한다 (문자열 값은 큰따옴표로 감싸서 출력).
TEST(WatchListTest, PrintAll_LocalStringVariable_PrintsLocalLabelAndStringType) {
	auto identifier = std::make_unique<IdentifierNode>();
	identifier->token.type = TokenType::Identifier;
	identifier->token.lexeme = "c";
	identifier->token.line = 1;

	auto stringLiteral = std::make_unique<StringLiteralNode>();
	stringLiteral->token.type = TokenType::String;
	stringLiteral->token.lexeme = "hello";
	stringLiteral->token.line = 1;

	auto assign = std::make_unique<AssignExprNode>();
	assign->token.type = TokenType::Assign;
	assign->token.lexeme = "=";
	assign->token.line = 1;
	assign->children.push_back(std::move(identifier));
	assign->children.push_back(std::move(stringLiteral));

	auto varDecl = std::make_unique<VarDeclareStatementNode>();
	varDecl->token.type = TokenType::KwVar;
	varDecl->token.lexeme = "var";
	varDecl->token.line = 1;
	varDecl->children.push_back(std::move(assign));

	auto program = std::make_unique<ProgramNode>();
	program->children.push_back(std::move(varDecl));

	ExecutorUnit executor;
	executor.Execute(*program);

	//watch c
	WatchList watchList;
	watchList.Add("c");

	testing::internal::CaptureStdout();
	watchList.PrintAll(executor);
	std::string output = testing::internal::GetCapturedStdout();

	EXPECT_THAT(output, HasSubstr("[LOCAL] c = \"hello\" (string)"));
}

// var b = true; { var a = 4; <marker> } 구조에서 a, b를 모두 watch하면, 블록이
// 아직 살아있는(a의 스코프가 팝되기 전) 시점엔 a는 [LOCAL], b는 [GLOBAL]로
// 각각 올바르게 구분되어 추가한 순서(a, b)대로 출력되어야 한다.
TEST(WatchListTest, PrintAll_MultipleVariablesAcrossLocalAndGlobalScopes_LabelsEachCorrectly) {
	auto bIdentifier = std::make_unique<IdentifierNode>();
	bIdentifier->token.type = TokenType::Identifier;
	bIdentifier->token.lexeme = "b";
	bIdentifier->token.line = 1;

	auto bBoolLiteral = std::make_unique<BoolLiteralNode>();
	bBoolLiteral->token.lexeme = "true";
	bBoolLiteral->token.line = 1;

	auto bAssign = std::make_unique<AssignExprNode>();
	bAssign->token.type = TokenType::Assign;
	bAssign->token.lexeme = "=";
	bAssign->token.line = 1;
	bAssign->children.push_back(std::move(bIdentifier));
	bAssign->children.push_back(std::move(bBoolLiteral));

	auto bVarDecl = std::make_unique<VarDeclareStatementNode>();
	bVarDecl->token.type = TokenType::KwVar;
	bVarDecl->token.lexeme = "var";
	bVarDecl->token.line = 1;
	bVarDecl->children.push_back(std::move(bAssign));

	auto aIdentifier = std::make_unique<IdentifierNode>();
	aIdentifier->token.type = TokenType::Identifier;
	aIdentifier->token.lexeme = "a";
	aIdentifier->token.line = 2;

	auto aNumberLiteral = std::make_unique<NumberLiteralNode>();
	aNumberLiteral->token.type = TokenType::Number;
	aNumberLiteral->token.realValue = 4;
	aNumberLiteral->token.line = 2;

	auto aAssign = std::make_unique<AssignExprNode>();
	aAssign->token.type = TokenType::Assign;
	aAssign->token.lexeme = "=";
	aAssign->token.line = 2;
	aAssign->children.push_back(std::move(aIdentifier));
	aAssign->children.push_back(std::move(aNumberLiteral));

	auto aVarDecl = std::make_unique<VarDeclareStatementNode>();
	aVarDecl->token.type = TokenType::KwVar;
	aVarDecl->token.lexeme = "var";
	aVarDecl->token.line = 2;
	aVarDecl->children.push_back(std::move(aAssign));

	// 블록이 아직 안 끝난 시점을 붙잡기 위한 marker statement (var a = 4; 다음 줄).
	auto markerIdentifier = std::make_unique<IdentifierNode>();
	markerIdentifier->token.type = TokenType::Identifier;
	markerIdentifier->token.lexeme = "a";
	markerIdentifier->token.line = 3;

	auto markerExprStmt = std::make_unique<ExprStmtNode>();
	markerExprStmt->token.line = 3;
	markerExprStmt->children.push_back(std::move(markerIdentifier));
	const SyntaxNode* markerPtr = markerExprStmt.get();

	auto block = std::make_unique<BlockStmtNode>();
	block->children.push_back(std::move(aVarDecl));
	block->children.push_back(std::move(markerExprStmt));

	auto program = std::make_unique<ProgramNode>();
	program->children.push_back(std::move(bVarDecl));
	program->children.push_back(std::move(block));

	WatchList watchList;
	watchList.Add("a");
	watchList.Add("b");

	std::string output;

	// marker statement에 진입하는 순간(블록 스코프가 아직 살아있을 때) watches를
	// 출력해 캡처하는, 이 테스트 전용 옵저버.
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

	ExecutorUnit executor;
	CaptureAtMarkerObserver observer(markerPtr, watchList, executor, output);
	executor.SetObserver(&observer);
	executor.Execute(*program);
	executor.SetObserver(nullptr);

	EXPECT_EQ(output, "[LOCAL] a = 4 (number)\n[GLOBAL] b = true (Boolean)\n");
}
#endif
