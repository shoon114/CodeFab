#pragma once
#include <memory>
#include <vector>
#include "Token.h"
#include "NodeVisitor.h"
#include "Value.h"

enum class NodeType {
	// expression
	NumberLiteral,
	StringLiteral,
	BoolLiteral,
	Identifier,
	BinaryExpr,
	UnaryExpr,
	AssignExpr,
	CallExpr,

	// statement
	VarDeclareStatement,
	ExprStmt,
	PrintStmt,
	IfStmt,
	ForStmt,
	BlockStmt,
	FuncDeclStmt,
	ReturnStmt,

	Program
};

// AST 노드의 추상 베이스. Visitor 패턴의 더블 디스패치를 위해 Accept()를
// 각 구체 타입(아래)이 override하여 자기 자신을 정확한 타입으로 NodeVisitor에 넘긴다.
// type 필드는 각 구체 클래스의 생성자가 채우므로, 노드를 생성할 때 별도로
// 설정할 필요는 없다(구체 타입 자체가 이미 타입 정보를 담고 있음).
class SyntaxNode {
public:
	NodeType type;
	Token token;
	std::vector<std::unique_ptr<SyntaxNode>> children;

	virtual ~SyntaxNode() = default;
	virtual void Accept(NodeVisitor& visitor) const = 0;
};

class NumberLiteralNode : public SyntaxNode {
public:
	NumberLiteralNode() { type = NodeType::NumberLiteral; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class StringLiteralNode : public SyntaxNode {
public:
	StringLiteralNode() { type = NodeType::StringLiteral; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class BoolLiteralNode : public SyntaxNode {
public:
	BoolLiteralNode() { type = NodeType::BoolLiteral; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class IdentifierNode : public SyntaxNode {
public:
	IdentifierNode() { type = NodeType::Identifier; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }

	// CheckerUnit이 정적 바인딩 단계에서 채워두는, 선언된 스코프까지의 거리(홉 수).
	// -1이면 미해결(체커가 못 찾음) 상태이며, ExecutorUnit은 이 경우 기존 동적 탐색으로 폴백한다.
	mutable int scopeDistance = -1;
};

class BinaryExprNode : public SyntaxNode {
public:
	BinaryExprNode() { type = NodeType::BinaryExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }

	// CheckerUnit이 상수 폴딩 단계에서 채워두는, 컴파일 타임에 미리 계산된 값.
	mutable bool isConstantFolded = false;
	mutable Value_t foldedValue{};
};

class UnaryExprNode : public SyntaxNode {
public:
	UnaryExprNode() { type = NodeType::UnaryExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }

	mutable bool isConstantFolded = false;
	mutable Value_t foldedValue{};
};

class AssignExprNode : public SyntaxNode {
public:
	AssignExprNode() { type = NodeType::AssignExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class CallExprNode : public SyntaxNode {
public:
	CallExprNode() { type = NodeType::CallExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class VarDeclareStatementNode : public SyntaxNode {
public:
	VarDeclareStatementNode() { type = NodeType::VarDeclareStatement; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class ExprStmtNode : public SyntaxNode {
public:
	ExprStmtNode() { type = NodeType::ExprStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class PrintStmtNode : public SyntaxNode {
public:
	PrintStmtNode() { type = NodeType::PrintStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class IfStmtNode : public SyntaxNode {
public:
	IfStmtNode() { type = NodeType::IfStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class ForStmtNode : public SyntaxNode {
public:
	ForStmtNode() { type = NodeType::ForStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class BlockStmtNode : public SyntaxNode {
public:
	BlockStmtNode() { type = NodeType::BlockStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class FuncDeclStmtNode : public SyntaxNode {
public:
	FuncDeclStmtNode() { type = NodeType::FuncDeclStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class ReturnStmtNode : public SyntaxNode {
public:
	ReturnStmtNode() { type = NodeType::ReturnStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class ProgramNode : public SyntaxNode {
public:
	ProgramNode() { type = NodeType::Program; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};
