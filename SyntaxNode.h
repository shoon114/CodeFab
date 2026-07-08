#pragma once
#include <memory>
#include <vector>
#include "Token.h"
#include "NodeVisitor.h"

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
	ArrExpr,
	IndexExpr,

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
};

class BinaryExprNode : public SyntaxNode {
public:
	BinaryExprNode() { type = NodeType::BinaryExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class UnaryExprNode : public SyntaxNode {
public:
	UnaryExprNode() { type = NodeType::UnaryExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class AssignExprNode : public SyntaxNode {
public:
	AssignExprNode() { type = NodeType::AssignExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

// token은 callee 식별자 토큰(호출 대상 이름). closeParenToken은 닫는 ')' 토큰
// (트리 출력 등에서 호출 구문의 괄호를 함께 보여주기 위함).
class CallExprNode : public SyntaxNode {
public:
	CallExprNode() { type = NodeType::CallExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }

	Token closeParenToken;
};

// Array(size) 배열 생성 전용 노드. children = [size식]. token은 'Array' 식별자 토큰.
// 일반 함수 호출(CallExprNode)과 분리해두면 Checker/Executor가 사용자 정의 함수
// 호출과 배열 생성을 타입 검사만으로(문자열 이름 비교 없이) 구분할 수 있다.
class ArrExprNode : public SyntaxNode {
public:
	ArrExprNode() { type = NodeType::ArrExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }

	Token closeParenToken;
};

// children = [array식, index식]. token은 여는 '[' 토큰(위치 정보용),
// closeBracketToken은 닫는 ']' 토큰(트리 출력 등에서 여는/닫는 토큰을 함께 보여주기 위함).
class IndexExprNode : public SyntaxNode {
public:
	IndexExprNode() { type = NodeType::IndexExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }

	Token closeBracketToken;
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
