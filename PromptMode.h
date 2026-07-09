#pragma once
#include "ShellMode.h"

// 프롬프트 모드(REPL): 사용자가 소스를 한 줄씩 입력하는 대화형 실행 모드.
// 전역 변수 저장소(ExecutorUnit)는 이 Run() 호출 동안, 즉 세션 종료 전까지 유지된다.
class PromptMode : public ShellMode {
public:
	int Run() override;
};
