#pragma once
#include "ExecutionObserver.h"
#include "StepController.h"
#include "WatchList.h"
#include <streambuf>
#include <string>
#include <vector>

class ExecutorUnit;

// factory debug 모드의 오케스트레이터. ExecutionObserver로 등록되어 문 경계마다
// 통보받고, "언제 정지할지"는 StepController, "정지 시 뭘 보여줄지"는 WatchList에
// 위임한다. 이 클래스 자신은 명령 문자열 파싱/라우팅과 depth 추적만 담당한다.
class DebugSession : public ExecutionObserver {
public:
	DebugSession(std::vector<std::string> sourceLines, ExecutorUnit& executor);
	~DebugSession();

	void OnStmtEnter(const SyntaxNode& node) override;
	void OnStmtExit(const SyntaxNode& node) override;

private:
	// 정지 시점마다 현재 줄/watch를 출력하고, step/next/continue가 입력될 때까지
	// break/remove/breakpoints/watch/unwatch/watches/inspect 명령을 반복 처리한다.
	void PromptAndHandleCommands(const SyntaxNode& node);
	// 실행을 재개해야 하면(step/next/continue) true를 반환한다.
	bool HandleCommand(const std::string& command);
	std::string PrintCurrentLine(const SyntaxNode& node) const;
	// 직전 std::cout 출력이 개행 없이 끝났으면("print"문처럼) 줄바꿈 하나를,
	// 이미 새 줄이면 빈 문자열을 반환한다 -- 불필요한 빈 줄 없이 항상 새 줄에서
	// 시작하도록 디버그 메시지를 찍기 전에 이 반환값을 먼저 출력한다.
	std::string EnsureNewLine() const;

	std::vector<std::string> sourceLines;
	ExecutorUnit& executor;

	StepController stepController;
	WatchList watchList;

	// OnStmtEnter에서 +1, OnStmtExit에서 -1 -- next가 "현재 문의 블록 내부로
	// 들어가지 않고 그 다음 문에서 정지"하려면 중첩 깊이를 추적해야 한다.
	int depth = 0;

	// std::cout에 마지막으로 쓰인 문자가 개행인지 추적하는 streambuf 래퍼.
	// ExecutorUnit은 이 클래스의 존재를 전혀 모른 채(디버깅 개념과 무관하게)
	// 그냥 std::cout에 쓸 뿐이고, 이 래퍼가 그 아래에서 실제 버퍼로 그대로
	// 전달하면서 마지막 문자만 훔쳐본다.
	class NewlineTrackingStreambuf : public std::streambuf {
	public:
		explicit NewlineTrackingStreambuf(std::streambuf* target);
		bool AtLineStart() const;
		// "(debug) "/">>> " 같은 프롬프트 장식은 일부러 개행 없이 끝나지만, 그건
		// 프로그램이 개행 없이 출력한 것과는 무관하므로(EnsureNewLine이 신경 쓰는
		// 대상이 아니므로) 프롬프트를 찍은 직후 이 상태를 "새 줄"로 되돌린다.
		void MarkLineStart();

	protected:
		int overflow(int ch) override;
		std::streamsize xsputn(const char* s, std::streamsize count) override;

	private:
		std::streambuf* target;
		bool atLineStart = true;
	};

	std::streambuf* originalCoutBuffer;
	NewlineTrackingStreambuf outputTracker;
};
