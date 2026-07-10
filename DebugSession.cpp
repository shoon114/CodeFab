#include "DebugSession.h"
#include "ExecutorUnit.h"
#include "SyntaxNode.h"
#include <iostream>
#include <sstream>

DebugSession::NewlineTrackingStreambuf::NewlineTrackingStreambuf(std::streambuf* target) : target(target) {}

bool DebugSession::NewlineTrackingStreambuf::AtLineStart() const {
	return atLineStart;
}

int DebugSession::NewlineTrackingStreambuf::overflow(int ch) {
	if (ch != EOF) {
		atLineStart = (ch == '\n');
		return target->sputc(static_cast<char>(ch));
	}
	return ch;
}

std::streamsize DebugSession::NewlineTrackingStreambuf::xsputn(const char* s, std::streamsize count) {
	if (count > 0) {
		atLineStart = (s[count - 1] == '\n');
	}
	return target->sputn(s, count);
}

void DebugSession::NewlineTrackingStreambuf::MarkLineStart() {
	atLineStart = true;
}

DebugSession::DebugSession(std::vector<std::string> sourceLines, ExecutorUnit& executor)
	: sourceLines(std::move(sourceLines)), executor(executor),
	  originalCoutBuffer(std::cout.rdbuf()), outputTracker(originalCoutBuffer) {
	std::cout.rdbuf(&outputTracker);
}

DebugSession::~DebugSession() {
	std::cout.rdbuf(originalCoutBuffer);
}

std::string DebugSession::EnsureNewLine() const {
	return outputTracker.AtLineStart() ? "" : "\n";
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

std::string DebugSession::PrintCurrentLine(const SyntaxNode& node) const {
	int tokenLine = node.token.line;  // 0-indexed
	int displayLine = tokenLine + 1;  // 사용자에게 보여주는 1-indexed

	std::string sourceCode;
	if (static_cast<size_t>(tokenLine) < sourceLines.size()) {
		sourceCode = sourceLines[tokenLine];
	}

	std::ostringstream out;
	out << EnsureNewLine();
	out << "[DEBUG] Stop at line : " << displayLine;
	if (stepController.IsRunningMode()) {
		out << " (breakpoint)";
	}
	out << " > " << sourceCode << std::endl;
	return out.str();
}

void DebugSession::PromptAndHandleCommands(const SyntaxNode& node) {
	std::cout << PrintCurrentLine(node);
	std::cout << watchList.Watches(executor);

	std::string command;
	while (true) {
		std::cout << ">>> ";
		// 프롬프트 장식 자체는 개행 없이 끝나지만, EnsureNewLine이 신경 쓰는 건
		// "프로그램이 방금 개행 없이 뭔가 출력했는지"이지 이 프롬프트가 아니므로
		// 상태를 새 줄로 되돌린다.
		outputTracker.MarkLineStart();
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
			std::cout << EnsureNewLine();
			std::cout << "[DEBUG] Set breakpoint at line : " << line << std::endl;
		}
		return false;
	}
	if (verb == "remove") {
		int line;
		if (iss >> line) {
			stepController.RemoveBreakpoint(line - 1);  // 사용자 입력은 1-indexed
			std::cout << EnsureNewLine();
			std::cout << "[DEBUG] Removed breakpoint at line : " << line << std::endl;
		}
		return false;
	}
	if (verb == "breakpoints") {
		std::cout << EnsureNewLine();
		for (int line : stepController.Breakpoints()) {
			std::cout << "  " << (line + 1) << std::endl;  // 내부 0-indexed → 1-indexed 표시
		}
		return false;
	}
	if (verb == "watch") {
		std::string name;
		if (iss >> name) {
			watchList.Add(name);
			std::cout << "[DEBUG] Watching '" << name << "'" << std::endl;
		}
		return false;
	}
	if (verb == "unwatch") {
		std::string name;
		if (iss >> name) {
			watchList.Remove(name);
			std::cout << "[DEBUG] Unwatching '" << name << "'" << std::endl;
		}
		return false;
	}
	if (verb == "watches") {
		std::cout << watchList.Watches(executor);
		return false;
	}
	if (verb == "inspect") {
		std::cout << watchList.Inspect(executor);
		return false;
	}

	std::cout << "Unknown command: " << command << std::endl;
	return false;
}
