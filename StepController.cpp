#include "StepController.h"

void StepController::SetBreakpoint(int line) {
	breakpoints.insert(line);
}

void StepController::RemoveBreakpoint(int line) {
	breakpoints.erase(line);
}

std::set<int> StepController::Breakpoints() const {
	return breakpoints;
}

void StepController::Step() {
	// TODO(스텝 담당자): mode = Mode::Stepping;
}

void StepController::Next(int currentDepth) {
	// TODO(스텝 담당자): mode = Mode::SteppingOver; stepOverDepth = currentDepth;
	(void)currentDepth;
}

void StepController::Continue() {
	// TODO(스텝 담당자): mode = Mode::Running;
}

bool StepController::ShouldPause(int line, int depth) const {
	// TODO(스텝 담당자): mode에 따라 Stepping(항상)/SteppingOver(depth<=stepOverDepth)/
	// Running(breakpoints.count(line)) 중 하나로 판단
	(void)line;
	(void)depth;
	return false;
}
