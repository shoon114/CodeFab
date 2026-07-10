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
	mode = Mode::Stepping;
}

void StepController::Next(int currentDepth) {
	mode = Mode::SteppingOver;
	stepOverDepth = currentDepth;
}

void StepController::Continue() {
	mode = Mode::Running;
}

bool StepController::IsRunningMode() const {
	return mode == Mode::Running;
}

bool StepController::ShouldPause(int line, int depth) const {
	switch (mode) {
	case Mode::Stepping:
		return true;
	case Mode::SteppingOver:
		return depth <= stepOverDepth;
	case Mode::Running:
		return breakpoints.count(line) > 0;
	}
	return false;
}
