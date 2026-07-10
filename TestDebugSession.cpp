#ifdef _DEBUG
#include "gmock/gmock.h"
#include "DebugSession.h"
#include "ExecutorUnit.h"
#include "SyntaxNode.h"
#include <memory>
#include <sstream>
#include <string>

using namespace testing;

// DebugSession::HandleCommand는 private이라 직접 호출할 수 없다. 대신 기본
// StepController 모드(Stepping)는 항상 정지하므로, 공개된 OnStmtEnter(node)를
// 호출하면 곧바로 PromptAndHandleCommands가 실행되어 std::cin에서 명령을 읽는다.
// std::cin을 미리 준비한 명령 시퀀스로 바꿔치기해서 그 경로를 그대로 태운다.
// (exit/quit은 std::exit(0)으로 테스트 프로세스 자체를 죽이므로 절대 넣지 않는다.)
class DebugSessionTest : public ::testing::Test {
protected:
	// var a = 4; 하나만 있는 프로그램. watches/inspect가 실제 값을 보여주는지
	// 확인하는 테스트가 실행 후 상태로 재사용한다.
	std::unique_ptr<SyntaxNode> MakeProgramWithVarA4() {
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
		return program;
	}

	// std::cin을 commands로 바꿔치기한 뒤 session.OnStmtEnter를 호출해 정지/명령
	// 루프를 그대로 태우고, 캡처된 stdout을 반환한다. 호출이 끝나면 std::cin을
	// 원래대로 복원한다.
	std::string RunDebugSession(DebugSession& session, const std::string& commands) {
		std::istringstream input(commands);
		std::streambuf* originalCin = std::cin.rdbuf(input.rdbuf());

		ExprStmtNode node;
		node.token.line = 0;

		testing::internal::CaptureStdout();
		session.OnStmtEnter(node);
		std::string output = testing::internal::GetCapturedStdout();

		std::cin.rdbuf(originalCin);
		return output;
	}
};

TEST_F(DebugSessionTest, WatchCommand_PrintsConfirmationMessage) {
	ExecutorUnit executor;
	DebugSession session({}, executor);

	std::string output = RunDebugSession(session, "watch a\ncontinue\n");

	EXPECT_THAT(output, HasSubstr("[DEBUG] Watching 'a'"));
}

TEST_F(DebugSessionTest, UnwatchCommand_PrintsConfirmationMessage) {
	ExecutorUnit executor;
	DebugSession session({}, executor);

	std::string output = RunDebugSession(session, "watch a\nunwatch a\ncontinue\n");

	EXPECT_THAT(output, HasSubstr("[DEBUG] Unwatching 'a'"));
}

TEST_F(DebugSessionTest, RemoveCommand_PrintsConfirmationMessage) {
	ExecutorUnit executor;
	DebugSession session({}, executor);

	std::string output = RunDebugSession(session, "break 3\nremove 3\ncontinue\n");

	EXPECT_THAT(output, HasSubstr("[DEBUG] Removed breakpoint at line : 3"));
}

// var a = 4; 실행 후(스코프가 하나뿐이라 CurrentScope()가 곧 global) watch a를 등록하고
// watches 명령을 치면, DebugSession이 WatchList::Watches(executor)의 반환값을 그대로
// std::cout으로 찍어야 한다.
TEST_F(DebugSessionTest, WatchesCommand_PrintsWatchedVariableValue) {
	auto program = MakeProgramWithVarA4();

	ExecutorUnit executor;
	executor.Execute(*program);

	DebugSession session({}, executor);

	std::string output = RunDebugSession(session, "watch a\nwatches\ncontinue\n");

	EXPECT_THAT(output, HasSubstr("[LOCAL] a = 4 (number)"));
}

// var a = 4; 실행 후 inspect 명령을 치면, watch 목록에 등록한 적이 없어도
// DebugSession이 WatchList::Inspect(executor)의 반환값을 그대로 std::cout으로 찍어야 한다.
TEST_F(DebugSessionTest, InspectCommand_PrintsCurrentScopeVariable) {
	auto program = MakeProgramWithVarA4();

	ExecutorUnit executor;
	executor.Execute(*program);

	DebugSession session({}, executor);

	std::string output = RunDebugSession(session, "inspect\ncontinue\n");

	EXPECT_THAT(output, HasSubstr("[LOCAL] a = 4 (number)"));
}
#endif
