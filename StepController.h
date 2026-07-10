#pragma once
#include <set>

// 디버그 모드에서 "언제 정지할지"만 담당한다. 정지 시점에 뭘 보여줄지(watch)는
// 전혀 모른다 -- 그건 WatchList의 책임이다.
//
// TODO(스텝 담당자): Step/Next/Continue/ShouldPause의 실제 정책을 구현한다.
// - Step: 다음 Stmt 경계에서 무조건 정지
// - Next: 현재 depth 이하로 돌아올 때까지 정지하지 않음(블록 내부로 진입 X)
// - Continue: breakpoint에 걸린 줄에서만 정지
// - ShouldPause(line, depth): 현재 모드 기준으로 이번 경계에서 정지해야 하는지 판단
class StepController {
public:
	enum class Mode { Stepping, SteppingOver, Running };

	void SetBreakpoint(int line);
	void RemoveBreakpoint(int line);
	std::set<int> Breakpoints() const;

	void Step();
	void Next(int currentDepth);
	void Continue();

	bool ShouldPause(int line, int depth) const;

private:
	std::set<int> breakpoints;
	Mode mode = Mode::Stepping;
	int stepOverDepth = 0;
};
