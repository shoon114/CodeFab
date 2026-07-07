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

	// ExecuteStmt가 위임하는 문(statement)별 실행 메서드
	void ExecutePrintStmt(const SyntaxNode& node);
	void ExecuteVarDeclareStatement(const SyntaxNode& node);
	void ExecuteBlockStmt(const SyntaxNode& node);
	void ExecuteIfStmt(const SyntaxNode& node);
	void ExecuteExprStmt(const SyntaxNode& node);
	void ExecuteForStmt(const SyntaxNode& node);

	// Evaluate가 위임하는 식(expression)별 평가 메서드
	Value_t EvaluateIdentifier(const SyntaxNode& node);
	Value_t EvaluateAssignExpr(const SyntaxNode& node);
	Value_t EvaluateBinaryExpr(const SyntaxNode& node);

	void EnterScope();
	void ExitScope();
	Value_t& ResolveVariable(const std::string& name, int line);
	double AsNumber(const Value_t& value, int line);
	bool IsTruthy(const Value_t& value);

	// scopes[0]이 Global 스코프, 이후 요소는 BlockStmt 진입마다 추가되는 Local 스코프.
	std::vector<std::unordered_map<std::string, Value_t>> scopes;
};
