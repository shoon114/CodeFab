#include "DebugSession.h"
#include "ExecutorUnit.h"
#include "SyntaxNode.h"
#include <iostream>
#include <sstream>

DebugSession::DebugSession(std::vector<std::string> sourceLines, ExecutorUnit& executor)
	: sourceLines(std::move(sourceLines)), executor(executor) {
}

void DebugSession::OnStmtEnter(const SyntaxNode& node) {
	depth++;

	if (stepController.ShouldPause(node.token.line, depth)) {
		PromptAndHandleCommands(node);
	}
}

void DebugSession::OnStmtExit(const SyntaxNode&) {
	depth--;
}

void DebugSession::PrintCurrentLine(const SyntaxNode& node) const {
	int tokenLine = node.token.line;  // 0-indexed
	int displayLine = tokenLine + 1;  // 사용자에게 보여주는 1-indexed

	std::string sourceCode;
	if (static_cast<size_t>(tokenLine) < sourceLines.size()) {
		sourceCode = sourceLines[tokenLine];
	}

	std::cout << "[DEBUG] Stop at line : " << displayLine;
	if (stepController.IsRunningMode()) {
		std::cout << " (breakpoint)";
	}
	std::cout << " > " << sourceCode << std::endl;
}

void DebugSession::PromptAndHandleCommands(const SyntaxNode& node) {
	PrintCurrentLine(node);
	watchList.Watches(executor);

	std::string command;
	while (true) {
		std::cout << "(debug) ";
		if (!std::getline(std::cin, command)) {
			// 입력 스트림이 끝나면(예: 파이프 입력 소진) 더 이상 멈추지 않고
			// 남은 프로그램을 끝까지 실행한다.
			stepController.Continue();
			return;
		}
		if (HandleCommand(command)) {
			return;
		}
	}
}

bool DebugSession::HandleCommand(const std::string& command) {
	std::istringstream iss(command);
	std::string verb;
	iss >> verb;

	if (verb == "exit" || verb == "quit") {
		std::exit(0);
	}
	if (verb == "step") {
		stepController.Step();
		return true;
	}
	if (verb == "next") {
		stepController.Next(depth);
		return true;
	}
	if (verb == "continue") {
		stepController.Continue();
		return true;
	}
	if (verb == "break") {
		int line;
		if (iss >> line) {
			stepController.SetBreakpoint(line - 1);  // 사용자 입력은 1-indexed
			std::cout << "[DEBUG] Set breakpoint at line : " << line << std::endl;
		}
		return false;
	}
	if (verb == "remove") {
		int line;
		if (iss >> line) {
			stepController.RemoveBreakpoint(line - 1);  // 사용자 입력은 1-indexed
		}
		return false;
	}
	if (verb == "breakpoints") {
		for (int line : stepController.Breakpoints()) {
			std::cout << "  " << (line + 1) << std::endl;  // 내부 0-indexed → 1-indexed 표시
		}
		return false;
	}
	if (verb == "watch") {
		std::string name;
		if (iss >> name) {
			watchList.Add(name);
		}
		return false;
	}
	if (verb == "unwatch") {
		std::string name;
		if (iss >> name) {
			watchList.Remove(name);
		}
		return false;
	}
	if (verb == "watches") {
		watchList.Watches(executor);
		return false;
	}
	if (verb == "inspect") {
		watchList.Inspect(executor);
		return false;
	}

	std::cout << "Unknown command: " << command << std::endl;
	return false;
}
