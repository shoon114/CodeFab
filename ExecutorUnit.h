#pragma once
#include "SyntaxNode.h"
#include <string>
#include <unordered_map>
#include <vector>

class ExecutorUnit {
public:
	void Execute(const SyntaxNode& tree);

private:
	void ExecuteStmt(const SyntaxNode& node);
	double Evaluate(const SyntaxNode& node);

	void EnterScope();
	void ExitScope();
	double& ResolveVariable(const std::string& name, int line);

	// scopes[0]이 Global 스코프, 이후 요소는 BlockStmt 진입마다 추가되는 Local 스코프.
	std::vector<std::unordered_map<std::string, double>> scopes;
};
