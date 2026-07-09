#pragma once
#include "ShellMode.h"
#include <string>

// 디버그 모드: Stmt 단위로 정지하며 step/next/break/watch 등을 지원할 예정.
// 현재는 쉘(진입점 분기) PR 범위라 스텁만 있고, stepping/watch 구현은 다음 PR에서 추가한다.
class DebugMode : public ShellMode {
public:
	explicit DebugMode(std::string path);
	int Run() override;

private:
	std::string path;
};
