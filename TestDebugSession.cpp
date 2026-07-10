#ifdef _DEBUG
#include "gmock/gmock.h"
#include "DebugSession.h"
#include "ExecutorUnit.h"
#include "SyntaxNode.h"
#include <sstream>
#include <string>

using namespace testing;

// DebugSession::HandleCommand는 private이라 직접 호출할 수 없다. 대신 기본
// StepController 모드(Stepping)는 항상 정지하므로, 공개된 OnStmtEnter(node)를
// 호출하면 곧바로 PromptAndHandleCommands가 실행되어 std::cin에서 명령을 읽는다.
// std::cin을 미리 준비한 명령 시퀀스로 바꿔치기해서 그 경로를 그대로 태운다.
// (exit/quit은 std::exit(0)으로 테스트 프로세스 자체를 죽이므로 절대 넣지 않는다.)

TEST(DebugSessionTest, WatchCommand_PrintsConfirmationMessage) {
	ExecutorUnit executor;
	DebugSession session({}, executor);

	std::istringstream input("watch a\ncontinue\n");
	std::streambuf* originalCin = std::cin.rdbuf(input.rdbuf());

	ExprStmtNode node;
	node.token.line = 0;

	testing::internal::CaptureStdout();
	session.OnStmtEnter(node);
	std::string output = testing::internal::GetCapturedStdout();

	std::cin.rdbuf(originalCin);

	EXPECT_THAT(output, HasSubstr("[DEBUG] Watching 'a'"));
}

TEST(DebugSessionTest, UnwatchCommand_PrintsConfirmationMessage) {
	ExecutorUnit executor;
	DebugSession session({}, executor);

	std::istringstream input("watch a\nunwatch a\ncontinue\n");
	std::streambuf* originalCin = std::cin.rdbuf(input.rdbuf());

	ExprStmtNode node;
	node.token.line = 0;

	testing::internal::CaptureStdout();
	session.OnStmtEnter(node);
	std::string output = testing::internal::GetCapturedStdout();

	std::cin.rdbuf(originalCin);

	EXPECT_THAT(output, HasSubstr("[DEBUG] Unwatching 'a'"));
}

TEST(DebugSessionTest, RemoveCommand_PrintsConfirmationMessage) {
	ExecutorUnit executor;
	DebugSession session({}, executor);

	std::istringstream input("break 3\nremove 3\ncontinue\n");
	std::streambuf* originalCin = std::cin.rdbuf(input.rdbuf());

	ExprStmtNode node;
	node.token.line = 0;

	testing::internal::CaptureStdout();
	session.OnStmtEnter(node);
	std::string output = testing::internal::GetCapturedStdout();

	std::cin.rdbuf(originalCin);

	EXPECT_THAT(output, HasSubstr("[DEBUG] Removed breakpoint at line : 3"));
}
#endif
