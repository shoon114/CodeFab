#pragma once
#include "NodeVisitor.h"
#include "SyntaxNode.h"
#include "Value.h"
#include <string>
#include <unordered_map>
#include <vector>

class ExecutorUnit : public NodeVisitor {
public:
	void Execute(const SyntaxNode& tree);

	// 문(statement) 노드: 실행 후 값을 반환하지 않는다.
	void Visit(const PrintStmtNode& node) override;
	void Visit(const VarDeclareStatementNode& node) override;
	void Visit(const BlockStmtNode& node) override;
	void Visit(const IfStmtNode& node) override;
	void Visit(const ExprStmtNode& node) override;
	void Visit(const ForStmtNode& node) override;
	void Visit(const FuncDeclStmtNode& node) override;
	void Visit(const ReturnStmtNode& node) override;

	// 식(expression) 노드: 결과를 lastValue에 남기고, Evaluate()가 이를 읽어 반환한다.
	void Visit(const NumberLiteralNode& node) override;
	void Visit(const StringLiteralNode& node) override;
	void Visit(const BoolLiteralNode& node) override;
	void Visit(const IdentifierNode& node) override;
	void Visit(const AssignExprNode& node) override;
	void Visit(const BinaryExprNode& node) override;
	void Visit(const UnaryExprNode& node) override;
	void Visit(const CallExprNode& node) override;

private:
	void ExecuteStmt(const SyntaxNode& node);
	Value_t Evaluate(const SyntaxNode& node);

	// Visit(Stmt)가 위임하는 문(statement)별 실행 메서드
	void ExecutePrintStmt(const SyntaxNode& node);
	void ExecuteVarDeclareStatement(const SyntaxNode& node);
	void ExecuteBlockStmt(const SyntaxNode& node);
	void ExecuteIfStmt(const SyntaxNode& node);
	void ExecuteExprStmt(const SyntaxNode& node);
	void ExecuteForStmt(const SyntaxNode& node);

	// Visit(Expr)가 위임하는 식(expression)별 평가 메서드
	Value_t EvaluateIdentifier(const IdentifierNode& node);
	Value_t EvaluateAssignExpr(const SyntaxNode& node);
	Value_t EvaluateBinaryExpr(const BinaryExprNode& node);
	Value_t EvaluateUnaryExpr(const UnaryExprNode& node);

	void EnterScope();
	void ExitScope();
	// distance는 CheckerUnit이 미리 계산해둔, 선언 스코프까지의 홉 수(정적 바인딩).
	// -1이면 미해결 상태이므로 기존 동적(선형) 탐색으로 폴백한다.
	Value_t& ResolveVariable(const std::string& name, int distance, int line);
	double AsNumber(const Value_t& value, int line);
	bool IsTruthy(const Value_t& value);

	// scopes[0]이 Global 스코프, 이후 요소는 BlockStmt 진입마다 추가되는 Local 스코프.
	std::vector<std::unordered_map<std::string, Value_t>> scopes;

	// Evaluate()가 Accept()를 통해 식 노드를 방문한 뒤 결과를 꺼내가는 임시 저장소.
	Value_t lastValue;
};
