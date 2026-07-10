#pragma once
#include "ExecutionObserver.h"
#include "StepController.h"
#include "WatchList.h"
#include <string>
#include <vector>

class ExecutorUnit;

// factory debug 모드의 오케스트레이터. ExecutionObserver로 등록되어 문 경계마다
// 통보받고, "언제 정지할지"는 StepController, "정지 시 뭘 보여줄지"는 WatchList에
// 위임한다. 이 클래스 자신은 명령 문자열 파싱/라우팅과 depth 추적만 담당한다.
class DebugSession : public ExecutionObserver {
public:
	DebugSession(std::vector<std::string> sourceLines, ExecutorUnit& executor);

	void OnStmtEnter(const SyntaxNode& node) override;
	void OnStmtExit(const SyntaxNode& node) override;

private:
	// 정지 시점마다 현재 줄/watch를 출력하고, step/next/continue가 입력될 때까지
	// break/remove/breakpoints/watch/unwatch/watches/inspect 명령을 반복 처리한다.
	void PromptAndHandleCommands(const SyntaxNode& node, bool isBreakpoint);
	// 실행을 재개해야 하면(step/next/continue) true를 반환한다.
	bool HandleCommand(const std::string& command);
	void PrintCurrentLine(const SyntaxNode& node, bool isBreakpoint) const;

	std::vector<std::string> sourceLines;
	ExecutorUnit& executor;

	StepController stepController;
	WatchList watchList;

	// OnStmtEnter에서 +1, OnStmtExit에서 -1 -- next가 "현재 문의 블록 내부로
	// 들어가지 않고 그 다음 문에서 정지"하려면 중첩 깊이를 추적해야 한다.
	int depth = 0;
};
