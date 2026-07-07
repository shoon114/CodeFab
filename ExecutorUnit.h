#pragma once
#include "SyntaxNode.h"
#include "Value.h"
#include <string>
#include <unordered_map>
#include <vector>

class ExecutorUnit {
public:
	void Execute(const SyntaxNode& tree);

private:
	void ExecuteStmt(const SyntaxNode& node);
	Value_t Evaluate(const SyntaxNode& node);

	void EnterScope();
	void ExitScope();
	Value_t& ResolveVariable(const std::string& name, int line);
	double AsNumber(const Value_t& value, int line);

	// scopes[0]이 Global 스코프, 이후 요소는 BlockStmt 진입마다 추가되는 Local 스코프.
	std::vector<std::unordered_map<std::string, Value_t>> scopes;
};
