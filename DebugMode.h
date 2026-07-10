#pragma once
#include "ShellMode.h"
#include <string>

// 디버그 모드: Stmt 단위로 정지하며 step/next/break/watch 등을 지원한다.
// DebugSession이 ExecutionObserver로 등록되어 ExecutorUnit::ExecuteStmt의
// 문 경계마다 통보받는다.
class DebugMode : public ShellMode {
public:
	explicit DebugMode(std::string path);
	int Run() override;

private:
	std::string path;
};
