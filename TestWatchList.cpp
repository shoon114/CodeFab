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
#endif
