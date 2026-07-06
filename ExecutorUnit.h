#pragma once
#include "SyntaxNode.h"

class ExecutorUnit {
public:
	void Execute(const SyntaxNode& tree);

private:
	void ExecuteStmt(const SyntaxNode& node);
	double Evaluate(const SyntaxNode& node);
};
