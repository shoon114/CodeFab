#pragma once
#include "SyntaxNode.h"
#include <string>
#include <unordered_map>

class ExecutorUnit {
public:
	void Execute(const SyntaxNode& tree);

private:
	void ExecuteStmt(const SyntaxNode& node);
	double Evaluate(const SyntaxNode& node);

	std::unordered_map<std::string, double> variables;
};
